/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#include "BooleanPolicy.hh"

using namespace Napi;

BooleanPolicy::BooleanPolicy(const std::string name, const std::string &productName)
    : PreferencesPolicy(name, productName)
{
}

Value BooleanPolicy::getJSValue(Env env, bool value) const
{
  return Napi::Boolean::New(env, value);
}

std::optional<bool> BooleanPolicy::read() const
{
  auto pref = CFPreferencesCopyAppValue(key, appID);

  if (pref == NULL)
    return std::nullopt;

  if (CFGetTypeID(pref) != CFBooleanGetTypeID())
  {
    CFRelease(pref);
    return std::nullopt;
  }

  bool value = (pref == kCFBooleanTrue);
  CFRelease(pref);
  return value;
}
