/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#include <userenv.h>

#include "../PolicyWatcher.hh"

using namespace Napi;

PolicyWatcher::PolicyWatcher(std::string productName, Function &okCallback)
    : AsyncProgressQueueWorker(okCallback),
      productName(productName)
{
}

PolicyWatcher::~PolicyWatcher()
{
}

void PolicyWatcher::AddStringPolicy(const std::string name) {}
void PolicyWatcher::AddNumberPolicy(const std::string name) {}

void PolicyWatcher::OnExecute(Napi::Env env)
{
  return SetError("Not supported");
}

void PolicyWatcher::Execute(const ExecutionProgress &progress) {}
void PolicyWatcher::OnOK() {}
void PolicyWatcher::OnError() {}
void PolicyWatcher::OnProgress(const Policy *const *policies, size_t count) {}
void PolicyWatcher::Dispose() {}
