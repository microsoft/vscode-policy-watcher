#include <chrono>
#include <napi.h>
#include <thread>
#include <windows.h>
#include <userenv.h>
#include <iostream>
#include <tuple>

using namespace Napi;

struct PolicyWatcher
{
  HANDLE hDispose;
  std::thread *thread;
};

void CallJs(
    Env env,
    Function callback,
    Reference<Value> *context,
    std::string *data)
{
  if (env != nullptr)
  {
    if (callback != nullptr)
    {
      callback.Call(context->Value(), {String::New(env, *data)});
    }
  }

  if (data != nullptr)
  {
    delete data;
  }
}

void PollForChanges(
    PolicyWatcher *watcher,
    TypedThreadSafeFunction<Reference<Value>, std::string, CallJs> tsfn)
{
  HANDLE hExit = CreateEvent(NULL, false, false, NULL);

  if (hExit == NULL)
  {
    goto done;
  }

  HANDLE hMachineEvent = CreateEvent(NULL, false, false, NULL);

  if (hMachineEvent == NULL)
  {
    goto done;
  }

  HANDLE hUserEvent = CreateEvent(NULL, false, false, NULL);

  if (hUserEvent == NULL)
  {
    goto done;
  }

  if (!RegisterGPNotification(hMachineEvent, TRUE))
  {
    goto done;
  }

  if (!RegisterGPNotification(hUserEvent, FALSE))
  {
    goto done;
  }

  HANDLE hHandles[4] = {
      hExit,
      watcher->hDispose,
      hMachineEvent,
      hUserEvent,
  };

  while (TRUE)
  {
    auto dwResult = WaitForMultipleObjects(4, hHandles, FALSE, INFINITE);

    if ((dwResult == WAIT_FAILED) || ((dwResult - WAIT_OBJECT_0) == 0))
    {
      break;
    }

    std::string *value;
    auto eventIndex = (dwResult - WAIT_OBJECT_0);

    if (eventIndex == 1)
    {
      break;
    }
    else if (eventIndex == 2)
    {
      value = new std::string("Machine notify event signaled.");
    }
    else
    {
      value = new std::string("User notify event signaled.");
    }

    napi_status status = tsfn.BlockingCall(value);

    if (status != napi_ok)
    {
      break;
    }
  }

done:
  UnregisterGPNotification(hMachineEvent);
  UnregisterGPNotification(hUserEvent);
  CloseHandle(hExit);
  CloseHandle(hMachineEvent);
  CloseHandle(hUserEvent);
  tsfn.Release();
}

void WaitForWatcher(
    Env,
    PolicyWatcher *watcher,
    Reference<Value> *ctx)
{
  watcher->thread->join();
  CloseHandle(watcher->hDispose);

  delete watcher->thread;
  delete watcher;
  delete ctx;
}

Value DisposeWatcher(const CallbackInfo &info)
{
  auto env = info.Env();
  auto watcher = (PolicyWatcher *)info.Data();

  if (watcher->hDispose != NULL)
  {
    SetEvent(watcher->hDispose);
    watcher->hDispose = NULL;
  }

  return env.Null();
}

Value CreateWatcher(const CallbackInfo &info)
{
  auto env = info.Env();

  if (info.Length() < 1)
  {
    throw TypeError::New(env, "Expected 1 argument");
  }
  else if (!info[0].IsFunction())
  {
    throw TypeError::New(env, "Expected first arg to be function");
  }

  auto hDispose = CreateEvent(NULL, false, false, NULL);

  if (hDispose == NULL)
  {
    throw TypeError::New(env, "Failed to create watcher dispose event");
  }

  auto context = new Reference<Value>(Persistent(info.This()));
  auto watcher = new PolicyWatcher();

  auto tsfn = TypedThreadSafeFunction<Reference<Value>, std::string, CallJs>::New(
      env,
      info[0].As<Function>(),
      "PolicyWatcher",
      0,
      1,
      context,
      WaitForWatcher,
      watcher);

  watcher->hDispose = hDispose;
  watcher->thread = new std::thread(PollForChanges, watcher, tsfn);

  auto result = Object::New(env);
  result.Set(String::New(env, "dispose"), Function::New(env, DisposeWatcher, "disposeWatcher", watcher));
  return result;
}

Object Init(Env env, Object exports)
{
  return Function::New(env, CreateWatcher, "createWatcher");
}

NODE_API_MODULE(vscodepolicy, Init)