{
    "targets": [
        {
            "target_name": "vscode-policy-watcher",
            "sources": [
                "src/main.cc"
            ],
            "dependencies": [
                "<!(node -p \"require('node-addon-api').targets\"):node_addon_api_except"
            ],
            "include_dirs": [
                "<!(node -p \"require('node-addon-api').include_dir\")"
            ],
            "defines": [ "NODE_API_SWALLOW_UNTHROWABLE_EXCEPTIONS" ],
            'conditions': [
                ['OS=="mac"', {
                    "sources": [
                        "src/macos/PolicyWatcher.cc",
                        "src/macos/StringPolicy.cc",
                        "src/macos/NumberPolicy.cc",
                        "src/macos/BooleanPolicy.cc"
                    ],
                    "defines": [
                        "MACOS",
                    ]
                }],
                ['OS=="mac" and target_arch=="arm64"', {
                    "xcode_settings": {
                        "ARCHS": ["arm64"]
                    }
                }],
                ['OS=="linux"', {
                    "sources": [
                        "src/linux/PolicyWatcher.cc",
                    ],
                    "defines": [
                        "LINUX",
                    ]
                }],
                ["OS=='win'", {
                    "sources": [
                        "src/windows/PolicyWatcher.cc",
                        "src/windows/StringPolicy.cc",
                        "src/windows/NumberPolicy.cc",
                        "src/windows/BooleanPolicy.cc"
                    ],
                    "defines": [
                        "WINDOWS"
                    ],
                    "libraries": [
                        "userenv.lib"
                    ],
                    "msvs_configuration_attributes": {
                        "SpectreMitigation": "Spectre"
                    },
                    "msvs_settings": {
                        "VCCLCompilerTool": {
                            'AdditionalOptions': [
                                '/guard:cf',
                                '/we4244',
                                '/we4267',
                                '/ZH:SHA_256'
                            ],
                        },
                        'VCLinkerTool': {
                            'AdditionalOptions': [
                                '/guard:cf'
                            ]
                        }
                    },
                }],
            ],
        }
    ]
}
