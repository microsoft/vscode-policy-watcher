/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#include "StringPolicy.hh"

using namespace Napi;

StringPolicy::StringPolicy(const std::string name, const std::string &productName)
    : RegistryPolicy(name, productName, REG_SZ) {}

std::string StringPolicy::parseRegistryValue(LPBYTE buffer, DWORD bufferSize) const
{
  return std::string(reinterpret_cast<char *>(buffer), bufferSize - 1);
}

Value StringPolicy::getJSValue(Env env, std::string value) const
{
  return String::New(env, value);
}
