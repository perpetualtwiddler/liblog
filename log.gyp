{
    'targets': [
      {
        'target_name': 'liblog',
        'type': 'static_library',
        'defines': [
        ],
        'include_dirs': [
          '.',
        ],
        'sources': [
          'log.c',
          'util.c',
        ],
        'conditions': [
          ['OS=="linux"', {
            'cflags': [
              '-g',
              '-Wall',
              '-std=c99',
              '-pthread',
            ],
            'defines': [
              '_POSIX_C_SOURCE=200112L',
              '_XOPEN_SOURCE=500L',
            ],
          }],
          ['OS=="win"', {
            'defines': [
              '_WIN32_WINNT=0x501',
              'NDEBUG',
              '_CRT_SECURE_NO_WARNINGS',
            ],
          }]
        ],
      },
      {
        'target_name': 'logtest',
        'type': 'executable',
        'dependencies': [
          'liblog',
        ],
        'defines': [
        ],
        'include_dirs': [
          '.',
        ],
        'sources': [
          'logtest.c',
        ],
        'conditions': [
          ['OS=="linux"', {
            'cflags': [
              '-g',
              '-Wall',
              '-std=c99',
              '-pthread',
            ],
            'defines': [
              '_POSIX_C_SOURCE=200112L',
              '_XOPEN_SOURCE=500L',
            ],
            'ldflags': [
              '-pthread',
            ]
          }],
          ['OS=="win"', {
            'defines': [
              '_WIN32_WINNT=0x501',
              'NDEBUG',
              '_CRT_SECURE_NO_WARNINGS',
            ],
          }]
        ],
      },
    ],
  }
