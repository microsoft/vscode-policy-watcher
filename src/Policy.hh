/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#ifndef POLICY_H
#define POLICY_H

#include <napi.h>
#include <string>

enum class PolicyRefreshResult {
  Updated,
  Unchanged,
  Removed,
  NotSet
};

class Policy
{
public:
  virtual ~Policy() = default;
  virtual PolicyRefreshResult refresh() = 0;
  virtual Napi::Value getValue(Napi::Env env) const = 0;
  const std::string name;

  explicit Policy(const std::string &name)
      : name(name) {}
};

#endif