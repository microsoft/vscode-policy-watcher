{
    "targets": [
        {
            "target_name": "vscode-policy-watcher",
            "sources": [
                "src/main.cc"
            ],
            "include_dirs": [
                "<!(node -p \"require('node-addon-api').include_dir\")"
            ],
            'cflags!': ['-fno-exceptions'],
            'cflags_cc!': ['-fno-exceptions'],
            'conditions': [
                ['OS=="mac"', {
                    "sources": [
                        "src/macos/PolicyWatcher.cc",
                    ],
                    "defines": [
                        "MACOS",
                    ],
                    "xcode_settings": {
                        "GCC_ENABLE_CPP_EXCEPTIONS": "YES"
                    }
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
                        "src/windows/NumberPolicy.cc"
                    ],
                    "defines": [
                        "WINDOWS",
                        "_HAS_EXCEPTIONS=1"
                    ],
                    "libraries": [
                        "userenv.lib"
                    ],
                    "msvs_configuration_attributes": {
                        "SpectreMitigation": "Spectre"
                    },
                    "msvs_settings": {
                        "VCCLCompilerTool": {
                            "ExceptionHandling": 1,
                            'AdditionalOptions': [
                                '/guard:cf',
                                '-std:c++17',
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
