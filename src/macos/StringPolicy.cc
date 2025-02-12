/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#include "StringPolicy.hh"

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
  {
    CFRelease(pref);
    return std::nullopt;
  }

  CFIndex length = CFStringGetLength((CFStringRef)pref);
  CFIndex maxSize = CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8) + 1;
  std::vector<char> buffer(maxSize);

  if (CFStringGetCString((CFStringRef)pref, buffer.data(), maxSize, kCFStringEncodingUTF8))
  {
    std::string result(buffer.data());
    CFRelease(pref);
    return result;
  }

  CFRelease(pref);
  return std::nullopt;
}
