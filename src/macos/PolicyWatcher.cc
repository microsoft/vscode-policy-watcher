/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#include "../PolicyWatcher.hh"
#include "StringPolicy.hh"
#include <iostream>
#include <chrono>
#include <thread>
#include <CoreServices/CoreServices.h>
using namespace Napi;

// Add FSEvents callback
static void fsevents_callback(ConstFSEventStreamRef streamRef,
                            void *clientCallBackInfo,
                            size_t numEvents,
                            void *eventPaths,
                            const FSEventStreamEventFlags eventFlags[],
                            const FSEventStreamEventId eventIds[])
{
    // Signal that changes occurred
    dispatch_semaphore_t *sem = (dispatch_semaphore_t *)clientCallBackInfo;
    dispatch_semaphore_signal(*sem);
}

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
  
  // Setup FSEvents monitoring
  CFStringRef path = CFSTR("/Library/Managed Preferences/");
  CFArrayRef pathsToWatch = CFArrayCreate(NULL, (const void **)&path, 1, NULL);
  
  // Create semaphore for blocking
  dispatch_semaphore_t sem = dispatch_semaphore_create(0);
  
  FSEventStreamContext context = {0, &sem, NULL, NULL, NULL};
  FSEventStreamRef stream = FSEventStreamCreate(NULL,
                                                &fsevents_callback,
                                                &context,
                                                pathsToWatch,
                                                kFSEventStreamEventIdSinceNow,
                                                1.0,
                                                kFSEventStreamCreateFlagFileEvents);

  dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
  FSEventStreamSetDispatchQueue(stream, queue);
  FSEventStreamStart(stream);

  while (true)
  {
    std::vector<const Policy *> updatedPolicies;

    for (auto &policy : policies)
    {
      if (first || policy->refresh())
      {
        updatedPolicies.push_back(policy.get());
      }
    }

    if (first || updatedPolicies.size() > 0)    
      progress.Send(&updatedPolicies[0], updatedPolicies.size());

    first = false;

    // Block until changes occur
    dispatch_semaphore_wait(sem, DISPATCH_TIME_FOREVER);
  }

  // Cleanup (though we never reach here in practice)
  FSEventStreamStop(stream);
  FSEventStreamInvalidate(stream);
  FSEventStreamRelease(stream);
  CFRelease(pathsToWatch);
  dispatch_release(sem);
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
