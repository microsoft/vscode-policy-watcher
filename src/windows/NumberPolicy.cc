/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#include "NumberPolicy.hh"

using namespace Napi;

NumberPolicy::NumberPolicy(const std::string name, const std::string &productName)
    : RegistryPolicy(name, productName, REG_QWORD) {}

long long NumberPolicy::parseRegistryValue(LPBYTE buffer, DWORD bufferSize) const
{
  return *reinterpret_cast<long long *>(buffer);
}

Value NumberPolicy::getJSValue(Env env, long long value) const
{
  return Number::New(env, static_cast<double>(value));
}
