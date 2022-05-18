/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#ifndef REGISTRY_POLICY_H
#define REGISTRY_POLICY_H

#include <napi.h>
#include <windows.h>
#include <optional>
#include "Policy.hh"

using namespace Napi;

template <typename T>
class RegistryPolicy : public Policy
{
public:
  RegistryPolicy(const std::string name, const std::string &productName, const DWORD regType)
      : Policy(name),
        registryKey("Software\\Policies\\Microsoft\\" + productName),
        regType(regType) {}

  bool refresh()
  {
    auto machine = read(HKEY_LOCAL_MACHINE);

    if (machine.has_value())
    {
      if (value != machine)
      {
        value = machine;
        return true;
      }

      return false;
    }

    auto user = read(HKEY_CURRENT_USER);

    if (value != user)
    {
      value = user;
      return true;
    }

    return false;
  }

  Value getValue(Env env) const
  {
    if (!value.has_value())
      return env.Undefined();

    return getJSValue(env, value.value());
  }

protected:
  virtual T parseRegistryValue(LPBYTE buffer, DWORD bufferSize) const = 0;
  virtual Value getJSValue(Env env, T value) const = 0;

private:
  const std::string registryKey;
  const DWORD regType;
  std::optional<T> value;

  std::optional<T> read(HKEY root)
  {
    HKEY hKey;

    if (ERROR_SUCCESS != RegOpenKeyEx(root, registryKey.c_str(), 0, KEY_READ, &hKey))
      return std::nullopt;

    BYTE buffer[1024];
    DWORD bufferSize = sizeof(buffer);
    DWORD type;

    auto readResult = RegQueryValueEx(hKey, name.c_str(), 0, &type, buffer, &bufferSize);
    RegCloseKey(hKey);

    if (ERROR_SUCCESS != readResult || type != regType)
      return std::nullopt;

    return std::optional<T>{parseRegistryValue(buffer, bufferSize)};
  }
};

#endif