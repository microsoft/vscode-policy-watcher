/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#ifndef REGISTRY_POLICY_H
#define REGISTRY_POLICY_H

#include <napi.h>
#include <windows.h>
#include <optional>
#include <vector>
#include <algorithm>
#include "../Policy.hh"

using namespace Napi;

template <typename T>
class RegistryPolicy : public Policy
{
public:
  RegistryPolicy(const std::string name, const std::string &productName, const std::vector<DWORD>& types, const std::string &customRegistryPath = "")
      : Policy(name),
        registryKey(customRegistryPath.empty() ? ("Software\\Policies\\Microsoft\\" + productName) : customRegistryPath),
        supportedTypes(types) {}

  PolicyRefreshResult refresh()
  {
    auto machine = read(HKEY_LOCAL_MACHINE);
    auto next = machine.status == PolicyReadStatus::Value ? machine : read(HKEY_CURRENT_USER);
    const bool nextInvalid = machine.status == PolicyReadStatus::Invalid || next.status == PolicyReadStatus::Invalid;
    if (next.status != PolicyReadStatus::Value)
    {
      const bool changed = value.has_value() || invalid != nextInvalid;
      value.reset();
      invalid = nextInvalid;
      return changed ? PolicyRefreshResult::Removed : (nextInvalid ? PolicyRefreshResult::Unchanged : PolicyRefreshResult::NotSet);
    }

    if (value != next.value || invalid != nextInvalid)
    {
      value = next.value;
      invalid = nextInvalid;
      return PolicyRefreshResult::Updated;
    }

    return PolicyRefreshResult::Unchanged;
  }

  Value getValue(Env env) const
  {
    if (!value.has_value())
      return env.Undefined();

    return getJSValue(env, value.value());
  }

  bool isInvalid() const
  {
    return invalid;
  }

protected:
  virtual std::optional<T> parseRegistryValue(LPBYTE buffer, DWORD bufferSize, DWORD type) const = 0;
  virtual Value getJSValue(Env env, T value) const = 0;

private:
  const std::string registryKey;
  const std::vector<DWORD> supportedTypes;
  std::optional<T> value;
  bool invalid = false;

  PolicyReadResult<T> read(HKEY root)
  {
    HKEY hKey;

    auto openResult = RegOpenKeyEx(root, registryKey.c_str(), 0, KEY_READ, &hKey);
    if (ERROR_SUCCESS != openResult)
      return {openResult == ERROR_FILE_NOT_FOUND ? PolicyReadStatus::Missing : PolicyReadStatus::Error, std::nullopt};

    DWORD bufferSize = 0;
    DWORD type;

    // First query to get required buffer size
    auto result = RegQueryValueEx(hKey, name.c_str(), 0, &type, nullptr, &bufferSize);

    if (ERROR_FILE_NOT_FOUND == result)
    {
      RegCloseKey(hKey);
      return {PolicyReadStatus::Missing, std::nullopt};
    }

    if (ERROR_SUCCESS != result && ERROR_MORE_DATA != result)
    {
      RegCloseKey(hKey);
      return {PolicyReadStatus::Error, std::nullopt};
    }

    if (std::find(supportedTypes.begin(), supportedTypes.end(), type) == supportedTypes.end())
    {
      RegCloseKey(hKey);
      return {PolicyReadStatus::Invalid, std::nullopt};
    }

    std::vector<BYTE> buffer(bufferSize);
    result = RegQueryValueEx(hKey, name.c_str(), 0, &type, buffer.data(), &bufferSize);
    RegCloseKey(hKey);

    if (ERROR_SUCCESS != result)
      return {PolicyReadStatus::Error, std::nullopt};

    auto value = parseRegistryValue(buffer.data(), bufferSize, type);
    return {value.has_value() ? PolicyReadStatus::Value : PolicyReadStatus::Invalid, value};
  }
};

#endif