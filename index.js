const createWatcher = require('bindings')('vscode-policy');

const watcher = createWatcher(msg => console.log(msg));
setTimeout(() => console.log(watcher.dispose()), 10000);