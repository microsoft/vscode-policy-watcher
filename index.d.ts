/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

type StringPolicy = { type: "string" };
type NumberPolicy = { type: "number" };

interface PolicyDefinitions {
  [policyName: string]: StringPolicy | NumberPolicy;
}

type PolicyValues = {
  [policyName: string]: string | number | undefined;
};

type PolicyUpdate = {
  [policyName: string]: string | number | undefined;
};

interface Watcher {
  registerPolicyDefinitions(policies: PolicyDefinitions): Promise<PolicyValues>;
  dispose(): void;
}

export function createWatcher(
  productName: string,
  onDidChange: (update: PolicyUpdate) => void
): Watcher;
