/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

'use strict';

const assert = require('node:assert/strict');
const { execFileSync } = require('node:child_process');
const { randomUUID } = require('node:crypto');
const test = require('node:test');

const supportedPlatform = process.platform === 'darwin' || process.platform === 'win32';
const policyName = 'InvalidValuePolicy';

function run(command, args) {
	execFileSync(command, args, { stdio: 'pipe' });
}

function createFixture() {
	const id = randomUUID();

	if (process.platform === 'darwin') {
		const domain = `com.microsoft.vscode-policy-watcher.invalid-values.${id}`;
		// Keep the isolated domain present while leaving the policy itself absent.
		run('defaults', ['write', domain, 'FixtureMarker', '-string', 'present']);
		return {
			productName: domain,
			setString: value => run('defaults', ['write', domain, policyName, '-string', value]),
			setNumber: value => run('defaults', ['write', domain, policyName, '-int', String(value)]),
			setObject: () => run('defaults', ['write', domain, policyName, '-dict', 'nested', 'value']),
			setArray: () => run('defaults', ['write', domain, policyName, '-array', 'first', 'second']),
			dispose: () => run('defaults', ['delete', domain]),
		};
	}

	const registryPath = `Software\\vscode-policy-watcher-tests\\invalid-values-${id}`;
	run('reg.exe', ['add', `HKCU\\${registryPath}`, '/f']);
	return {
		productName: `InvalidValues${id}`,
		options: { registryPath },
		setString: value => run('reg.exe', ['add', `HKCU\\${registryPath}`, '/v', policyName, '/t', 'REG_SZ', '/d', value, '/f']),
		setNumber: value => run('reg.exe', ['add', `HKCU\\${registryPath}`, '/v', policyName, '/t', 'REG_DWORD', '/d', String(value), '/f']),
		dispose: () => run('reg.exe', ['delete', `HKCU\\${registryPath}`, '/f']),
	};
}

function readInitialSnapshot(fixture) {
	return new Promise((resolve, reject) => {
		let watcher;
		let settled = false;
		const finish = (error, value) => {
			if (settled) {
				return;
			}
			settled = true;
			clearTimeout(timeout);
			try {
				watcher?.dispose();
			} catch (disposeError) {
				if (!error) {
					error = disposeError;
				}
			}
			error ? reject(error) : resolve(value);
		};
		const timeout = setTimeout(
			() => finish(new Error('Timed out waiting for the watcher initial callback')),
			5_000,
		);

		try {
			const { createWatcher } = require('../index');
			watcher = createWatcher(
				fixture.productName,
				{ [policyName]: { type: 'string' } },
				(values, metadata) => finish(undefined, { values, metadata }),
				fixture.options,
			);
		} catch (error) {
			finish(error);
		}
	});
}

async function withFixture(configure, verify) {
	const fixture = createFixture();
	let testError;
	try {
		await configure(fixture);
		await verify(await readInitialSnapshot(fixture));
	} catch (error) {
		testError = error;
		throw error;
	} finally {
		try {
			fixture.dispose();
		} catch (cleanupError) {
			if (testError) {
				testError.cleanupError = cleanupError;
			} else {
				throw cleanupError;
			}
		}
	}
}

function assertSnapshot(snapshot, value, invalidPolicies) {
	assert.equal(snapshot.values[policyName], value);
	assert.deepEqual(snapshot.metadata, { invalidPolicies });
}

test('string policy reports an absent isolated policy without invalid metadata', { skip: !supportedPlatform }, async () => {
	await withFixture(
		async () => {},
		snapshot => assertSnapshot(snapshot, undefined, []),
	);
});

test('string policy reports a valid isolated string without invalid metadata', { skip: !supportedPlatform }, async () => {
	await withFixture(
		fixture => fixture.setString('managed-file-path'),
		snapshot => assertSnapshot(snapshot, 'managed-file-path', []),
	);
});

test('string policy reports an isolated numeric native value as invalid metadata', { skip: !supportedPlatform }, async () => {
	await withFixture(
		fixture => fixture.setNumber(42),
		snapshot => assertSnapshot(snapshot, undefined, [policyName]),
	);
});

test('string policy reports isolated object and array native values as invalid metadata on macOS', { skip: process.platform !== 'darwin' }, async t => {
	for (const [name, configure] of [
		['object', fixture => fixture.setObject()],
		['array', fixture => fixture.setArray()],
	]) {
		await t.test(name, async () => {
			await withFixture(
				configure,
				snapshot => assertSnapshot(snapshot, undefined, [policyName]),
			);
		});
	}
});
