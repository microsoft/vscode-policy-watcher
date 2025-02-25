/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#include "NumberPolicy.hh"

using namespace Napi;

NumberPolicy::NumberPolicy(const std::string name, const std::string &productName)
    : PreferencesPolicy(name, productName)
{
}

Value NumberPolicy::getJSValue(Env env, long long value) const
{
  return Number::New(env, static_cast<double>(value));
}

std::optional<long long> NumberPolicy::read() const
{
  auto pref = CFPreferencesCopyAppValue(key, appID);

  if (pref == NULL)
    return std::nullopt;

  if (CFGetTypeID(pref) != CFNumberGetTypeID())
  {
    CFRelease(pref);
    return std::nullopt;
  }

  long long value;
  if (CFNumberGetValue((CFNumberRef)pref, kCFNumberLongLongType, &value))
  {
    CFRelease(pref);
    return value;
  }

  CFRelease(pref);
  return std::nullopt;
}
