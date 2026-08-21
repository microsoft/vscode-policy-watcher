/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#include "UnionPolicy.hh"
#include <windows.h>

UnionPolicy::UnionPolicy(const std::string name, const std::string &productName, const std::string &customRegistryPath, const std::vector<std::string> &types)
    : Policy(name),
      registryKey(customRegistryPath.empty() ? ("Software\\Policies\\Microsoft\\" + productName) : customRegistryPath)
{
  for (const auto &type : types) {
    acceptsBoolean = acceptsBoolean || type == "boolean";
    acceptsNumber = acceptsNumber || type == "number";
    acceptsString = acceptsString || type == "string";
  }
}

PolicyRefreshResult UnionPolicy::refresh()
{
  auto result = read(HKEY_LOCAL_MACHINE);
  if (!result.present)
    result = read(HKEY_CURRENT_USER);

  auto next = result.value;
  if (result.present && !next.has_value() && acceptsString)
    next = std::string();

  if (value == next)
    return value.has_value() ? PolicyRefreshResult::Unchanged : PolicyRefreshResult::NotSet;
  auto removed = value.has_value() && !next.has_value();
  value = next;
  return removed ? PolicyRefreshResult::Removed : PolicyRefreshResult::Updated;
}

Napi::Value UnionPolicy::getValue(Napi::Env env) const
{
  if (!value.has_value())
    return env.Undefined();
  if (std::holds_alternative<bool>(*value))
    return Napi::Boolean::New(env, std::get<bool>(*value));
  if (std::holds_alternative<double>(*value))
    return Napi::Number::New(env, std::get<double>(*value));
  return Napi::String::New(env, std::get<std::string>(*value));
}

UnionPolicy::ReadResult UnionPolicy::read(HKEY root) const
{
  HKEY key;
  if (RegOpenKeyEx(root, registryKey.c_str(), 0, KEY_READ, &key) != ERROR_SUCCESS)
    return {false, std::nullopt};

  DWORD type;
  DWORD size = 0;
  auto status = RegQueryValueEx(key, name.c_str(), nullptr, &type, nullptr, &size);
  if (status != ERROR_SUCCESS && status != ERROR_MORE_DATA) {
    RegCloseKey(key);
    return {false, std::nullopt};
  }

  std::vector<BYTE> buffer(size);
  status = RegQueryValueEx(key, name.c_str(), nullptr, &type, buffer.data(), &size);
  RegCloseKey(key);
  if (status != ERROR_SUCCESS)
    return {true, std::nullopt};

  if (type == REG_DWORD && acceptsBoolean && size == sizeof(DWORD))
    return {true, *reinterpret_cast<DWORD *>(buffer.data()) != 0};
  if (type == REG_QWORD && acceptsNumber && size == sizeof(long long))
    return {true, static_cast<double>(*reinterpret_cast<long long *>(buffer.data()))};
  if ((type == REG_SZ || type == REG_MULTI_SZ) && acceptsString) {
    if (type == REG_SZ)
      return {true, std::string(reinterpret_cast<char *>(buffer.data()), size > 0 ? size - 1 : 0)};
    std::string result;
    const char *current = reinterpret_cast<char *>(buffer.data());
    const char *end = current + size;
    while (current < end && *current != '\0') {
      std::string line(current);
      if (!result.empty())
        result += '\n';
      result += line;
      current += line.length() + 1;
    }
    return {true, result};
  }
  return {true, std::nullopt};
}
