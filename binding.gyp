{
  'targets': [
    {
      'target_name': 'ble-adapter-native',
      'cflags!': [ '-fno-exceptions' ],
      'cflags_cc!': [ '-fno-exceptions' ],
      'xcode_settings': { 'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
        'CLANG_CXX_LIBRARY': 'libc++',
        'MACOSX_DEPLOYMENT_TARGET': '10.7',
      },
      'msvs_settings': {
        'VCCLCompilerTool': { 'ExceptionHandling': 1 },
      },
      "defines": [
        "NAPI_VERSION=<(napi_build_version)"
      ],
      'conditions': [
        ['OS=="win"', {
          'sources': [ 'src/ble_adapter.cc' ],
          'include_dirs': [
            '<!(node -p "require(\'node-addon-api\').include_dir")'
          ],
          'cflags!': [ '-fno-exceptions' ],
          'cflags_cc!': [ '-fno-exceptions' ],
          'msvs_settings': {
            'VCCLCompilerTool': { 'ExceptionHandling': 1 },
          }
        }]
      ]
    }
  ]
}
