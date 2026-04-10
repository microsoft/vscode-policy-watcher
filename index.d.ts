/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

interface Watcher {
  dispose(): void;
}

type StringPolicy = { type: "string" };
type NumberPolicy = { type: "number" };
type BooleanPolicy = { type: "boolean" };

export interface Policies {
  [policyName: string]: StringPolicy | NumberPolicy | BooleanPolicy;
}

export type PolicyUpdate<T extends Policies> = {
  [K in keyof T]:
    | undefined
    | (T[K] extends StringPolicy
        ? string
        : (T[K] extends BooleanPolicy
        ? boolean
        : T[K] extends NumberPolicy
        ? number
        : never));
};

export interface WatcherOptions {
  /**
   * Sets the registry path prefix before the product name.
   * Defaults to `"Microsoft"`, resulting in `Software\Policies\Microsoft\{productName}`.
   * Set to `""` for `Software\Policies\{productName}`.
   */
  registryPathPrefix?: string;
}

export function createWatcher<T extends Policies>(
  productName: string,
  policies: T,
  onDidChange: (update: PolicyUpdate<T>) => void,
  options?: WatcherOptions
): Watcher;
