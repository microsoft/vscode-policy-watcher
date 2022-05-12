const createWatcher = require('bindings')('vscode-policy');

const watcher = createWatcher(
  'CodeOSS',
  [{ name: 'UpdateMode', type: 'string' }],
  msg => console.log(msg)
);

setTimeout(() => console.log(watcher.dispose()), 1000);