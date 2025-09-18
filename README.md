# @vscode/policy-watcher

[![CI](https://github.com/microsoft/vscode-policy-watcher/actions/workflows/ci.yml/badge.svg)](https://github.com/microsoft/vscode-policy-watcher/actions/workflows/ci.yml)
[![Version](https://img.shields.io/npm/v/@vscode/policy-watcher.svg)](https://npmjs.org/package/@vscode/policy-watcher)

Example usage:

```js
const createWatcher = require("@vscode/policy-watcher");

createWatcher(
  "CodeOSS",
  {
    UpdateMode: { type: "string" },
    SCMInputFontSize: { type: "number" },
  },
  (update) => console.log(update)
);
```

## Installation

The package includes prebuilt binaries for the following platforms:
- Windows (x64, x86)
- macOS (x64, arm64)
- Linux (x64, arm64)

When installing via npm, the appropriate binary will be downloaded automatically. If a prebuilt binary is not available for your platform, the module will be compiled from source.

```bash
npm install @vscode/policy-watcher
```

## Development

### Prerequisites

- Node.js 18.x or 20.x
- Python 3.11+
- Platform-specific build tools:
  - Windows: Visual Studio Build Tools
  - macOS: Xcode Command Line Tools
  - Linux: GCC toolchain

### Building from source

```bash
# Install dependencies
npm install

# Build the native module
npm run rebuild

# Run tests
npm test
```

### Creating prebuilt binaries

```bash
# Build prebuilt binaries for the current platform
npm run prebuild

# Or use the build script
node .github/scripts/build.js
```

## Contributing

This project welcomes contributions and suggestions. Most contributions require you to agree to a
Contributor License Agreement (CLA) declaring that you have the right to, and actually do, grant us
the rights to use your contribution. For details, visit https://cla.opensource.microsoft.com.

When you submit a pull request, a CLA bot will automatically determine whether you need to provide
a CLA and decorate the PR appropriately (e.g., status check, comment). Simply follow the instructions
provided by the bot. You will only need to do this once across all repos using our CLA.

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).
For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or
contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.

## Trademarks

This project may contain trademarks or logos for projects, products, or services. Authorized use of Microsoft
trademarks or logos is subject to and must follow
[Microsoft's Trademark & Brand Guidelines](https://www.microsoft.com/en-us/legal/intellectualproperty/trademarks/usage/general).
Use of Microsoft trademarks or logos in modified versions of this project must not cause confusion or imply Microsoft sponsorship.
Any use of third-party trademarks or logos are subject to those third-party's policies.
