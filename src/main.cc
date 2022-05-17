/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#include <napi.h>
#include <vector>

#include "Policy.hh"
#include "StringPolicy.hh"
#include "NumberPolicy.hh"
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

  if (info.Length() < 3)
    throw TypeError::New(env, "Expected 3 arguments");
  else if (!info[0].IsString())
    throw TypeError::New(env, "Expected first arg to be string");
  else if (!info[1].IsObject())
    throw TypeError::New(env, "Expected second arg to be object");
  else if (!info[2].IsFunction())
    throw TypeError::New(env, "Expected third arg to be function");

  auto productName = info[0].As<String>();
  auto rawPolicies = info[1].As<Object>();
  auto policies = std::vector<std::unique_ptr<Policy>>();

  for (auto const &item : rawPolicies)
  {
    auto rawPolicyName = item.first.As<String>();
    auto rawPolicyValue = static_cast<Value>(item.second);

    if (!rawPolicyValue.IsObject())
      throw TypeError::New(env, "Expected policy to be object");

    auto rawPolicy = rawPolicyValue.As<Object>();
    auto rawPolicyType = rawPolicy.Get("type");

    if (!rawPolicyType.IsString())
      throw TypeError::New(env, "Expected policy type to be string");

    auto policyType = std::string(rawPolicyType.As<String>());

    if (policyType == "string")
      policies.push_back(std::make_unique<StringPolicy>(rawPolicyName.As<String>(), productName));
    else if (policyType == "number")
      policies.push_back(std::make_unique<NumberPolicy>(rawPolicyName.As<String>(), productName));
    else
      throw TypeError::New(env, "Unknown policy type '" + policyType + "'");
  }

  auto watcher = new PolicyWatcher(info[2].As<Function>(), std::move(policies));
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