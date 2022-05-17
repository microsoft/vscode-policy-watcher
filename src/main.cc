/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#include <napi.h>
#include <vector>

#include "Policy.hh"
#include "PolicyWatcher.hh"
#include "PolicyRegisterer.hh"
#include "StringPolicy.hh"
#include "NumberPolicy.hh"

using namespace Napi;

Value DisposeWatcher(const CallbackInfo &info)
{
  auto watcher = (PolicyWatcher *)info.Data();
  watcher->Dispose();
  return info.Env().Null();
}

Value RegisterPolicyDefinitions(const CallbackInfo &info)
{
  auto env = info.Env();

  if (!info[0].IsObject())
    throw TypeError::New(env, "Expected second arg to be object");

  auto watcher = (PolicyWatcher *)info.Data();
  auto rawPolicies = info[0].As<Object>();
  auto policies = std::vector<std::unique_ptr<Policy>>();

  for (auto const &item : rawPolicies)
  {
    auto rawPolicyValue = static_cast<Value>(item.second);

    if (!rawPolicyValue.IsObject())
      throw TypeError::New(env, "Expected policy to be object");

    auto rawPolicy = rawPolicyValue.As<Object>();
    auto rawPolicyType = rawPolicy.Get("type");

    if (!rawPolicyType.IsString())
      throw TypeError::New(env, "Expected policy type to be string");

    auto policyName = std::string(item.first.As<String>());
    auto policyType = std::string(rawPolicyType.As<String>());

    if (policyType == "string")
      policies.push_back(std::make_unique<StringPolicy>(policyName, watcher->productName));
    else if (policyType == "number")
      policies.push_back(std::make_unique<NumberPolicy>(policyName, watcher->productName));
    else
      throw TypeError::New(env, "Unknown policy type '" + policyType + "'");
  }

  auto registerer = new PolicyRegisterer(env, watcher, std::move(policies));
  auto promise = registerer->Promise();
  registerer->Queue();
  return promise;
}

Value CreateWatcher(const CallbackInfo &info)
{
  auto env = info.Env();

  if (info.Length() < 2)
    throw TypeError::New(env, "Expected 2 arguments");
  else if (!info[0].IsString())
    throw TypeError::New(env, "Expected first arg to be string");
  else if (!info[1].IsFunction())
    throw TypeError::New(env, "Expected second arg to be function");

  auto watcher = new PolicyWatcher(info[0].As<String>(), info[1].As<Function>());
  watcher->Queue();

  auto result = Object::New(env);
  result.Set(String::New(env, "registerPolicyDefinitions"), Function::New(env, RegisterPolicyDefinitions, "registerPolicyDefinitions", watcher));
  result.Set(String::New(env, "dispose"), Function::New(env, DisposeWatcher, "disposeWatcher", watcher));
  return result;
}

Object Init(Env env, Object exports)
{
  return Function::New(env, CreateWatcher, "createWatcher");
}

NODE_API_MODULE(vscodepolicy, Init)