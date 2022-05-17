/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#ifndef POLICY_WATCHER_H
#define POLICY_WATCHER_H

#include <napi.h>
#include <windows.h>
#include <userenv.h>
#include <vector>
#include "Policy.hh"

using namespace Napi;

class PolicyWatcher : public AsyncProgressWorker<const Policy *>
{
public:
  PolicyWatcher(Function &okCallback, std::vector<std::unique_ptr<Policy>> _policies);
  ~PolicyWatcher();

  void OnExecute(Napi::Env env);
  void Execute(const ExecutionProgress &progress);
  void OnOK();
  void OnError();
  void OnProgress(const Policy *const *policies, size_t count);
  void Dispose();

private:
  std::vector<std::unique_ptr<Policy>> policies;
  HANDLE handles[4];
};

#endif
