/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#include "BooleanPolicy.hh"

using namespace Napi;

BooleanPolicy::BooleanPolicy(const std::string& name, const std::string& productName)
    : RegistryPolicy(name, productName, {REG_QWORD}) {}

long long BooleanPolicy::parseRegistryValue(LPBYTE buffer, DWORD bufferSize, DWORD type) const
{
  // TODO: Unimplemented
  return 0; 
}

Value BooleanPolicy::getJSValue(Env env, long long value) const
{
  return Boolean::New(env, static_cast<bool>(value));
}
