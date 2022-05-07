const { start } = require('bindings')('vscode-policy');

start(function (clock) {
  console.log(clock);
}, 5);