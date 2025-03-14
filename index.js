/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

exports.createWatcher = require('bindings')('vscode-policy-watcher');

if (require.main === module) {
  const platform = process.platform;
  exports.createWatcher(
    platform === 'darwin'
      ? 'com.visualstudio.code.oss'  // MacOS
      : 'CodeOSS',                   // Windows
    {
      UpdateMode: { type: 'string' },
      SCMInputFontSize: { type: 'number' },
      DisableFeedback: { type: 'boolean' },
    },
    msg => console.log(msg)
  );
}
