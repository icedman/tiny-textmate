project('text-edit', 'c', 'cpp',
  version : '0.1',
  default_options : ['warning_level=3', 'cpp_std=c++11']
)

textmate_files = [
  'textmate.c',
  'parser.c',
  'theme.c',
  'reader_cjson.c'
]

textmate_inc = [
  './'
]

cjson_files = [
  './cJSON/cJSON.c',
  './cJSON/cJSON_Utils.c'
]

cjson_inc = [
  './cJSON'
]

onigmo_dep = dependency('onigmo', required: true, disabler: true)

executable('tiny',
  'main.c',
  'tests/dump.c',
  cjson_files,
  textmate_files,
  include_directories: [ cjson_inc, textmate_inc ],
  dependencies: [ onigmo_dep ]
)

executable('test_syntax',
  'tests/syntax.c',
  'tests/dump.c',
  cjson_files,
  textmate_files,
  include_directories: [ cjson_inc, textmate_inc ],
  dependencies: [ onigmo_dep ]
)

executable('test_package',
  'tests/package.c',
  'tests/dump.c',
  cjson_files,
  textmate_files,
  include_directories: [ cjson_inc, textmate_inc ],
  dependencies: [ onigmo_dep ]
)

executable('test_list',
  'tests/dump.c',
  'tests/list.c',
  cjson_files,
  textmate_files,
  include_directories: [ cjson_inc, textmate_inc ],
  dependencies: [ onigmo_dep ]
)

executable('test_json',
  'tests/dump.c',
  'tests/json.c',
  cjson_files,
  textmate_files,
  include_directories: [ cjson_inc, textmate_inc ],
  dependencies: [ onigmo_dep ]
)

executable('test_regex',
  'tests/regex.c',
  dependencies: [ onigmo_dep ]
)