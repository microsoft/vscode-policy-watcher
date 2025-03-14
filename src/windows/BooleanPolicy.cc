/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#include "BooleanPolicy.hh"
#include <iostream>

using namespace Napi;

BooleanPolicy::BooleanPolicy(const std::string& name, const std::string& productName)
  : RegistryPolicy(name, productName, {REG_DWORD}) {}

bool BooleanPolicy::parseRegistryValue(LPBYTE buffer, DWORD bufferSize, DWORD type) const
{
  if (type != REG_DWORD || bufferSize != sizeof(DWORD))
  {
    return false;
  }

  DWORD value = *reinterpret_cast<DWORD*>(buffer);
  return (value != 0);
}

Value BooleanPolicy::getJSValue(Env env, bool value) const
{
  return Boolean::New(env, value);
}
