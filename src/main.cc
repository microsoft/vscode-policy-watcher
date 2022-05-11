#include <chrono>
#include <napi.h>
#include <thread>
#include <windows.h>
#include <userenv.h>
#include <iostream>
#include <tuple>

using namespace Napi;

void CallJs(Env env, Function callback, Reference<Value> *context,
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

void PollForChanges(std::tuple<HANDLE, std::thread *> *watcherContext, TypedThreadSafeFunction<Reference<Value>, std::string, CallJs> tsfn)
{
  // for (int i = 0; i < count; i++) {
  //   // Create new data
  //   int *value = new int(clock());

  //   // Perform a blocking call
  // napi_status status = tsfn.BlockingCall(value);
  // if (status != napi_ok) {
  //   // Handle error
  //   break;
  // }

  // std::this_thread::sleep_for(std::chrono::seconds(1));
  // }

  HANDLE hHandles[4];
  DWORD dwResult;

  // This code assumes that hMachineEvent and hUserEvent
  // have been initialized.

  HANDLE hExit = CreateEvent(NULL, false, false, NULL);
  HANDLE hMachineEvent = CreateEvent(NULL, false, false, NULL);
  HANDLE hUserEvent = CreateEvent(NULL, false, false, NULL);

  RegisterGPNotification(hMachineEvent, TRUE);
  RegisterGPNotification(hUserEvent, FALSE);

  hHandles[0] = hExit;
  hHandles[1] = std::get<0>(*watcherContext);
  hHandles[2] = hMachineEvent;
  hHandles[3] = hUserEvent;

  while (TRUE)
  {
    dwResult = WaitForMultipleObjects(4, hHandles, FALSE, INFINITE);

    if ((dwResult == WAIT_FAILED) || ((dwResult - WAIT_OBJECT_0) == 0))
    {
      break;
    }

    std::string *value;
    auto eventIndex = (dwResult - WAIT_OBJECT_0);

    std::cout << "got event " << eventIndex << std::endl;

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

  UnregisterGPNotification(hMachineEvent);
  UnregisterGPNotification(hUserEvent);
  CloseHandle(hExit);
  CloseHandle(hMachineEvent);
  CloseHandle(hUserEvent);

  // Release the thread-safe function
  tsfn.Release();
}

void StopPolling(Env, std::tuple<HANDLE, std::thread *> *watcherContext, Reference<Value> *ctx)
{
  std::get<1>(*watcherContext)->join();
  CloseHandle(std::get<0>(*watcherContext));
  delete std::get<1>(*watcherContext);
  delete watcherContext;
  delete ctx;
}

Value DisposeWatcher(const CallbackInfo &info)
{
  auto env = info.Env();
  auto watcherContext = (std::tuple<HANDLE, std::thread *> *)info.Data();

  std::cout << "want to dispose" << std::endl;
  SetEvent(std::get<0>(*watcherContext));

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

  auto context = new Reference<Value>(Persistent(info.This()));
  auto watcherContext = new std::tuple<HANDLE, std::thread *>();

  auto tsfn = TypedThreadSafeFunction<Reference<Value>, std::string, CallJs>::New(
      env,
      info[0].As<Function>(),
      "PolicyWatcher",
      0,
      1,
      context,
      StopPolling,
      watcherContext);

  std::get<0>(*watcherContext) = CreateEvent(NULL, false, false, NULL);
  std::get<1>(*watcherContext) = new std::thread(PollForChanges, watcherContext, tsfn);

  auto result = Object::New(env);
  result.Set(String::New(env, "dispose"), Function::New(env, DisposeWatcher, "disposeWatcher", watcherContext));
  return result;
}

Object Init(Env env, Object exports)
{
  return Function::New(env, CreateWatcher, "createWatcher");
}

NODE_API_MODULE(clock, Init)