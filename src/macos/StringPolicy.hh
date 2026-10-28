/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#ifndef STRING_POLICY_H
#define STRING_POLICY_H

#include <napi.h>
#include <string>
#include "PreferencesPolicy.hh"

class StringPolicy : public PreferencesPolicy<std::string>
{
public:
    StringPolicy(const std::string &name, const std::string &productName);
protected:
    Napi::Value getJSValue(Napi::Env env, const std::string &value) const override;
    std::optional<std::string> read() const override;
};

#endif