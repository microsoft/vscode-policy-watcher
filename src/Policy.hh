/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#ifndef POLICY_H
#define POLICY_H

#include <napi.h>

using namespace Napi;

class Policy
{
public:
  virtual ~Policy() {}
  virtual bool refresh() = 0;
  virtual Value getValue(Env env) const = 0;
  const std::string name;

  Policy(const std::string name)
      : name(name) {}
};

#endif