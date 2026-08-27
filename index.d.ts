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
type PolicyType = "string" | "number" | "boolean";
type UnionPolicy = { type: readonly [PolicyType, ...PolicyType[]] };

export interface Policies {
  [policyName: string]: StringPolicy | NumberPolicy | BooleanPolicy | UnionPolicy;
}

export interface WatcherOptions {
  /** Windows only: custom registry key path. Defaults to `Software\Policies\Microsoft\<productName>`. */
  registryPath?: string;
}

export type PolicyUpdate<T extends Policies> = {
  [K in keyof T]:
    | undefined
      | (T[K]["type"] extends readonly PolicyType[]
          ? PolicyTypeValue<T[K]["type"][number]>
          : PolicyTypeValue<T[K]["type"]>);
};

type PolicyTypeValue<T extends PolicyType> =
    T extends "string" ? string :
    T extends "boolean" ? boolean :
    T extends "number" ? number :
    never;

export function createWatcher<T extends Policies>(
  productName: string,
  policies: T,
  onDidChange: (update: PolicyUpdate<T>) => void,
  options?: WatcherOptions
): Watcher;
