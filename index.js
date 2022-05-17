/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

exports.createWatcher = require('bindings')('vscode-policy-watcher');

if (require.main === module) {
  (async () => {
    const watcher = exports.createWatcher(
      'CodeOSS',
      msg => console.log('event', msg)
    );

    console.log('register', await watcher.registerPolicyDefinitions({ UpdateMode: { type: 'string' } }));
    console.log('register', await watcher.registerPolicyDefinitions({ SCMInputFontSize: { type: 'number' } }));
    console.log('register', await watcher.registerPolicyDefinitions({ SCMInputFontSize: { type: 'number' } }));
    console.log('register', await watcher.registerPolicyDefinitions({ SCMInputFontSize: { type: 'number' } }));

    setTimeout(() => watcher.dispose(), 60000);
  })();

}
