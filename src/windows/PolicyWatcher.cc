/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#include <userenv.h>

#include "../PolicyWatcher.hh"
#include "StringPolicy.hh"
#include "NumberPolicy.hh"

using namespace Napi;

PolicyWatcher::PolicyWatcher(std::string productName, const Function &okCallback)
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
}

void PolicyWatcher::AddStringPolicy(const std::string name)
{
  policies.push_back(std::make_unique<StringPolicy>(name, productName));
}

void PolicyWatcher::AddNumberPolicy(const std::string name)
{
  policies.push_back(std::make_unique<NumberPolicy>(name, productName));
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
  if (!RegisterGPNotification(handles[2], TRUE))
    return SetError("Failed to register machine GP event");
  if (!RegisterGPNotification(handles[3], FALSE))
    return SetError("Failed to register user GP event");

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

    auto dwResult = WaitForMultipleObjects(4, handles, FALSE, INFINITE);

    if (dwResult == WAIT_FAILED || (dwResult - WAIT_OBJECT_0) == 0 || (dwResult - WAIT_OBJECT_0) == 1 /* someone called dispose() */)
      break;
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
  SetEvent(handles[1]);
}
