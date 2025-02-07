/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#ifndef STRING_POLICY_H
#define STRING_POLICY_H

#include <napi.h>
#include <windows.h>
#include "RegistryPolicy.hh"

using namespace Napi;

class StringPolicy : public RegistryPolicy<std::string>
{
public:
  StringPolicy(const std::string name, const std::string &productName);

protected:
  std::string parseRegistryValue(LPBYTE buffer, DWORD bufferSize, DWORD type) const;
  Value getJSValue(Env env, std::string value) const;
};

#endif