const EventEmitter = require('events');
const createWatcher = require('bindings')('vscode-policy');

class Watcher extends EventEmitter {

  constructor(productName, policies) {
    super();

    this._policies = new Map(Object.keys(policies).map(name => [name, undefined]));
    this._watcher = createWatcher(productName, policies, update => {
      for (const name of update) {
        this._policies.set(name, update[name]);
      }

      this.emit('change', this._policies);
    });
  }

  dispose() {
    console.log('bye');
    this._watcher.dispose();
  }
}

exports.Watcher = Watcher;

if (require.main === module) {
  // A
  // const watcher = new Watcher('CodeOSS', {
  //   UpdateMode: { type: 'string' },
  //   SCMInputFontSize: { type: 'number' },
  // });

  // watcher.on('change', policies => console.log(policies));

  // B
  createWatcher(
    'CodeOSS',
    {
      UpdateMode: { type: 'string' },
      SCMInputFontSize: { type: 'number' },
    },
    msg => console.log(msg)
  );
}
