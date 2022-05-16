/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#include "NumberPolicy.hh"

using namespace Napi;

NumberPolicy::NumberPolicy(const std::string name, const std::string &productName)
    : RegistryPolicy(name, productName, REG_SZ) {}

long long NumberPolicy::parseRegistryValue(LPBYTE buffer, DWORD bufferSize)
{
  return *reinterpret_cast<long long *>(buffer);
}

Value NumberPolicy::getJSValue(Env env, long long value)
{
  return Number::New(env, value);
}
