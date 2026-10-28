/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#include "NumberPolicy.hh"

NumberPolicy::NumberPolicy(const std::string &name, const std::string &productName)
    : PreferencesPolicy(name, productName)
{
}

Napi::Value NumberPolicy::getJSValue(Napi::Env env, const long long &value) const
{
  return Napi::Number::New(env, static_cast<double>(value));
}

std::optional<long long> NumberPolicy::read() const
{
  auto pref = CFPreferencesCopyAppValue(key, appID);

  if (pref == nullptr)
    return std::nullopt;

  if (CFGetTypeID(pref) != CFNumberGetTypeID())
  {
    CFRelease(pref);
    return std::nullopt;
  }

  long long value;
  if (CFNumberGetValue(static_cast<CFNumberRef>(pref), kCFNumberLongLongType, &value))
  {
    CFRelease(pref);
    return value;
  }

  CFRelease(pref);
  return std::nullopt;
}
