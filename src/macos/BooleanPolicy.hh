/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#ifndef MAC_BOOLEAN_POLICY_H
#define MAC_BOOLEAN_POLICY_H

#include <napi.h>
#include "PreferencesPolicy.hh"

using namespace Napi;

class BooleanPolicy : public PreferencesPolicy<bool>
{
public:
  BooleanPolicy(const std::string name, const std::string &productName);
  std::optional<bool> read() const override;

protected:
  Value getJSValue(Env env, bool value) const override;
};

#endif
