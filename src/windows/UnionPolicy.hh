/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#ifndef UNION_POLICY_H
#define UNION_POLICY_H

#include <optional>
#include <string>
#include <variant>
#include <vector>
#include <windows.h>
#include "../Policy.hh"

using UnionPolicyValue = std::variant<bool, double, std::string>;

class UnionPolicy : public Policy
{
public:
  UnionPolicy(const std::string name, const std::string &productName, const std::string &registryPath, const std::vector<std::string> &types);
  PolicyRefreshResult refresh();
  Napi::Value getValue(Napi::Env env) const;

private:
  std::optional<UnionPolicyValue> read(HKEY root) const;
  const std::string registryKey;
  bool acceptsBoolean = false;
  bool acceptsNumber = false;
  bool acceptsString = false;
  std::optional<UnionPolicyValue> value;
};

#endif
