/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#ifndef POLICY_WATCHER_H
#define POLICY_WATCHER_H

#include <napi.h>
#include <array>
#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <vector>
#include "Policy.hh"

#ifdef WINDOWS
#include <windows.h>
#endif

#ifdef MACOS
#define Boolean CFBoolean
#include <CoreServices/CoreServices.h>
#endif

class PolicyWatcher
{
public:
  PolicyWatcher(const std::string &productName, const Napi::Function &okCallback, const std::string &registryPath = "");
  ~PolicyWatcher();

  PolicyWatcher(const PolicyWatcher &) = delete;
  PolicyWatcher &operator=(const PolicyWatcher &) = delete;

  void AddStringPolicy(const std::string &name);
  void AddNumberPolicy(const std::string &name);
  void AddBooleanPolicy(const std::string &name);
  void AddUnionPolicy(const std::string &name, const std::vector<std::string> &types);

  bool Start(Napi::Env env);
  void Execute();
  void Dispose();

private:
  void RefreshPolicies(bool first);
  static void CallJS(Napi::Env env, Napi::Function callback, PolicyWatcher *context, void *data);
  static void OnEnvCleanup(void *arg);

protected:
  Napi::Env env;
  std::string productName;
  std::string registryPath;
  Napi::FunctionReference callbackRef;
  std::vector<std::unique_ptr<Policy>> policies;
  std::vector<const Policy *> updatedPolicies;
  std::mutex mutex;
  using TSFN = Napi::TypedThreadSafeFunction<PolicyWatcher, void, &PolicyWatcher::CallJS>;
  TSFN tsfn;
  std::atomic<bool> disposed{false};

#ifdef WINDOWS
  std::thread workerThread;
  std::array<HANDLE, 4> handles{};
#endif

#ifdef MACOS
  FSEventStreamRef stream = nullptr;
  dispatch_queue_t queue = nullptr;
#endif

};

#endif
