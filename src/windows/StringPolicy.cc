/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#include "StringPolicy.hh"

using namespace Napi;

StringPolicy::StringPolicy(const std::string name, const std::string &productName)
    : RegistryPolicy(name, productName, {REG_SZ, REG_MULTI_SZ}) {}

std::string StringPolicy::parseRegistryValue(LPBYTE buffer, DWORD bufferSize, DWORD type) const
{
  if (type == REG_MULTI_SZ)
  {
    std::string result;
    const char *current = reinterpret_cast<char *>(buffer);
    const char *end = reinterpret_cast<char *>(buffer) + bufferSize;

    while (current < end && *current != '\0')
    {
      std::string line(current);
      if (!result.empty())
      {
        result += '\n';
      }
      result += line;
      current += line.length() + 1; // Skip past null terminator
    }

    return result;
  }

  // REG_SZ handling
  return std::string(reinterpret_cast<char *>(buffer), bufferSize - 1);
}

Value StringPolicy::getJSValue(Env env, std::string value) const
{
  return String::New(env, value);
}
