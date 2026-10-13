/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#include "UnionPolicy.hh"
#include <algorithm>
#include <cstdint>
#include <cstring>
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
  auto machine = read(HKEY_LOCAL_MACHINE);
  auto next = machine;
  if (machine.status != PolicyReadStatus::Value)
    next = read(HKEY_CURRENT_USER);
  const bool nextInvalid = machine.status == PolicyReadStatus::Invalid || next.status == PolicyReadStatus::Invalid;
  if (next.status != PolicyReadStatus::Value) {
    const bool changed = value.has_value() || invalid != nextInvalid;
    value.reset();
    invalid = nextInvalid;
    return changed ? PolicyRefreshResult::Removed : (nextInvalid ? PolicyRefreshResult::Unchanged : PolicyRefreshResult::NotSet);
  }
  if (value == next.value && invalid == nextInvalid)
    return PolicyRefreshResult::Unchanged;
  value = next.value;
  invalid = nextInvalid;
  return PolicyRefreshResult::Updated;
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

bool UnionPolicy::isInvalid() const
{
  return invalid;
}

PolicyReadResult<UnionPolicyValue> UnionPolicy::read(HKEY root) const
{
  HKEY key;
  auto openStatus = RegOpenKeyEx(root, registryKey.c_str(), 0, KEY_READ, &key);
  if (openStatus != ERROR_SUCCESS)
    return {openStatus == ERROR_FILE_NOT_FOUND ? PolicyReadStatus::Missing : PolicyReadStatus::Error, std::nullopt};

  DWORD type;
  DWORD size = 0;
  auto status = RegQueryValueEx(key, name.c_str(), nullptr, &type, nullptr, &size);
  if (status == ERROR_FILE_NOT_FOUND) {
    RegCloseKey(key);
    return {PolicyReadStatus::Missing, std::nullopt};
  }
  if (status != ERROR_SUCCESS && status != ERROR_MORE_DATA) {
    RegCloseKey(key);
    return {PolicyReadStatus::Error, std::nullopt};
  }

  std::vector<BYTE> buffer(size);
  status = RegQueryValueEx(key, name.c_str(), nullptr, &type, buffer.data(), &size);
  RegCloseKey(key);
  if (status != ERROR_SUCCESS)
    return {PolicyReadStatus::Error, std::nullopt};

  if (type == REG_DWORD && acceptsBoolean && size == sizeof(DWORD))
  {
    DWORD value;
    std::memcpy(&value, buffer.data(), sizeof(value));
    return {PolicyReadStatus::Value, value != 0};
  }
  if (type == REG_DWORD && acceptsNumber && size == sizeof(DWORD))
  {
    DWORD value;
    std::memcpy(&value, buffer.data(), sizeof(value));
    return {PolicyReadStatus::Value, static_cast<double>(value)};
  }
  if (type == REG_QWORD && acceptsNumber && size == sizeof(std::int64_t))
  {
    std::int64_t value;
    std::memcpy(&value, buffer.data(), sizeof(value));
    return {PolicyReadStatus::Value, static_cast<double>(value)};
  }
  if ((type == REG_SZ || type == REG_MULTI_SZ) && acceptsString) {
    if (type == REG_SZ && size == 0)
      return {PolicyReadStatus::Value, std::string()};
    const char *begin = reinterpret_cast<char *>(buffer.data());
    const char *end = begin + size;
    if (type == REG_SZ) {
      const char *terminator = std::find(begin, end, '\0');
      return {PolicyReadStatus::Value, std::string(begin, terminator)};
    }
    if (size < 2 || end[-1] != '\0' || end[-2] != '\0')
      return {PolicyReadStatus::Invalid, std::nullopt};
    std::string result;
    const char *current = begin;
    while (current < end && *current != '\0') {
      const char *terminator = std::find(current, end, '\0');
      if (!result.empty())
        result += '\n';
      result.append(current, terminator);
      current = terminator == end ? end : terminator + 1;
    }
    return {PolicyReadStatus::Value, result};
  }
  return {PolicyReadStatus::Invalid, std::nullopt};
}
