/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#ifndef BOOLEAN_POLICY_H
#define BOOLEAN_POLICY_H

#include <napi.h>
#include <string>
#include <windows.h>
#include "RegistryPolicy.hh"

class BooleanPolicy : public RegistryPolicy<bool>
{
public:
  BooleanPolicy(const std::string &name, const std::string &productName, const std::string &registryPath = "");

protected:
  std::optional<bool> parseRegistryValue(LPBYTE buffer, DWORD bufferSize, DWORD type) const override;
  Napi::Value getJSValue(Napi::Env env, const bool &value) const override;
};

#endif