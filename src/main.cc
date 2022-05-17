/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#include <napi.h>
#include <windows.h>
#include <userenv.h>
#include <thread>
#include <vector>
#include <list>
#include <optional>

#include "Policy.hh"
#include "StringPolicy.hh"
#include "NumberPolicy.hh"

using namespace Napi;

void CallJs(Env env, Function callback, Reference<Value> *context, std::list<const Policy *> *data);

struct PolicyWatcher
{
  std::vector<std::unique_ptr<Policy>> policies;
  HANDLE hDispose;
  std::unique_ptr<std::thread> thread;

  PolicyWatcher(std::vector<std::unique_ptr<Policy>> _policies, HANDLE _hDispose)
      : policies(std::move(_policies)),
        hDispose(_hDispose) {}

  void poll(
      HANDLE *handles,
      size_t handleSize,
      TypedThreadSafeFunction<Reference<Value>, std::list<const Policy *>, CallJs> &tsfn)
  {
    bool first = true;

    while (TRUE)
    {
      std::list<const Policy *> *updatedPolicies = nullptr;

      for (auto &policy : policies)
      {
        if (policy->refresh())
        {
          if (updatedPolicies == nullptr)
            updatedPolicies = new std::list<const Policy *>();

          updatedPolicies->push_back(policy.get());
        }
      }

      if (first || (updatedPolicies != nullptr && updatedPolicies->size() > 0))
        if (napi_ok != tsfn.BlockingCall(updatedPolicies))
          break;

      first = false;

      auto dwResult = WaitForMultipleObjects(handleSize, handles, FALSE, INFINITE);

      if (dwResult == WAIT_FAILED || (dwResult - WAIT_OBJECT_0) == 0 || (dwResult - WAIT_OBJECT_0) == 1 /* someone called dispose() */)
        break;
    }
  }
};

void CallJs(
    Env env,
    Function callback,
    Reference<Value> *context,
    std::list<const Policy *> *updatedPolicies)
{
  if (env != nullptr)
  {
    if (callback != nullptr)
    {
      auto result = Object::New(env);

      if (updatedPolicies != nullptr)
        for (auto const &policy : *updatedPolicies)
          result.Set(policy->name, policy->getValue(env));

      callback.Call(context->Value(), {result});
    }
  }

  if (updatedPolicies != nullptr)
    delete updatedPolicies;
}

void PollForChanges(
    PolicyWatcher *watcher,
    TypedThreadSafeFunction<Reference<Value>, std::list<const Policy *>, CallJs> tsfn)
{
  HANDLE hExit = CreateEvent(NULL, false, false, NULL);

  if (hExit == NULL)
    goto done;

  HANDLE hMachineEvent = CreateEvent(NULL, false, false, NULL);

  if (hMachineEvent == NULL)
    goto done;

  HANDLE hUserEvent = CreateEvent(NULL, false, false, NULL);

  if (hUserEvent == NULL)
    goto done;
  else if (!RegisterGPNotification(hMachineEvent, TRUE))
    goto done;
  else if (!RegisterGPNotification(hUserEvent, FALSE))
    goto done;

  HANDLE handles[4] = {
      hExit,
      watcher->hDispose,
      hMachineEvent,
      hUserEvent,
  };

  watcher->poll(handles, 4, tsfn);

done:
  UnregisterGPNotification(hMachineEvent);
  UnregisterGPNotification(hUserEvent);
  CloseHandle(hExit);
  CloseHandle(hMachineEvent);
  CloseHandle(hUserEvent);
  tsfn.Release();
}

void WaitForWatcher(
    Env,
    PolicyWatcher *watcher,
    Reference<Value> *context)
{
  watcher->thread->join();
  CloseHandle(watcher->hDispose);

  delete watcher;
  delete context;
}

Value DisposeWatcher(const CallbackInfo &info)
{
  auto env = info.Env();
  auto watcher = (PolicyWatcher *)info.Data();

  if (watcher->hDispose != NULL)
  {
    SetEvent(watcher->hDispose);
    watcher->hDispose = NULL;
  }

  return env.Null();
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

  auto hDispose = CreateEvent(NULL, false, false, NULL);

  if (hDispose == NULL)
    throw TypeError::New(env, "Failed to create watcher dispose event");

  auto context = new Reference<Value>(Persistent(info.This()));
  auto watcher = new PolicyWatcher(std::move(policies), hDispose);

  auto tsfn = TypedThreadSafeFunction<Reference<Value>, std::list<const Policy *>, CallJs>::New(
      env,
      info[2].As<Function>(),
      "PolicyWatcher",
      0,
      1,
      context,
      WaitForWatcher,
      watcher);

  watcher->hDispose = hDispose;
  watcher->thread = std::make_unique<std::thread>(PollForChanges, watcher, tsfn);

  auto result = Object::New(env);
  result.Set(String::New(env, "dispose"), Function::New(env, DisposeWatcher, "disposeWatcher", watcher));
  return result;
}

Object Init(Env env, Object exports)
{
  return Function::New(env, CreateWatcher, "createWatcher");
}

NODE_API_MODULE(vscodepolicy, Init)