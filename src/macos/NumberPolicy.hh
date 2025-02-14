/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#ifndef MAC_NUMBER_POLICY_H
#define MAC_NUMBER_POLICY_H

#include <napi.h>
#include "PreferencesPolicy.hh"

using namespace Napi;

class NumberPolicy : public PreferencesPolicy<long long>
{
public:
  NumberPolicy(const std::string name, const std::string &productName);
  std::optional<long long> read() const override;

protected:
  Value getJSValue(Env env, long long value) const override;
};

#endif
