/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#include "StringPolicy.hh"  
#include <iostream>

using namespace Napi;

StringPolicy::StringPolicy(const std::string name, const std::string &productName)
    : PreferencesPolicy(name, productName) 
    {
    }

Value StringPolicy::getJSValue(Env env, std::string value) const
{
  return String::New(env, value);
}

std::optional<std::string> StringPolicy::read() const
{
  auto pref = CFPreferencesCopyAppValue(key, appID);

  if (pref == NULL)
    return std::nullopt;

  if (CFGetTypeID(pref) != CFStringGetTypeID())
    return std::nullopt;

  auto result = std::string(CFStringGetCStringPtr((CFStringRef)pref, kCFStringEncodingUTF8));
  CFRelease(pref);
  return result;
}
