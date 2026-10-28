/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#include "BooleanPolicy.hh"
#include <cstring>

BooleanPolicy::BooleanPolicy(const std::string &name, const std::string &productName, const std::string &registryPath)
    : RegistryPolicy(name, productName, {REG_DWORD}, registryPath) {}

std::optional<bool> BooleanPolicy::parseRegistryValue(LPBYTE buffer, DWORD bufferSize, DWORD type) const
{
  if (type != REG_DWORD || bufferSize != sizeof(DWORD))
  {
    return std::nullopt;
  }

  DWORD value;
  std::memcpy(&value, buffer, sizeof(value));
  return (value != 0);
}

Napi::Value BooleanPolicy::getJSValue(Napi::Env env, const bool &value) const
{
  return Napi::Boolean::New(env, value);
}
