{
    "targets": [
        {
            "target_name": "vscode-policy-watcher",
            "sources": [
                "src/main.cc",
                "src/StringPolicy.cc",
                "src/NumberPolicy.cc",
                "src/PolicyWatcher.cc"
            ],
            "include_dirs": [
                "<!(node -p \"require('node-addon-api').include_dir\")"
            ],
            'cflags!': ['-fno-exceptions'],
            'cflags_cc!': ['-fno-exceptions'],
            'conditions': [
                ["OS=='win'", {
                    "defines": [
                        "_HAS_EXCEPTIONS=1"
                    ],
                    "libraries": [
                        "userenv.lib"
                    ],
                    "msvs_settings": {
                        "VCCLCompilerTool": {
                            "ExceptionHandling": 1,
                            'AdditionalOptions': [
                                '/W3',
                                '/Qspectre',
                                '/guard:cf',
                                '-std:c++17'
                            ]
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
