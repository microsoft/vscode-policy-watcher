/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#include "NumberPolicy.hh"
#include <cstdint>
#include <cstring>

using namespace Napi;

NumberPolicy::NumberPolicy(const std::string name, const std::string &productName, const std::string &registryPath)
    : RegistryPolicy(name, productName, {REG_DWORD, REG_QWORD}, registryPath) {}

std::optional<long long> NumberPolicy::parseRegistryValue(LPBYTE buffer, DWORD bufferSize, DWORD type) const
{
  if (type == REG_DWORD && bufferSize == sizeof(DWORD))
  {
    DWORD value;
    std::memcpy(&value, buffer, sizeof(value));
    return static_cast<long long>(value);
  }

  if (type == REG_QWORD && bufferSize == sizeof(QWORD))
  {
    std::int64_t value;
    std::memcpy(&value, buffer, sizeof(value));
    return static_cast<long long>(value);
  }

  return std::nullopt;
}

Value NumberPolicy::getJSValue(Env env, long long value) const
{
  return Number::New(env, static_cast<double>(value));
}
