/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#ifndef POLICY_REGISTERER_H
#define POLICY_REGISTERER_H

#include <napi.h>
#include <vector>
#include "Policy.hh"
#include "PolicyWatcher.hh"

using namespace Napi;

class PolicyRegisterer : public AsyncWorker
{
public:
  PolicyRegisterer(Napi::Env &env, PolicyWatcher *watcher, std::vector<std::unique_ptr<Policy>> newPolicies);
  ~PolicyRegisterer();

  void Execute();
  void OnOK();
  void OnError(Error const &error);

  Promise Promise();

private:
  PolicyWatcher *watcher;
  std::vector<std::unique_ptr<Policy>> newPolicies;
  std::vector<const Policy *> newPoliciesWithValue;
  Promise::Deferred deferred;
};

#endif
