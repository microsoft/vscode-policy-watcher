/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#include "BooleanPolicy.hh"
#include <cstring>
#include <iostream>

using namespace Napi;

BooleanPolicy::BooleanPolicy(const std::string& name, const std::string& productName, const std::string &registryPath)
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

Value BooleanPolicy::getJSValue(Env env, bool value) const
{
  return Boolean::New(env, value);
}
