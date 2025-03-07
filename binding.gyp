{
    "targets": [
        {
            "target_name": "printer_pdf_electron_node",
            "cflags!": [ "-fno-exceptions" ],
            "cflags_cc!": [ "-fno-exceptions" ],
            "cflags_cc": ["-std=c++17"],
            "binding_name": "printer_pdf_electron_node",
            "xcode_settings": { 
                "GCC_ENABLE_CPP_EXCEPTIONS": "YES",
                "CLANG_CXX_LIBRARY": "libc++",
                "MACOSX_DEPLOYMENT_TARGET": "10.7",
                "CLANG_CXX_LANGUAGE_STANDARD": "c++17"
            },
            "msvs_settings": {
                "VCCLCompilerTool": { 
                    "ExceptionHandling": 1,
                    "AdditionalOptions": ["/std:c++17"]
                }
            },
            "sources": [
                "src/pdfium.cc",
                "src/pdfium_option.cc"
            ],
            "include_dirs": [
                "<!@(node -p \"require('node-addon-api').include\")",
                "./src/include"
            ],
            "dependencies": [
                "<!(node -p \"require('node-addon-api').gyp\")"
            ],
            "defines": [ "NAPI_DISABLE_CPP_EXCEPTIONS" ],
            "conditions": [
                ["OS=='win'", {
                    "sources": [
                        "src/printer_win.cc",
                        "src/pdfium_imp.cc"
                    ],
                    "conditions": [
                        ["target_arch=='x64'", {
                            "libraries": ["<(module_root_dir)/pdfium/lib/win/x64/lib/pdfium.dll.lib"],
                            "copies": [{
                                "destination": "<(PRODUCT_DIR)",
                                "files": ["<(module_root_dir)/pdfium/lib/win/x64/bin/pdfium.dll"]
                            }]
                        }],
                        ["target_arch=='ia32'", {
                            "libraries": ["<(module_root_dir)/pdfium/lib/win/x86/lib/pdfium.dll.lib"],
                            "copies": [{
                                "destination": "<(PRODUCT_DIR)",
                                "files": ["<(module_root_dir)/pdfium/lib/win/x86/bin/pdfium.dll"]
                            }]
                        }],
                        ["target_arch=='arm64'", {
                            "libraries": ["<(module_root_dir)/pdfium/lib/win/arm64/lib/pdfium.dll.lib"],
                            "copies": [{
                                "destination": "<(PRODUCT_DIR)",
                                "files": ["<(module_root_dir)/pdfium/lib/win/arm64/bin/pdfium.dll"]
                            }]
                        }]
                    ]
                }],
                ["OS=='linux'", {
                    "sources": [
                        "src/printer_linux.cc"
                    ],
                    "libraries": [
                        "-lcups",
                        "-Wl,-rpath,'$$ORIGIN'"
                    ],
                    "conditions": [
                        ["target_arch=='x64'", {
                            "libraries": ["<(module_root_dir)/pdfium/lib/linux/x64/lib/libpdfium.so"],
                            "copies": [{
                                "destination": "<(PRODUCT_DIR)",
                                "files": ["<(module_root_dir)/pdfium/lib/linux/x64/lib/libpdfium.so"]
                            }]
                        }],
                        ["target_arch=='ia32'", {
                            "libraries": ["<(module_root_dir)/pdfium/lib/linux/x86/lib/libpdfium.so"],
                            "copies": [{
                                "destination": "<(PRODUCT_DIR)",
                                "files": ["<(module_root_dir)/pdfium/lib/linux/x86/lib/libpdfium.so"]
                            }]
                        }],
                        ["target_arch=='arm64'", {
                            "libraries": ["<(module_root_dir)/pdfium/lib/linux/arm64/lib/libpdfium.so"],
                            "copies": [{
                                "destination": "<(PRODUCT_DIR)",
                                "files": ["<(module_root_dir)/pdfium/lib/linux/arm64/lib/libpdfium.so"]
                            }]
                        }]
                    ]
                }]
            ]
        }
    ]
}
