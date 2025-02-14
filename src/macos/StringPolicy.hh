/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#ifndef STRING_POLICY_H
#define STRING_POLICY_H

#include <napi.h>
#include "PreferencesPolicy.hh"

using namespace Napi;

class StringPolicy : public PreferencesPolicy<std::string>
{
public:
    StringPolicy(const std::string name, const std::string &productName);
protected:
    Value getJSValue(Env env, std::string value) const override;
    std::optional<std::string> read() const override;
};

#endif