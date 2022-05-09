#include <chrono>
#include <napi.h>
#include <thread>
#include <windows.h>
#include <userenv.h>
#include <iostream>

using namespace Napi;

void CallJs(Napi::Env env, Function callback, Reference<Value> *context, std::string *data);

std::thread nativeThread;
TypedThreadSafeFunction<Reference<Value>, std::string, CallJs> tsfn;

void CallJs(Napi::Env env, Function callback, Reference<Value> *context,
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

void PollForChanges()
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

  HANDLE hHandles[3];
  DWORD dwResult;

  // This code assumes that hMachineEvent and hUserEvent
  // have been initialized.

  HANDLE hExit = CreateEvent(NULL, false, false, NULL);
  HANDLE hMachineEvent = CreateEvent(NULL, false, false, NULL);
  HANDLE hUserEvent = CreateEvent(NULL, false, false, NULL);

  RegisterGPNotification(hMachineEvent, TRUE);
  RegisterGPNotification(hUserEvent, FALSE);

  hHandles[0] = hExit;
  hHandles[1] = hMachineEvent;
  hHandles[2] = hUserEvent;

  while (TRUE)
  {
    dwResult = WaitForMultipleObjects(3, hHandles, FALSE, INFINITE);

    if ((dwResult == WAIT_FAILED) || ((dwResult - WAIT_OBJECT_0) == 0))
    {
      break;
    }

    std::string *value;

    if ((dwResult - WAIT_OBJECT_0) == 1)
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

void StopPolling(Napi::Env, void *, Reference<Value> *ctx)
{
  nativeThread.join();
  delete ctx;
}

Value Start(const CallbackInfo &info)
{
  Napi::Env env = info.Env();

  if (info.Length() < 2)
  {
    throw TypeError::New(env, "Expected two arguments");
  }
  else if (!info[0].IsFunction())
  {
    throw TypeError::New(env, "Expected first arg to be function");
  }
  else if (!info[1].IsNumber())
  {
    throw TypeError::New(env, "Expected second arg to be number");
  }

  // auto count = info[1].As<Number>().Int32Value();
  auto context = new Reference<Value>(Persistent(info.This()));

  tsfn = TypedThreadSafeFunction<Reference<Value>, std::string, CallJs>::New(
      env,
      info[0].As<Function>(),
      "PolicyWatcher",
      0,
      1,
      context,
      StopPolling);

  nativeThread = std::thread(PollForChanges);
  return Boolean::New(env, true);
}

Napi::Object Init(Napi::Env env, Object exports)
{
  exports.Set("start", Function::New(env, Start));
  return exports;
}

NODE_API_MODULE(clock, Init)