/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#include "PolicyRegisterer.hh"

using namespace Napi;

PolicyRegisterer::PolicyRegisterer(Napi::Env &env, PolicyWatcher *watcher, std::vector<std::unique_ptr<Policy>> newPolicies)
    : AsyncWorker(env),
      watcher(watcher),
      newPolicies(std::move(newPolicies)),
      deferred(Promise::Deferred::New(env))
{
}

PolicyRegisterer::~PolicyRegisterer()
{
}

void PolicyRegisterer::Execute()
{
  newPoliciesWithValue = watcher->RegisterPolicyDefinitions(newPolicies);
}

void PolicyRegisterer::OnOK()
{
  auto env = Env();
  auto result = Object::New(env);

  for (auto &policy : newPoliciesWithValue)
    result.Set(policy->name, policy->getValue(env));

  deferred.Resolve(result);

  // Wake up the policy watcher, so it knows there are new policy definitions
  watcher->Update();
}

void PolicyRegisterer::OnError(Error const &error)
{
  deferred.Reject(error.Value());
}

Promise PolicyRegisterer::Promise()
{
  return deferred.Promise();
}
