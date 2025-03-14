/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#ifndef BOOLEAN_POLICY_H
#define BOOLEAN_POLICY_H

#include <napi.h>
#include <windows.h>
#include "RegistryPolicy.hh"

using namespace Napi;

class BooleanPolicy : public RegistryPolicy<bool>
{
public:
  BooleanPolicy(const std::string& name, const std::string &productName);

protected:
  bool parseRegistryValue(LPBYTE buffer, DWORD bufferSize, DWORD type) const;
  Value getJSValue(Env env, bool value) const;
};

#endif