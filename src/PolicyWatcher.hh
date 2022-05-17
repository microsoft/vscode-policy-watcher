/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#ifndef POLICY_WATCHER_H
#define POLICY_WATCHER_H

#include <napi.h>
#include <windows.h>
#include <userenv.h>
#include <unordered_map>
#include <mutex>
#include "Policy.hh"

using namespace Napi;

class PolicyWatcher : public AsyncProgressQueueWorker<const Policy *>
{
public:
  PolicyWatcher(const std::string productName, Function &okCallback);
  ~PolicyWatcher();

  void OnExecute(Napi::Env env);
  void Execute(const ExecutionProgress &progress);
  void OnOK();
  void OnError();
  void OnProgress(const Policy *const *policies, size_t count);
  std::vector<const Policy *> RegisterPolicyDefinitions(std::vector<std::unique_ptr<Policy>> &newPolicies);
  void Update();
  void Dispose();

  const std::string productName;

private:
  std::mutex mutex;
  std::vector<const Policy *> newPolicies;
  std::unordered_map<std::string, std::unique_ptr<Policy>> policies;
  HANDLE handles[5];
};

#endif
