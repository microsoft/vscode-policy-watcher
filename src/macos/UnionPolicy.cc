/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#include "UnionPolicy.hh"
#include <CoreFoundation/CoreFoundation.h>

UnionPolicy::UnionPolicy(const std::string name, const std::string &productName, const std::vector<std::string> &types)
    : Policy(name),
      appID(CFStringCreateWithCString(nullptr, productName.c_str(), kCFStringEncodingUTF8)),
      key(CFStringCreateWithCString(nullptr, name.c_str(), kCFStringEncodingUTF8))
{
  for (const auto &type : types) {
    acceptsBoolean = acceptsBoolean || type == "boolean";
    acceptsNumber = acceptsNumber || type == "number";
    acceptsString = acceptsString || type == "string";
  }
}

UnionPolicy::~UnionPolicy()
{
  CFRelease(appID);
  CFRelease(key);
}

PolicyRefreshResult UnionPolicy::refresh()
{
  auto next = read();
  if (value == next)
    return value.has_value() ? PolicyRefreshResult::Unchanged : PolicyRefreshResult::NotSet;
  auto removed = value.has_value() && !next.has_value();
  value = next;
  return removed ? PolicyRefreshResult::Removed : PolicyRefreshResult::Updated;
}

Napi::Value UnionPolicy::getValue(Napi::Env env) const
{
  if (!value.has_value())
    return env.Undefined();
  if (std::holds_alternative<bool>(*value))
    return Napi::Boolean::New(env, std::get<bool>(*value));
  if (std::holds_alternative<double>(*value))
    return Napi::Number::New(env, std::get<double>(*value));
  return Napi::String::New(env, std::get<std::string>(*value));
}

std::optional<UnionPolicyValue> UnionPolicy::read() const
{
  if (!CFPreferencesAppValueIsForced(key, appID))
    return std::nullopt;
  auto pref = CFPreferencesCopyAppValue(key, appID);
  if (pref == nullptr)
    return std::nullopt;

  std::optional<UnionPolicyValue> result;
  auto type = CFGetTypeID(pref);
  if (type == CFBooleanGetTypeID() && acceptsBoolean) {
    result = pref == kCFBooleanTrue;
  } else if (type == CFNumberGetTypeID() && acceptsNumber) {
    long long number;
    if (CFNumberGetValue(static_cast<CFNumberRef>(pref), kCFNumberLongLongType, &number))
      result = static_cast<double>(number);
  } else if (type == CFStringGetTypeID() && acceptsString) {
    CFIndex length = CFStringGetLength(static_cast<CFStringRef>(pref));
    CFIndex maxSize = CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8) + 1;
    std::vector<char> buffer(maxSize);
    if (CFStringGetCString(static_cast<CFStringRef>(pref), buffer.data(), maxSize, kCFStringEncodingUTF8))
      result = std::string(buffer.data());
  }
  CFRelease(pref);
  return result;
}
