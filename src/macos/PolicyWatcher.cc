/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#include "../PolicyWatcher.hh"
#include "StringPolicy.hh"
#include <iostream>
using namespace Napi;

PolicyWatcher::PolicyWatcher(std::string productName, const Function &okCallback)
    : AsyncProgressQueueWorker(okCallback),
      productName(productName)
{
}

PolicyWatcher::~PolicyWatcher()
{
}

void PolicyWatcher::AddStringPolicy(const std::string name) {
  std::cout << "Adding string policy " << name << std::endl;
  policies.push_back(std::make_unique<StringPolicy>(name, productName));
  std::cout << "Added string policy " << name << std::endl;

}
void PolicyWatcher::AddNumberPolicy(const std::string name) {
  throw TypeError::New(Env(), "Not implemented");
}

void PolicyWatcher::OnExecute(Napi::Env env) {
    AsyncProgressQueueWorker::OnExecute(env);
}

void PolicyWatcher::Execute(const ExecutionProgress &progress) {
  
  while (TRUE) {
    std::vector<const Policy *> updatedPolicies;

    for (auto &policy : policies) {
      if (policy->refresh()) {
        updatedPolicies.push_back(policy.get());
      }
    }

    if (!updatedPolicies.empty()) {
      std::cout << "Updated " << updatedPolicies.size() << " policie(s)" << std::endl;
      progress.Send(&updatedPolicies[0], updatedPolicies.size());
    }

    // WAIT with CFNotificationCenterAddObserver

  }
}
void PolicyWatcher::OnProgress(const Policy *const *policies, size_t count) {}

void PolicyWatcher::Dispose() {}
