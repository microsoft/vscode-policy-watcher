/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#include "StringPolicy.hh"

StringPolicy::StringPolicy(const std::string &name, const std::string &productName)
    : PreferencesPolicy(name, productName)
{
}

Napi::Value StringPolicy::getJSValue(Napi::Env env, const std::string &value) const
{
  return Napi::String::New(env, value);
}

std::optional<std::string> StringPolicy::read() const
{
  auto pref = CFPreferencesCopyAppValue(key, appID);

  if (pref == nullptr)
    return std::nullopt;

  if (CFGetTypeID(pref) != CFStringGetTypeID())
  {
    CFRelease(pref);
    return std::nullopt;
  }

  auto strRef = static_cast<CFStringRef>(pref);
  CFIndex length = CFStringGetLength(strRef);
  CFIndex maxSize = CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8) + 1;
  std::string result(static_cast<size_t>(maxSize), '\0');

  if (CFStringGetCString(strRef, result.data(), maxSize, kCFStringEncodingUTF8))
  {
    result.resize(std::char_traits<char>::length(result.data()));
    CFRelease(pref);
    return result;
  }

  CFRelease(pref);
  return std::nullopt;
}
