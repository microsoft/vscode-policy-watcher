/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#ifndef PREFERENCES_POLICY_H
#define PREFERENCES_POLICY_H

#include <napi.h>
#include <CoreFoundation/CoreFoundation.h>
#include "../Policy.hh"

using namespace Napi;

template <typename T>
class PreferencesPolicy : public Policy
{
public:
    PreferencesPolicy(const std::string name, const std::string &productName)
        : Policy(name),
          appID(CFStringCreateWithCString(NULL, productName.c_str(), kCFStringEncodingUTF8)),
          key(CFStringCreateWithCString(NULL, name.c_str(), kCFStringEncodingUTF8))
    {
    }

    ~PreferencesPolicy()
    {
        CFRelease(appID);
        CFRelease(key);
    }

    PolicyRefreshResult refresh()
    {
        lastReadStatus = PolicyReadStatus::Missing;
        auto next = read();
        if (lastReadStatus == PolicyReadStatus::Error)
            return PolicyRefreshResult::Unchanged;

        const bool nextInvalid = lastReadStatus == PolicyReadStatus::Invalid;
        if (!next.has_value() || nextInvalid)
        {
            const bool changed = value.has_value() || invalid != nextInvalid;
            value.reset();
            invalid = nextInvalid;
            if (!changed)
                return nextInvalid ? PolicyRefreshResult::Unchanged : PolicyRefreshResult::NotSet;
            return PolicyRefreshResult::Removed;
        }

        if (value != next || invalid)
        {
            value = next;
            invalid = false;
            return PolicyRefreshResult::Updated;
        }

        return PolicyRefreshResult::Unchanged;
    }

    Value getValue(Env env) const
    {
        if (!value.has_value())
            return env.Undefined();
        return getJSValue(env, *value); // value.value() is only supported after macOS 10.13
    }

    bool isInvalid() const
    {
        return invalid;
    }

protected:
    std::optional<T> value;
    const CFStringRef appID;
    const CFStringRef key;
    virtual Value getJSValue(Env env, T value) const = 0;
    bool invalid = false;
    mutable PolicyReadStatus lastReadStatus = PolicyReadStatus::Missing;
    virtual std::optional<T> read() const = 0;

private:
};
#endif