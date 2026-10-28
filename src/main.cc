/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#include <napi.h>
#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "Policy.hh"
#include "PolicyWatcher.hh"

namespace {

Napi::Value DisposeWatcher(const Napi::CallbackInfo &info)
{
  auto *watcher = static_cast<PolicyWatcher *>(info.Data());
  watcher->Dispose();
  return info.Env().Null();
}

Napi::Value CreateWatcher(const Napi::CallbackInfo &info)
{
  auto env = info.Env();

#if !defined(WINDOWS) && !defined(MACOS)
  Napi::TypeError::New(env, "Unsupported platform").ThrowAsJavaScriptException();
  return env.Undefined();
#endif

  if (info.Length() < 3)
  {
    Napi::TypeError::New(env, "Expected 3 arguments").ThrowAsJavaScriptException();
    return env.Undefined();
  }
  if (!info[0].IsString())
  {
    Napi::TypeError::New(env, "Expected first arg to be string").ThrowAsJavaScriptException();
    return env.Undefined();
  }
  if (!info[1].IsObject())
  {
    Napi::TypeError::New(env, "Expected second arg to be object").ThrowAsJavaScriptException();
    return env.Undefined();
  }
  if (!info[2].IsFunction())
  {
    Napi::TypeError::New(env, "Expected third arg to be function").ThrowAsJavaScriptException();
    return env.Undefined();
  }

  std::string registryPath;

  if (info.Length() > 3 && !info[3].IsUndefined())
  {
    if (!info[3].IsObject())
    {
      Napi::TypeError::New(env, "Expected fourth arg to be object").ThrowAsJavaScriptException();
      return env.Undefined();
    }

    auto rawOptions = info[3].As<Napi::Object>();
    auto rawRegistryPath = rawOptions.Get("registryPath");

    if (!rawRegistryPath.IsUndefined())
    {
      if (!rawRegistryPath.IsString())
      {
        Napi::TypeError::New(env, "Expected options.registryPath to be string").ThrowAsJavaScriptException();
        return env.Undefined();
      }

      registryPath = rawRegistryPath.As<Napi::String>();
    }
  }

  auto rawPolicies = info[1].As<Napi::Object>();
  auto policyNames = rawPolicies.GetPropertyNames();
  uint32_t policyCount = policyNames.Length();
  auto watcher = std::make_unique<PolicyWatcher>(
      info[0].As<Napi::String>(),
      info[2].As<Napi::Function>(),
      registryPath);

  std::vector<std::string> policyTypes;
  for (uint32_t idx = 0; idx < policyCount; idx++)
  {
    auto rawPolicyName = policyNames.Get(idx);
    const std::string policyName = rawPolicyName.As<Napi::String>();
    auto rawPolicyValue = rawPolicies.Get(rawPolicyName);

    if (!rawPolicyValue.IsObject())
    {
      Napi::TypeError::New(env, "Expected policy to be object").ThrowAsJavaScriptException();
      return env.Undefined();
    }

    auto rawPolicy = rawPolicyValue.As<Napi::Object>();
    auto rawPolicyType = rawPolicy.Get("type");
    policyTypes.clear();
    if (rawPolicyType.IsString())
    {
      policyTypes.push_back(rawPolicyType.As<Napi::String>());
    }
    else if (rawPolicyType.IsArray())
    {
      auto rawPolicyTypes = rawPolicyType.As<Napi::Array>();
      uint32_t length = rawPolicyTypes.Length();
      if (length == 0)
      {
        Napi::TypeError::New(env, "Expected policy type array to be non-empty").ThrowAsJavaScriptException();
        return env.Undefined();
      }
      policyTypes.reserve(length);
      for (uint32_t i = 0; i < length; i++)
      {
        auto rawType = rawPolicyTypes.Get(i);
        if (!rawType.IsString())
        {
          Napi::TypeError::New(env, "Expected policy type array entries to be strings").ThrowAsJavaScriptException();
          return env.Undefined();
        }
        std::string type = rawType.As<Napi::String>();
        if (std::ranges::find(policyTypes, type) == policyTypes.end())
        {
          policyTypes.push_back(type);
        }
      }
    }
    else
    {
      Napi::TypeError::New(env, "Expected policy type to be a string or non-empty string array").ThrowAsJavaScriptException();
      return env.Undefined();
    }

    for (const auto &policyType : policyTypes)
    {
      if (policyType != "string" && policyType != "number" && policyType != "boolean")
      {
        Napi::TypeError::New(env, "Unknown policy type '" + policyType + "'").ThrowAsJavaScriptException();
        return env.Undefined();
      }
    }

    if (policyTypes.size() > 1)
    {
      watcher->AddUnionPolicy(policyName, policyTypes);
    }
    else if (policyTypes[0] == "string")
    {
      watcher->AddStringPolicy(policyName);
    }
    else if (policyTypes[0] == "number")
    {
      watcher->AddNumberPolicy(policyName);
    }
    else
    {
      watcher->AddBooleanPolicy(policyName);
    }
  }

  if (!watcher->Start(env))
  {
    return env.Undefined();
  }

  auto *rawWatcher = watcher.release();
  auto result = Napi::Object::New(env);
  result.Set(Napi::String::New(env, "dispose"), Napi::Function::New(env, DisposeWatcher, "disposeWatcher", rawWatcher));

  return result;
}

Napi::Object Init(Napi::Env env, Napi::Object exports)
{
  return Napi::Function::New(env, CreateWatcher, "createWatcher");
}

} // namespace

NODE_API_MODULE(vscodepolicy, Init)