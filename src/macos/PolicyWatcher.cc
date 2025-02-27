/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#include "../PolicyWatcher.hh"
#include "StringPolicy.hh"
#include "NumberPolicy.hh"
#include <thread>

using namespace Napi;

static void fsevents_callback(ConstFSEventStreamRef streamRef,
                            void *clientCallBackInfo,
                            size_t numEvents,
                            void *eventPaths,
                            const FSEventStreamEventFlags eventFlags[],
                            const FSEventStreamEventId eventIds[])
{
    dispatch_semaphore_t *sem = (dispatch_semaphore_t *)clientCallBackInfo;
    dispatch_semaphore_signal(*sem);
}

PolicyWatcher::PolicyWatcher(std::string productName, const Function &okCallback)
    : AsyncProgressQueueWorker(okCallback),
      productName(productName),
      stream(nullptr),
      sem(nullptr),
      disposed(false)
{
}

PolicyWatcher::~PolicyWatcher()
{
    if (stream) {
        FSEventStreamStop(stream);
        FSEventStreamInvalidate(stream);
        FSEventStreamRelease(stream);
    }
    if (sem) {
        dispatch_release(sem);
    }
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
  AsyncProgressQueueWorker::OnExecute(env);
}

void PolicyWatcher::Execute(const ExecutionProgress &progress)
{
    std::vector<const Policy *> updatedPolicies;
    bool first = true;

    // Watch for changes
    CFStringRef path = CFSTR("/Library/Managed Preferences/");
    sem = dispatch_semaphore_create(0);
    FSEventStreamContext context = {0, &sem, NULL, NULL, NULL};
    stream = FSEventStreamCreate(NULL,
                               &fsevents_callback,
                               &context,
                               CFArrayCreate(NULL, (const void **)&path, 1, NULL),
                               kFSEventStreamEventIdSinceNow,
                               1.0,
                               kCFStreamEventNone);

    dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
    FSEventStreamSetDispatchQueue(stream, queue);
    FSEventStreamStart(stream);

    while (!disposed)
    {
        updatedPolicies.clear();
        bool update = false;
        for (auto &policy : policies)
        {            
            switch(policy->refresh())
            {
                case PolicyRefreshResult::Updated:
                    updatedPolicies.push_back(policy.get());
                    update = true;
                    break;
                case PolicyRefreshResult::Unchanged:
                    updatedPolicies.push_back(policy.get());
                    break;
                case PolicyRefreshResult::Removed:
                    update = true;
                    break;
                case PolicyRefreshResult::NotSet:
                    break;
            }
        }

        if (first || update)
            progress.Send(&updatedPolicies[0], updatedPolicies.size());

        first = false;
        dispatch_semaphore_wait(sem, DISPATCH_TIME_FOREVER);
    }
}

void PolicyWatcher::OnProgress(const Policy *const *policies, size_t count) 
{
  HandleScope scope(Env());
  auto result = Object::New(Env());

  if (count == 0)
  {
    Callback().Call(Receiver().Value(), {result});
    return;
  }

  for (size_t i = 0; i < count; i++)
    result.Set(policies[i]->name, policies[i]->getValue(Env()));

  Callback().Call(Receiver().Value(), {result});
}

void PolicyWatcher::Dispose() 
{
    disposed = true;
    if (sem) {
        dispatch_semaphore_signal(sem);
    }
}
