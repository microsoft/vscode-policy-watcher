/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

interface Watcher {
  dispose(): void;
}

type StringPolicy = { type: "string" };
type NumberPolicy = { type: "number" };

export interface Policies {
  [policyName: string]: StringPolicy | NumberPolicy;
}

export type PolicyUpdate<T extends Policies> = {
  [K in keyof T]:
    | undefined
    | (T[K] extends StringPolicy
        ? string
        : T[K] extends NumberPolicy
        ? number
        : never);
};

export function createWatcher<T extends Policies>(
  productName: string,
  policies: T,
  onDidChange: (update: PolicyUpdate<T>) => void
): Watcher;
