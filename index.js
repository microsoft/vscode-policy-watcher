const createWatcher = require('bindings')('vscode-policy');

const watcher = createWatcher(
  'CodeOSS',
  [
    { name: 'UpdateMode', type: 'string' },
    { name: 'SCMInputFontSize', type: 'number' }
  ],
  msg => console.log(msg)
);

// setTimeout(() => watcher.dispose(), 10000);