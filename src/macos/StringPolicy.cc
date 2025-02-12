/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#include "StringPolicy.hh"  
#include <iostream>

using namespace Napi;

StringPolicy::StringPolicy(const std::string name, const std::string &productName)
    : PreferencesPolicy(name, productName) {
      std::cout << "Created string policy" << std::endl;
    }

Value StringPolicy::getJSValue(Env env, std::string value) const
{
  return String::New(env, value);
}
