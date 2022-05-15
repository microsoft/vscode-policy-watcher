#include <chrono>
#include <napi.h>
#include <thread>
#include <windows.h>
#include <userenv.h>
#include <iostream>
#include <vector>
#include <list>
#include <optional>

using namespace Napi;

template <typename T>
class Policy
{
public:
  Policy(const std::string &productName, const std::string name)
      : name(name),
        registryKey("Software\\Policies\\Microsoft\\" + productName)
  {
  }

  bool refresh()
  {
    auto machine = read(HKEY_LOCAL_MACHINE);

    if (_value != machine)
    {
      _value = machine;
      return true;
    }

    auto user = read(HKEY_CURRENT_USER);

    if (_value != user)
    {
      _value = user;
      return true;
    }

    return false;
  }

  Napi::Value getValue(Napi::Env env)
  {
    if (!_value.has_value())
    {
      return env.Undefined();
    }

    return Napi::String::New(env, _value.value());
  }

  const std::string name;

protected:
  const std::string registryKey;
  std::optional<T> _value;

  virtual std::optional<T> read(HKEY root) = 0;
};

class StringPolicy : public Policy<std::string>
{
  using Policy<std::string>::Policy;

protected:
  std::optional<std::string> read(HKEY root)
  {
    HKEY hKey;

    if (ERROR_SUCCESS != RegOpenKeyEx(root, registryKey.c_str(), 0, KEY_READ, &hKey))
    {
      return std::nullopt;
    }

    char buffer[1024];
    DWORD bufferSize = sizeof(buffer);
    DWORD type;

    auto readResult = RegQueryValueEx(hKey, name.c_str(), 0, &type, (LPBYTE)buffer, &bufferSize);
    RegCloseKey(hKey);

    if (ERROR_SUCCESS != readResult || type != REG_SZ)
    {
      return std::nullopt;
    }

    return std::optional<std::string>{buffer};
  }
};

void CallJs(Env env, Function callback, Reference<Value> *context, std::list<StringPolicy *> *data);

struct PolicyWatcher
{
  std::vector<StringPolicy> policies;
  HANDLE hDispose;
  std::thread *thread;

  PolicyWatcher(std::vector<StringPolicy> _policies, HANDLE _hDispose)
      : policies(_policies),
        hDispose(_hDispose),
        thread(NULL)
  {
  }

  void poll(
      HANDLE *handles,
      size_t handleSize,
      TypedThreadSafeFunction<Reference<Value>, std::list<StringPolicy *>, CallJs> &tsfn)
  {
    bool first = true;

    while (TRUE)
    {
      std::list<StringPolicy *> *updatedPolicies = nullptr;

      for (auto &policy : policies)
      {
        if (policy.refresh())
        {
          if (updatedPolicies == nullptr)
          {
            updatedPolicies = new std::list<StringPolicy *>();
          }

          updatedPolicies->push_back(&policy);
        }
      }

      if (first || (updatedPolicies != nullptr && updatedPolicies->size() > 0))
      {
        if (napi_ok != tsfn.BlockingCall(updatedPolicies))
        {
          break;
        }
      }

      first = false;

      auto dwResult = WaitForMultipleObjects(handleSize, handles, FALSE, INFINITE);

      if (dwResult == WAIT_FAILED || (dwResult - WAIT_OBJECT_0) == 0 || (dwResult - WAIT_OBJECT_0) == 1 /* someone called dispose() */)
      {
        break;
      }
    }
  }
};

void CallJs(
    Env env,
    Function callback,
    Reference<Value> *context,
    std::list<StringPolicy *> *updatedPolicies)
{
  if (env != nullptr)
  {
    if (callback != nullptr)
    {
      auto result = Object::New(env);

      if (updatedPolicies != nullptr)
      {
        for (auto const &policy : *updatedPolicies)
        {
          result.Set(policy->name, policy->getValue(env));
        }
      }

      callback.Call(context->Value(), {result});
    }
  }

  if (updatedPolicies != nullptr)
  {
    delete updatedPolicies;
  }
}

void PollForChanges(
    PolicyWatcher *watcher,
    TypedThreadSafeFunction<Reference<Value>, std::list<StringPolicy *>, CallJs> tsfn)
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

  HANDLE handles[4] = {
      hExit,
      watcher->hDispose,
      hMachineEvent,
      hUserEvent,
  };

  watcher->poll(handles, 4, tsfn);

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
    Reference<Value> *context)
{
  watcher->thread->join();
  CloseHandle(watcher->hDispose);

  delete watcher->thread;
  delete watcher;
  delete context;
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
  auto policies = std::vector<StringPolicy>();
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

    auto policy = StringPolicy(productName, rawPolicyName.As<String>());
    policies.push_back(policy);
  }

  auto hDispose = CreateEvent(NULL, false, false, NULL);

  if (hDispose == NULL)
  {
    throw TypeError::New(env, "Failed to create watcher dispose event");
  }

  auto context = new Reference<Value>(Persistent(info.This()));
  auto watcher = new PolicyWatcher(policies, hDispose);

  auto tsfn = TypedThreadSafeFunction<Reference<Value>, std::list<StringPolicy *>, CallJs>::New(
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