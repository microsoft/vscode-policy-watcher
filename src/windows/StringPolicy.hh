/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#ifndef STRING_POLICY_H
#define STRING_POLICY_H

#include <napi.h>
#include <string>
#include <windows.h>
#include "RegistryPolicy.hh"

class StringPolicy : public RegistryPolicy<std::string>
{
public:
  StringPolicy(const std::string &name, const std::string &productName, const std::string &registryPath = "");

protected:
  std::optional<std::string> parseRegistryValue(LPBYTE buffer, DWORD bufferSize, DWORD type) const override;
  Napi::Value getJSValue(Napi::Env env, const std::string &value) const override;
};

#endif