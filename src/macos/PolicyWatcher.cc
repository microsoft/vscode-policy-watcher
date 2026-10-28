/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#include "../PolicyWatcher.hh"
#include "StringPolicy.hh"
#include "NumberPolicy.hh"
#include "BooleanPolicy.hh"
#include "UnionPolicy.hh"

namespace {

void FSEventsCallback(ConstFSEventStreamRef /*streamRef*/,
                      void *clientCallBackInfo,
                      size_t /*numEvents*/,
                      void * /*eventPaths*/,
                      const FSEventStreamEventFlags /*eventFlags*/[],
                      const FSEventStreamEventId /*eventIds*/[])
{
  auto *watcher = static_cast<PolicyWatcher *>(clientCallBackInfo);
  watcher->Execute();
}

} // namespace

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
  if (queue)
  {
    dispatch_release(queue);
  }
}

void PolicyWatcher::AddStringPolicy(const std::string &name)
{
  policies.push_back(std::make_unique<StringPolicy>(name, productName));
}

void PolicyWatcher::AddNumberPolicy(const std::string &name)
{
  policies.push_back(std::make_unique<NumberPolicy>(name, productName));
}

void PolicyWatcher::AddBooleanPolicy(const std::string &name)
{
  policies.push_back(std::make_unique<BooleanPolicy>(name, productName));
}

void PolicyWatcher::AddUnionPolicy(const std::string &name, const std::vector<std::string> &types)
{
  policies.push_back(std::make_unique<UnionPolicy>(name, productName, types));
}

bool PolicyWatcher::Start(Napi::Env env)
{
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

  const std::string queueLabel = productName + ".policy-watcher";
  queue = dispatch_queue_create(queueLabel.c_str(), DISPATCH_QUEUE_SERIAL);
  CFStringRef path = CFSTR("/Library/Managed Preferences/");
  FSEventStreamContext context = {0, this, nullptr, nullptr, nullptr};
  CFArrayRef pathsToWatch = CFArrayCreate(nullptr, reinterpret_cast<const void **>(&path), 1, nullptr);
  stream = FSEventStreamCreate(nullptr,
                               &FSEventsCallback,
                               &context,
                               pathsToWatch,
                               kFSEventStreamEventIdSinceNow,
                               1.0,
                               kCFStreamEventNone);
  CFRelease(pathsToWatch);

  FSEventStreamSetDispatchQueue(stream, queue);
  FSEventStreamStart(stream);

  dispatch_async(queue, ^{
    RefreshPolicies(true);
  });

  napi_add_env_cleanup_hook(env, OnEnvCleanup, this);
  return true;
}

void PolicyWatcher::OnEnvCleanup(void *arg)
{
  auto *watcher = static_cast<PolicyWatcher *>(arg);
  watcher->Dispose();
}

void PolicyWatcher::Execute()
{
  RefreshPolicies(false);
}

void PolicyWatcher::RefreshPolicies(bool first)
{
  if (disposed.load(std::memory_order_acquire))
  {
    return;
  }

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
    if (queue)
    {
      dispatch_sync(queue, ^{
        if (stream)
        {
          FSEventStreamStop(stream);
          FSEventStreamInvalidate(stream);
          FSEventStreamRelease(stream);
          stream = nullptr;
        }
      });
    }
    if (static_cast<napi_threadsafe_function>(tsfn) != nullptr)
    {
      tsfn.Release();
    }
  }
}
