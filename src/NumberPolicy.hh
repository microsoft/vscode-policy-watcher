/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#ifndef NUMBER_POLICY_H
#define NUMBER_POLICY_H

#include <napi.h>
#include <windows.h>
#include "RegistryPolicy.hh"

using namespace Napi;

class NumberPolicy : public RegistryPolicy<long long>
{
public:
  NumberPolicy(const std::string name, const std::string &productName);

protected:
  long long parseRegistryValue(LPBYTE buffer, DWORD bufferSize) const;
  Value getJSValue(Env env, long long value) const;
};

#endif