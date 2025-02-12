/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#ifndef PREFERENCES_POLICY_H
#define PREFERENCES_POLICY_H

#include <napi.h>
#include <CoreFoundation/CoreFoundation.h>

#include <iostream>
#include "../Policy.hh"

using namespace Napi;

template <typename T>
class PreferencesPolicy : public Policy
{
public:
    PreferencesPolicy(const std::string name, const std::string &productName)
        : Policy(name)
    //  registryKey("Software\\Policies\\Microsoft\\" + productName),
    //   supportedTypes(types)
    {
    }

    bool refresh()
    {
        // Check for changes in MDM-managed preferences
        CFStringRef appID = CFSTR("com.visualstudio.code.oss"); // Replace with the provided app ID
        CFStringRef key = CFSTR("AllowedExtensions");           // Replace with provided preference key
        CFPropertyListRef value = CFPreferencesCopyAppValue(key, appID);

        if (value != NULL)
        {
            // Convert the value to a human-readable string
            if (CFGetTypeID(value) == CFStringGetTypeID())
            {
                const char *cValueString = CFStringGetCStringPtr((CFStringRef)value, kCFStringEncodingUTF8);
                char buffer[256];
                if (cValueString)
                {
                    strncpy(buffer, cValueString, sizeof(buffer));
                    buffer[sizeof(buffer) - 1] = '\0'; // Ensure null-termination
                    std::cout << "The value for the specified key is: " << buffer << std::endl;
                }
                else
                {
                    if (CFStringGetCString((CFStringRef)value, buffer, sizeof(buffer), kCFStringEncodingUTF8))
                    {
                        std::cout << "The value for the specified key is: " << buffer << std::endl;
                    }
                    else
                    {
                        std::cerr << "Failed to convert the value to a string." << std::endl;
                    }
                }
            }
            else
            {
                std::cerr << "The value is not a CFString." << std::endl;
            }
            CFRelease(value);
        }
        else
        {
            std::cerr << "Failed to get the value for the specified key." << std::endl;
            return false;
        }
        return true;
    }

    Value getValue(Env env) const
    {
        if (!value.has_value())
            return env.Undefined();
        return getJSValue(env, value.value());
    }

protected:
    virtual Value getJSValue(Env env, T value) const = 0;

private:
    std::optional<T> value;
};

#endif