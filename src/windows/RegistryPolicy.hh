/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#ifndef REGISTRY_POLICY_H
#define REGISTRY_POLICY_H

#include <napi.h>
#include <windows.h>
#include <algorithm>
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include "../Policy.hh"

template <typename T>
class RegistryPolicy : public Policy
{
public:
  RegistryPolicy(const std::string &name, const std::string &productName, const std::vector<DWORD> &types, const std::string &customRegistryPath = "")
      : Policy(name),
        registryKey(customRegistryPath.empty() ? ("Software\\Policies\\Microsoft\\" + productName) : customRegistryPath),
        supportedTypes(types) {}

  PolicyRefreshResult refresh() override
  {
    auto machine = read(HKEY_LOCAL_MACHINE);

    if (machine.has_value())
    {
      if (value != machine)
      {
        value = std::move(machine);
        return PolicyRefreshResult::Updated;
      }

      return PolicyRefreshResult::Unchanged;
    }

    auto user = read(HKEY_CURRENT_USER);

    // Check for no value or removal
    if (!user.has_value())
    {
      if (!value.has_value())
        return PolicyRefreshResult::NotSet;

      value.reset();
      return PolicyRefreshResult::Removed;
    }

    if (value != user)
    {
      value = std::move(user);
      return PolicyRefreshResult::Updated;
    }

    return PolicyRefreshResult::Unchanged;
  }

  Napi::Value getValue(Napi::Env env) const override
  {
    if (!value.has_value())
      return env.Undefined();

    return getJSValue(env, value.value());
  }

protected:
  virtual std::optional<T> parseRegistryValue(LPBYTE buffer, DWORD bufferSize, DWORD type) const = 0;
  virtual Napi::Value getJSValue(Napi::Env env, const T &value) const = 0;

private:
  const std::string registryKey;
  const std::vector<DWORD> supportedTypes;
  std::optional<T> value;

  std::optional<T> read(HKEY root)
  {
    HKEY hKey;

    if (ERROR_SUCCESS != RegOpenKeyEx(root, registryKey.c_str(), 0, KEY_READ, &hKey))
      return std::nullopt;

    DWORD bufferSize = 0;
    DWORD type;

    // First query to get required buffer size
    auto result = RegQueryValueEx(hKey, name.c_str(), nullptr, &type, nullptr, &bufferSize);

    if (ERROR_SUCCESS != result && ERROR_MORE_DATA != result)
    {
      RegCloseKey(hKey);
      return std::nullopt;
    }

    if (std::ranges::find(supportedTypes, type) == supportedTypes.end())
    {
      RegCloseKey(hKey);
      return std::nullopt;
    }

    std::vector<BYTE> buffer(bufferSize);
    result = RegQueryValueEx(hKey, name.c_str(), nullptr, &type, buffer.data(), &bufferSize);
    RegCloseKey(hKey);

    if (ERROR_SUCCESS != result)
      return std::nullopt;

    return parseRegistryValue(buffer.data(), bufferSize, type);
  }
};

#endif