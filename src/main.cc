/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#include <napi.h>
#include <algorithm>
#include <vector>

#include "Policy.hh"
#include "PolicyWatcher.hh"

using namespace Napi;

Value DisposeWatcher(const CallbackInfo &info)
{
  auto watcher = (PolicyWatcher *)info.Data();
  watcher->Dispose();
  return info.Env().Null();
}

Value CreateWatcher(const CallbackInfo &info)
{
  auto env = info.Env();

#if !defined(WINDOWS) && !defined(MACOS)
  throw TypeError::New(env, "Unsupported platform");
#endif

  if (info.Length() < 3)
    throw TypeError::New(env, "Expected 3 arguments");
  else if (!info[0].IsString())
    throw TypeError::New(env, "Expected first arg to be string");
  else if (!info[1].IsObject())
    throw TypeError::New(env, "Expected second arg to be object");
  else if (!info[2].IsFunction())
    throw TypeError::New(env, "Expected third arg to be function");

  std::string registryPath;

  if (info.Length() > 3 && !info[3].IsUndefined())
  {
    if (!info[3].IsObject())
      throw TypeError::New(env, "Expected fourth arg to be object");

    auto rawOptions = info[3].As<Object>();
    auto rawRegistryPath = rawOptions.Get("registryPath");

    if (!rawRegistryPath.IsUndefined())
    {
      if (!rawRegistryPath.IsString())
        throw TypeError::New(env, "Expected options.registryPath to be string");

      registryPath = rawRegistryPath.As<String>();
    }
  }

  auto rawPolicies = info[1].As<Object>();
  auto watcher = new PolicyWatcher(info[0].As<String>(), info[2].As<Function>(), registryPath);

  for (auto const &item : rawPolicies)
  {
    auto rawPolicyName = item.first.As<String>();
    auto rawPolicyValue = static_cast<Value>(item.second);

    if (!rawPolicyValue.IsObject())
      throw TypeError::New(env, "Expected policy to be object");

    auto rawPolicy = rawPolicyValue.As<Object>();
    auto rawPolicyType = rawPolicy.Get("type");
    std::vector<std::string> policyTypes;
    if (rawPolicyType.IsString()) {
      policyTypes.push_back(std::string(rawPolicyType.As<String>()));
    } else if (rawPolicyType.IsArray()) {
      auto rawPolicyTypes = rawPolicyType.As<Array>();
      if (rawPolicyTypes.Length() == 0)
        throw TypeError::New(env, "Expected policy type array to be non-empty");
      for (uint32_t i = 0; i < rawPolicyTypes.Length(); i++) {
        auto rawType = rawPolicyTypes.Get(i);
        if (!rawType.IsString())
          throw TypeError::New(env, "Expected policy type array entries to be strings");
        auto type = std::string(rawType.As<String>());
        if (std::find(policyTypes.begin(), policyTypes.end(), type) == policyTypes.end())
          policyTypes.push_back(type);
      }
    } else {
      throw TypeError::New(env, "Expected policy type to be a string or non-empty string array");
    }

    for (const auto &policyType : policyTypes) {
      if (policyType != "string" && policyType != "number" && policyType != "boolean")
        throw TypeError::New(env, "Unknown policy type '" + policyType + "'");
    }

    if (policyTypes.size() > 1) {
      watcher->AddUnionPolicy(rawPolicyName.As<String>(), policyTypes);
    } else if (policyTypes[0] == "string") {
        watcher->AddStringPolicy(rawPolicyName.As<String>());
    } else if (policyTypes[0] == "number") {
        watcher->AddNumberPolicy(rawPolicyName.As<String>());
    } else {
        watcher->AddBooleanPolicy(rawPolicyName.As<String>());
    }
  }

  watcher->Queue();

  auto result = Object::New(env);
  result.Set(String::New(env, "dispose"), Function::New(env, DisposeWatcher, "disposeWatcher", watcher));

  return result;
}

Object Init(Env env, Object exports)
{
  return Function::New(env, CreateWatcher, "createWatcher");
}

NODE_API_MODULE(vscodepolicy, Init)