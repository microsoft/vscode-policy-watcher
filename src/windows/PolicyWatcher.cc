/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#include <userenv.h>
#include <utility>

#include "../PolicyWatcher.hh"
#include "StringPolicy.hh"
#include "NumberPolicy.hh"
#include "BooleanPolicy.hh"
#include "UnionPolicy.hh"

PolicyWatcher::PolicyWatcher(const std::string &productName, const Napi::Function &okCallback, const std::string &registryPath)
    : env(okCallback.Env()),
      productName(productName),
      registryPath(registryPath),
      callbackRef(Napi::Persistent(okCallback))
{
}

PolicyWatcher::~PolicyWatcher()
{
  napi_remove_env_cleanup_hook(env, OnEnvCleanup, this);
  Dispose();
  if (workerThread.joinable())
  {
    workerThread.join();
  }
  if (handles[2])
  {
    UnregisterGPNotification(handles[2]);
  }
  if (handles[3])
  {
    UnregisterGPNotification(handles[3]);
  }
  for (HANDLE handle : handles)
  {
    if (handle)
    {
      CloseHandle(handle);
    }
  }
}

void PolicyWatcher::AddStringPolicy(const std::string &name)
{
  policies.push_back(std::make_unique<StringPolicy>(name, productName, registryPath));
}

void PolicyWatcher::AddNumberPolicy(const std::string &name)
{
  policies.push_back(std::make_unique<NumberPolicy>(name, productName, registryPath));
}

void PolicyWatcher::AddBooleanPolicy(const std::string &name)
{
  policies.push_back(std::make_unique<BooleanPolicy>(name, productName, registryPath));
}

void PolicyWatcher::AddUnionPolicy(const std::string &name, const std::vector<std::string> &types)
{
  policies.push_back(std::make_unique<UnionPolicy>(name, productName, registryPath, types));
}

bool PolicyWatcher::Start(Napi::Env env)
{
  if ((handles[0] = CreateEvent(nullptr, FALSE, FALSE, nullptr)) == nullptr)
  {
    Napi::Error::New(env, "Failed to create exit event").ThrowAsJavaScriptException();
    return false;
  }
  if ((handles[1] = CreateEvent(nullptr, FALSE, FALSE, nullptr)) == nullptr)
  {
    Napi::Error::New(env, "Failed to create dispose event").ThrowAsJavaScriptException();
    return false;
  }
  if ((handles[2] = CreateEvent(nullptr, FALSE, FALSE, nullptr)) == nullptr)
  {
    Napi::Error::New(env, "Failed to create machine GP event").ThrowAsJavaScriptException();
    return false;
  }
  if ((handles[3] = CreateEvent(nullptr, FALSE, FALSE, nullptr)) == nullptr)
  {
    Napi::Error::New(env, "Failed to create user GP event").ThrowAsJavaScriptException();
    return false;
  }
  if (!RegisterGPNotification(handles[2], TRUE))
  {
    Napi::Error::New(env, "Failed to register machine GP event").ThrowAsJavaScriptException();
    return false;
  }
  if (!RegisterGPNotification(handles[3], FALSE))
  {
    Napi::Error::New(env, "Failed to register user GP event").ThrowAsJavaScriptException();
    return false;
  }

  updatedPolicies.reserve(policies.size());
  tsfn = TSFN::New(
      env,
      callbackRef.Value(),
      "PolicyWatcher",
      0,
      1,
      this,
      [](Napi::Env, void *, PolicyWatcher *watcher) {
        delete watcher;
      },
      static_cast<void *>(nullptr));
  callbackRef.Reset();
  workerThread = std::thread(&PolicyWatcher::Execute, this);
  napi_add_env_cleanup_hook(env, OnEnvCleanup, this);
  return true;
}

void PolicyWatcher::OnEnvCleanup(void *arg)
{
  auto *watcher = static_cast<PolicyWatcher *>(arg);
  watcher->Dispose();
  if (watcher->workerThread.joinable())
  {
    watcher->workerThread.join();
  }
}

void PolicyWatcher::Execute()
{
  RefreshPolicies(true);

  while (!disposed.load(std::memory_order_acquire))
  {
    auto dwResult = WaitForMultipleObjects(static_cast<DWORD>(handles.size()), handles.data(), FALSE, INFINITE);

    if (dwResult == WAIT_FAILED || (dwResult - WAIT_OBJECT_0) == 0 || (dwResult - WAIT_OBJECT_0) == 1 /* someone called dispose() */)
    {
      break;
    }

    RefreshPolicies(false);
  }

  tsfn.Release();
}

void PolicyWatcher::RefreshPolicies(bool first)
{
  bool shouldNotify = false;
  {
    std::scoped_lock lock(mutex);
    updatedPolicies.clear();
    bool update = false;
    for (const auto &policy : policies)
    {
      switch (policy->refresh())
      {
      case PolicyRefreshResult::Updated:
      case PolicyRefreshResult::Removed:
        updatedPolicies.push_back(policy.get());
        update = true;
        break;
      case PolicyRefreshResult::Unchanged:
        updatedPolicies.push_back(policy.get());
        break;
      case PolicyRefreshResult::NotSet:
        break;
      }
    }
    shouldNotify = first || update;
  }

  if (shouldNotify)
  {
    tsfn.NonBlockingCall();
  }
}

void PolicyWatcher::CallJS(Napi::Env env, Napi::Function callback, PolicyWatcher *context, void * /*data*/)
{
  if (env == nullptr || callback == nullptr || context->disposed.load(std::memory_order_acquire))
  {
    return;
  }

  Napi::HandleScope scope(env);
  auto result = Napi::Object::New(env);

  {
    std::scoped_lock lock(context->mutex);
    for (const auto *policy : context->updatedPolicies)
    {
      result.Set(policy->name, policy->getValue(env));
    }
  }

  callback.Call({result});
}

void PolicyWatcher::Dispose()
{
  if (!disposed.exchange(true, std::memory_order_acq_rel))
  {
    if (handles[1])
    {
      SetEvent(handles[1]);
    }
  }
}
