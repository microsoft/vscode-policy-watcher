/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#ifndef POLICY_WATCHER_H
#define POLICY_WATCHER_H

#include <napi.h>
#include <vector>
#include "Policy.hh"

#ifdef WINDOWS
#include <windows.h>
#endif

#ifdef MACOS
#define Boolean CFBoolean
#include <CoreServices/CoreServices.h>
#endif

using namespace Napi;

class PolicyWatcher : public AsyncProgressQueueWorker<const Policy *>
{
public:
  PolicyWatcher(std::string productName, const Function &okCallback);
  ~PolicyWatcher();

  void AddStringPolicy(const std::string name);
  void AddNumberPolicy(const std::string name);

  void OnExecute(Napi::Env env);
  void Execute(const ExecutionProgress &progress);
  void OnProgress(const Policy *const *policies, size_t count);
  void Dispose();

protected:
  std::string productName;
  std::vector<std::unique_ptr<Policy>> policies;

#ifdef WINDOWS
  HANDLE handles[4];
#endif

#ifdef MACOS
  FSEventStreamRef stream;
  dispatch_semaphore_t sem;
  bool disposed;
#endif

};

#endif
