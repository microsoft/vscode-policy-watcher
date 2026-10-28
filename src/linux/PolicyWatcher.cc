/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#include "../PolicyWatcher.hh"
#include <utility>

PolicyWatcher::PolicyWatcher(const std::string &productName, const Napi::Function &okCallback, const std::string &registryPath)
    : env(okCallback.Env()),
      productName(productName),
      registryPath(registryPath)
{
}

PolicyWatcher::~PolicyWatcher() = default;

void PolicyWatcher::AddStringPolicy(const std::string & /*name*/) {}
void PolicyWatcher::AddNumberPolicy(const std::string & /*name*/) {}
void PolicyWatcher::AddBooleanPolicy(const std::string & /*name*/) {}
void PolicyWatcher::AddUnionPolicy(const std::string & /*name*/, const std::vector<std::string> & /*types*/) {}
bool PolicyWatcher::Start(Napi::Env /*env*/) { return true; }
void PolicyWatcher::Execute() {}
void PolicyWatcher::RefreshPolicies(bool /*first*/) {}
void PolicyWatcher::CallJS(Napi::Env /*env*/, Napi::Function /*callback*/, PolicyWatcher * /*context*/, void * /*data*/) {}
void PolicyWatcher::OnEnvCleanup(void * /*arg*/) {}
void PolicyWatcher::Dispose() {}
