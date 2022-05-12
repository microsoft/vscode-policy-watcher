#include <chrono>
#include <napi.h>
#include <thread>
#include <windows.h>
#include <userenv.h>
#include <iostream>
#include <vector>

using namespace Napi;

struct Policy
{
  std::string name;
  std::string type;
};

struct PolicyWatcher
{
  std::string productName;
  std::vector<Policy> policies;
  HANDLE hDispose;
  std::thread *thread;

  PolicyWatcher(std::string _productName, std::vector<Policy> _policies, HANDLE _hDispose)
      : productName(_productName),
        policies(_policies),
        hDispose(_hDispose),
        thread(NULL)
  {
  }
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

  // auto policyValues = std::unordered_map<std::string,

  // RegQueryValueEx()

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

  if (info.Length() < 3)
  {
    throw TypeError::New(env, "Expected 3 arguments");
  }
  else if (!info[0].IsString())
  {
    throw TypeError::New(env, "Expected first arg to be string");
  }
  else if (!info[1].IsArray())
  {
    throw TypeError::New(env, "Expected second arg to be array");
  }
  else if (!info[2].IsFunction())
  {
    throw TypeError::New(env, "Expected third arg to be function");
  }

  auto productName = info[0].As<String>();
  auto rawPolicies = info[1].As<Array>();
  auto policies = std::vector<Policy>();
  policies.reserve(rawPolicies.Length());

  for (auto const &item : rawPolicies)
  {
    auto value = static_cast<Value>(item.second);

    if (!value.IsObject())
    {
      throw TypeError::New(env, "Expected policy to be object");
    }

    auto rawPolicy = value.As<Object>();
    auto rawPolicyName = rawPolicy.Get("name");

    if (!rawPolicyName.IsString())
    {
      throw TypeError::New(env, "Expected policy name to be string");
    }

    auto rawPolicyType = rawPolicy.Get("type");

    if (!rawPolicyType.IsString())
    {
      throw TypeError::New(env, "Expected policy type to be string");
    }

    policies.push_back(Policy{
        std::string(rawPolicyName.As<String>()),
        std::string(rawPolicyType.As<String>())});
  }

  auto hDispose = CreateEvent(NULL, false, false, NULL);

  if (hDispose == NULL)
  {
    throw TypeError::New(env, "Failed to create watcher dispose event");
  }

  auto context = new Reference<Value>(Persistent(info.This()));
  auto watcher = new PolicyWatcher(productName, policies, hDispose);

  auto tsfn = TypedThreadSafeFunction<Reference<Value>, std::string, CallJs>::New(
      env,
      info[2].As<Function>(),
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