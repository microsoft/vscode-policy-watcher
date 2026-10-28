/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

import { execFile } from 'node:child_process';
import { promisify } from 'node:util';
import { describe, expect, it } from 'vitest';
import { createWatcher } from '../index.js';

const execFileAsync = promisify(execFile);
const isSupportedPlatform = process.platform === 'darwin' || process.platform === 'win32';

describe('policy-watcher', () => {
	it.runIf(!isSupportedPlatform)('throws on unsupported platforms', () => {
		expect(() => createWatcher('com.visualstudio.code.oss', {}, () => {})).toThrow(/Unsupported platform/);
	});

	describe.runIf(isSupportedPlatform)('supported platforms', () => {
		it('validates arguments and policy definitions', () => {
			expect(() => createWatcher()).toThrow(/Expected 3 arguments/);
			expect(() => createWatcher(123, {}, () => {})).toThrow(/Expected first arg to be string/);
			expect(() => createWatcher('com.visualstudio.code.oss', null, () => {})).toThrow(/Expected second arg to be object/);
			expect(() => createWatcher('com.visualstudio.code.oss', {}, 'not-a-fn')).toThrow(/Expected third arg to be function/);
			expect(() => createWatcher('com.visualstudio.code.oss', {}, () => {}, 'not-an-obj')).toThrow(/Expected fourth arg to be object/);
			expect(() => createWatcher('com.visualstudio.code.oss', {}, () => {}, { registryPath: 123 })).toThrow(/Expected options\.registryPath to be string/);
			expect(() => createWatcher('com.visualstudio.code.oss', { PolicyA: 'not-an-obj' }, () => {})).toThrow(/Expected policy to be object/);
			expect(() => createWatcher('com.visualstudio.code.oss', { PolicyA: { type: [] } }, () => {})).toThrow(/Expected policy type array to be non-empty/);
			expect(() => createWatcher('com.visualstudio.code.oss', { PolicyA: { type: [123] } }, () => {})).toThrow(/Expected policy type array entries to be strings/);
			expect(() => createWatcher('com.visualstudio.code.oss', { PolicyA: { type: 123 } }, () => {})).toThrow(/Expected policy type to be a string or non-empty string array/);
			expect(() => createWatcher('com.visualstudio.code.oss', { PolicyA: { type: 'invalid' } }, () => {})).toThrow(/Unknown policy type 'invalid'/);
		});

		it('invokes initial callback and supports multiple dispose calls', async () => {
			let watcher;
			const update = await new Promise(resolve => {
				watcher = createWatcher(
					'com.visualstudio.code.oss',
					{
						UpdateMode: { type: 'string' },
						SCMInputFontSize: { type: 'number' },
						DisableFeedback: { type: 'boolean' },
						UnionSetting: { type: ['string', 'number', 'boolean'] },
					},
					resolve,
				);
			});

			expect(update).toBeTypeOf('object');
			watcher.dispose();
			watcher.dispose();
		});

		it('does not block libuv threadpool workers when multiple watchers are active', async () => {
			const script = `
				const fs = require('node:fs/promises');
				const { createWatcher } = require('./index.js');

				const w1 = createWatcher('com.microsoft.VSCode', { UpdateMode: { type: 'string' } }, () => {});
				const w2 = createWatcher('com.github.copilot', { Allow: { type: 'boolean' } }, async () => {
					const pkg = await fs.readFile('./package.json', 'utf8');
					if (!pkg.includes('@vscode/policy-watcher')) {
						process.exit(2);
					}
					w1.dispose();
					w2.dispose();
					console.log('ok');
				});
			`;

			const { stdout } = await execFileAsync(process.execPath, ['-e', script], {
				cwd: process.cwd(),
				env: { ...process.env, UV_THREADPOOL_SIZE: '1' },
				timeout: 5000,
			});

			expect(stdout.trim()).toBe('ok');
		});

		it('cleans up active watchers on process exit without explicit dispose', async () => {
			const script = `
				const { createWatcher } = require('./index.js');
				createWatcher('com.visualstudio.code.oss', { UpdateMode: { type: 'string' } }, () => {
					process.exit(0);
				});
			`;

			const { stderr } = await execFileAsync(process.execPath, ['-e', script], {
				cwd: process.cwd(),
				timeout: 5000,
			});

			expect(stderr).toBe('');
		});
	});
});
