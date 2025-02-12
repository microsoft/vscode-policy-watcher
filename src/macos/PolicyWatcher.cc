/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#include "../PolicyWatcher.hh"
#include "StringPolicy.hh"
#include <iostream>
#include <chrono>
#include <thread>
using namespace Napi;

PolicyWatcher::PolicyWatcher(std::string productName, const Function &okCallback)
    : AsyncProgressQueueWorker(okCallback),
      productName(productName)
{
}

PolicyWatcher::~PolicyWatcher()
{
}

void PolicyWatcher::AddStringPolicy(const std::string name)
{
  std::cout << "Adding string policy " << name << std::endl;
  policies.push_back(std::make_unique<StringPolicy>(name, productName));
  std::cout << "Added string policy " << name << std::endl;
}
void PolicyWatcher::AddNumberPolicy(const std::string name)
{
  throw TypeError::New(Env(), "Not implemented");
}

void PolicyWatcher::OnExecute(Napi::Env env)
{
  AsyncProgressQueueWorker::OnExecute(env);
}

void PolicyWatcher::Execute(const ExecutionProgress &progress)
{
  bool first = true;

  while (TRUE)
  {
    std::vector<const Policy *> updatedPolicies;

    for (auto &policy : policies)
    {
      if (policy->refresh())
      {
        updatedPolicies.push_back(policy.get());
      }
    }

    if (first || updatedPolicies.size() > 0)    
      progress.Send(&updatedPolicies[0], updatedPolicies.size());

    first = false;

    // WAIT with CFNotificationCenterAddObserver

    // TODO: Temporary
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }
}

void PolicyWatcher::OnProgress(const Policy *const *policies, size_t count) 
{
  HandleScope scope(Env());
  auto result = Object::New(Env());

  for (size_t i = 0; i < count; i++)
    result.Set(policies[i]->name, policies[i]->getValue(Env()));

  Callback().Call(Receiver().Value(), {result});
}

void PolicyWatcher::Dispose() 
{
  // TODO
}
