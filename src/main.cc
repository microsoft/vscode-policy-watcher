#include <napi.h>
#include <windows.h>
#include <userenv.h>
#include <thread>
#include <vector>
#include <list>
#include <optional>

using namespace Napi;

class Policy
{
public:
  virtual bool refresh() = 0;
  virtual Value getValue(Env env) = 0;
  const std::string name;

  Policy(const std::string name)
      : name(name) {}
};

template <typename T>
class RegistryPolicy : public Policy
{
public:
  RegistryPolicy(const std::string name, const std::string &productName, const DWORD regType)
      : Policy(name),
        registryKey("Software\\Policies\\Microsoft\\" + productName),
        regType(regType) {}

  bool refresh()
  {
    auto machine = read(HKEY_LOCAL_MACHINE);

    if (machine.has_value())
    {
      if (value != machine)
      {
        value = machine;
        return true;
      }

      return false;
    }

    auto user = read(HKEY_CURRENT_USER);

    if (value != user)
    {
      value = user;
      return true;
    }

    return false;
  }

  Value getValue(Env env)
  {
    if (!value.has_value())
      return env.Undefined();

    return getJSValue(env, value.value());
  }

protected:
  virtual T parseRegistryValue(LPBYTE buffer, DWORD bufferSize) = 0;
  virtual Value getJSValue(Env env, T value) = 0;

private:
  const std::string registryKey;
  const DWORD regType;
  std::optional<T> value;

  std::optional<T> read(HKEY root)
  {
    HKEY hKey;

    if (ERROR_SUCCESS != RegOpenKeyEx(root, registryKey.c_str(), 0, KEY_READ, &hKey))
      return std::nullopt;

    BYTE buffer[1024];
    DWORD bufferSize = sizeof(buffer);
    DWORD type;

    auto readResult = RegQueryValueEx(hKey, name.c_str(), 0, &type, buffer, &bufferSize);
    RegCloseKey(hKey);

    if (ERROR_SUCCESS != readResult || type != regType)
      return std::nullopt;

    return std::optional<T>{parseRegistryValue(buffer, bufferSize)};
  }
};

class StringPolicy : public RegistryPolicy<std::string>
{
public:
  StringPolicy(const std::string name, const std::string &productName)
      : RegistryPolicy(name, productName, REG_SZ) {}

protected:
  std::string parseRegistryValue(LPBYTE buffer, DWORD bufferSize)
  {
    return std::string(reinterpret_cast<char *>(buffer), bufferSize - 1);
  }

  Value getJSValue(Env env, std::string value)
  {
    return String::New(env, value);
  }
};

class NumberPolicy : public RegistryPolicy<long long>
{
public:
  NumberPolicy(const std::string name, const std::string &productName)
      : RegistryPolicy(name, productName, REG_QWORD) {}

protected:
  long long parseRegistryValue(LPBYTE buffer, DWORD bufferSize)
  {
    return *reinterpret_cast<long long *>(buffer);
  }

  Value getJSValue(Env env, long long value)
  {
    return Number::New(env, value);
  }
};

void CallJs(Env env, Function callback, Reference<Value> *context, std::list<std::shared_ptr<Policy>> *data);

struct PolicyWatcher
{
  std::vector<std::shared_ptr<Policy>> policies;
  HANDLE hDispose;
  std::thread *thread;

  PolicyWatcher(std::vector<std::shared_ptr<Policy>> _policies, HANDLE _hDispose)
      : policies(_policies),
        hDispose(_hDispose),
        thread(NULL) {}

  void poll(
      HANDLE *handles,
      size_t handleSize,
      TypedThreadSafeFunction<Reference<Value>, std::list<std::shared_ptr<Policy>>, CallJs> &tsfn)
  {
    bool first = true;

    while (TRUE)
    {
      std::list<std::shared_ptr<Policy>> *updatedPolicies = nullptr;

      for (auto &policy : policies)
      {
        if (policy->refresh())
        {
          if (updatedPolicies == nullptr)
            updatedPolicies = new std::list<std::shared_ptr<Policy>>();

          updatedPolicies->push_back(policy);
        }
      }

      if (first || (updatedPolicies != nullptr && updatedPolicies->size() > 0))
        if (napi_ok != tsfn.BlockingCall(updatedPolicies))
          break;

      first = false;

      auto dwResult = WaitForMultipleObjects(handleSize, handles, FALSE, INFINITE);

      if (dwResult == WAIT_FAILED || (dwResult - WAIT_OBJECT_0) == 0 || (dwResult - WAIT_OBJECT_0) == 1 /* someone called dispose() */)
        break;
    }
  }
};

void CallJs(
    Env env,
    Function callback,
    Reference<Value> *context,
    std::list<std::shared_ptr<Policy>> *updatedPolicies)
{
  if (env != nullptr)
  {
    if (callback != nullptr)
    {
      auto result = Object::New(env);

      if (updatedPolicies != nullptr)
        for (auto const &policy : *updatedPolicies)
          result.Set(policy->name, policy->getValue(env));

      callback.Call(context->Value(), {result});
    }
  }

  if (updatedPolicies != nullptr)
    delete updatedPolicies;
}

void PollForChanges(
    PolicyWatcher *watcher,
    TypedThreadSafeFunction<Reference<Value>, std::list<std::shared_ptr<Policy>>, CallJs> tsfn)
{
  HANDLE hExit = CreateEvent(NULL, false, false, NULL);

  if (hExit == NULL)
    goto done;

  HANDLE hMachineEvent = CreateEvent(NULL, false, false, NULL);

  if (hMachineEvent == NULL)
    goto done;

  HANDLE hUserEvent = CreateEvent(NULL, false, false, NULL);

  if (hUserEvent == NULL)
    goto done;
  else if (!RegisterGPNotification(hMachineEvent, TRUE))
    goto done;
  else if (!RegisterGPNotification(hUserEvent, FALSE))
    goto done;

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
    throw TypeError::New(env, "Expected 3 arguments");
  else if (!info[0].IsString())
    throw TypeError::New(env, "Expected first arg to be string");
  else if (!info[1].IsArray())
    throw TypeError::New(env, "Expected second arg to be array");
  else if (!info[2].IsFunction())
    throw TypeError::New(env, "Expected third arg to be function");

  auto productName = info[0].As<String>();
  auto rawPolicies = info[1].As<Array>();
  auto policies = std::vector<std::shared_ptr<Policy>>();
  policies.reserve(rawPolicies.Length());

  for (auto const &item : rawPolicies)
  {
    auto value = static_cast<Value>(item.second);

    if (!value.IsObject())
      throw TypeError::New(env, "Expected policy to be object");

    auto rawPolicy = value.As<Object>();
    auto rawPolicyName = rawPolicy.Get("name");

    if (!rawPolicyName.IsString())
      throw TypeError::New(env, "Expected policy name to be string");

    auto rawPolicyType = rawPolicy.Get("type");

    if (!rawPolicyType.IsString())
      throw TypeError::New(env, "Expected policy type to be string");

    auto policyType = std::string(rawPolicyType.As<String>());

    if (policyType == "string")
      policies.push_back(std::make_shared<StringPolicy>(rawPolicyName.As<String>(), productName));
    else if (policyType == "number")
      policies.push_back(std::make_shared<NumberPolicy>(rawPolicyName.As<String>(), productName));
    else
      throw TypeError::New(env, "Unknown policy type '" + policyType + "'");
  }

  auto hDispose = CreateEvent(NULL, false, false, NULL);

  if (hDispose == NULL)
    throw TypeError::New(env, "Failed to create watcher dispose event");

  auto context = new Reference<Value>(Persistent(info.This()));
  auto watcher = new PolicyWatcher(policies, hDispose);

  auto tsfn = TypedThreadSafeFunction<Reference<Value>, std::list<std::shared_ptr<Policy>>, CallJs>::New(
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