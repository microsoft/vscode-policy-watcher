/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#include "PolicyWatcher.hh"

using namespace Napi;

PolicyWatcher::PolicyWatcher(const std::string productName, Function &okCallback)
    : AsyncProgressQueueWorker(okCallback),
      productName(productName)
{
}

PolicyWatcher::~PolicyWatcher()
{
  UnregisterGPNotification(handles[2]);
  UnregisterGPNotification(handles[3]);
  CloseHandle(handles[0]);
  CloseHandle(handles[1]);
  CloseHandle(handles[2]);
  CloseHandle(handles[3]);
  CloseHandle(handles[4]);
}

void PolicyWatcher::OnExecute(Napi::Env env)
{
  if ((handles[0] = CreateEvent(NULL, false, false, NULL)) == NULL)
    return SetError("Failed to create exit event");
  if ((handles[1] = CreateEvent(NULL, false, false, NULL)) == NULL)
    return SetError("Failed to create dispose event");
  if ((handles[2] = CreateEvent(NULL, false, false, NULL)) == NULL)
    return SetError("Failed to create machine GP event");
  if ((handles[3] = CreateEvent(NULL, false, false, NULL)) == NULL)
    return SetError("Failed to create user GP event");
  if ((handles[4] = CreateEvent(NULL, false, false, NULL)) == NULL)
    return SetError("Failed to create policy registration event");
  if (!RegisterGPNotification(handles[2], TRUE))
    return SetError("Failed to register machine GP event");
  if (!RegisterGPNotification(handles[3], FALSE))
    return SetError("Failed to register user GP event");

  AsyncProgressQueueWorker::OnExecute(env);
}

void PolicyWatcher::Execute(const ExecutionProgress &progress)
{
  while (TRUE)
  {
    std::vector<const Policy *> updatedPolicies;

    mutex.lock();

    for (auto &policy : newPolicies)
      if (policy->hasValue())
        updatedPolicies.push_back(policy);

    newPolicies.clear();

    for (auto &pair : policies)
      if (pair.second->refresh())
        updatedPolicies.push_back(pair.second.get());

    mutex.unlock();

    if (updatedPolicies.size() > 0)
      progress.Send(&updatedPolicies[0], updatedPolicies.size());

    auto dwResult = WaitForMultipleObjects(5, handles, FALSE, INFINITE);

    if (dwResult == WAIT_FAILED || (dwResult - WAIT_OBJECT_0) == 0 || (dwResult - WAIT_OBJECT_0) == 1 /* someone called dispose() */)
      break;
  }
}

void PolicyWatcher::OnOK() {}
void PolicyWatcher::OnError() {}

void PolicyWatcher::OnProgress(const Policy *const *policies, size_t count)
{
  auto env = Env();
  auto result = Object::New(env);

  for (size_t i = 0; i < count; i++)
    result.Set(policies[i]->name, policies[i]->getValue(env));

  Callback().Call(Receiver().Value(), {result});
}

std::vector<const Policy *> PolicyWatcher::RegisterPolicyDefinitions(std::vector<std::unique_ptr<Policy>> &_policies)
{
  std::vector<const Policy *> result;

  mutex.lock();

  for (auto &policy : _policies)
  {
    if (!policies[policy->name])
    {
      policy->refresh();

      if (policy->hasValue())
      {
        result.push_back(policy.get());
        newPolicies.push_back(policy.get());
      }

      policies[policy->name] = std::move(policy);
    }
  }

  mutex.unlock();
  return result;
}

void PolicyWatcher::Update()
{
  SetEvent(handles[4]);
}

void PolicyWatcher::Dispose()
{
  SetEvent(handles[1]);
}
