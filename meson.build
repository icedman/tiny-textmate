project('text-edit', 'c', 'cpp',
  version : '0.1',
  default_options : ['warning_level=3', 'c_std=gnu17']
)

textmate_files = [
  'textmate.c',
  'processor.c',
  'parser.c',
  'theme.c',
  'package.c',
  'reader_cjson.c'
]

textmate_inc = [
  './'
]

cjson_files = [
  './cJSON/cJSON.c',
  # './cJSON/cJSON_Utils.c'
]

cjson_inc = [
  './cJSON'
]

munit_files = [
  './munit/munit.c'
]

munit_inc = [
  './munit'
]

onigmo_files = [
    './Onigmo/regcomp.c',
    './Onigmo/regexec.c',
    './Onigmo/regparse.c',
    './Onigmo/regsyntax.c',
    './Onigmo/st.c',
    './Onigmo/regenc.c',
    './Onigmo/regext.c',
    './Onigmo/regposerr.c',
    './Onigmo/regtrav.c',
    './Onigmo/regerror.c',
    './Onigmo/reggnu.c',
    './Onigmo/regposix.c',
    './Onigmo/regversion.c',
    './Onigmo/enc/ascii.c',
    #'./Onigmo/enc/euc_kr.c',
    './Onigmo/enc/iso_8859_10.c',
    './Onigmo/enc/iso_8859_16.c',
    './Onigmo/enc/iso_8859_5.c',
    #'./Onigmo/enc/koi8_r.c',
    #'./Onigmo/enc/us_ascii.c',
    './Onigmo/enc/utf_8.c',
    #'./Onigmo/enc/windows_1254.c',
    #'./Onigmo/enc/big5.c   ',
    #'./Onigmo/enc/euc_tw.c',
    #'./Onigmo/enc/iso_8859_11.c',
    './Onigmo/enc/iso_8859_1.c',
    #'./Onigmo/enc/iso_8859_6.c',
    #'./Onigmo/enc/koi8_u.c',
    './Onigmo/enc/utf_16be.c',
    #'./Onigmo/enc/windows_1250.c',
    #'./Onigmo/enc/windows_1257.c',
    #'./Onigmo/enc/cp949.c  ',
    #'./Onigmo/enc/gb18030.c',
    #'./Onigmo/enc/iso_8859_13.c',
    #'./Onigmo/enc/iso_8859_2.c',
    #'./Onigmo/enc/iso_8859_7.c',
    #'./Onigmo/enc/mktable.c ',
    './Onigmo/enc/utf_16le.c',
    #'./Onigmo/enc/windows_1251.c',
    #'./Onigmo/enc/windows_31j.c',
    #'./Onigmo/enc/emacs_mule.c',
    #'./Onigmo/enc/gb2312.c',
    #'./Onigmo/enc/iso_8859_14.c',
    #'./Onigmo/enc/iso_8859_3.c',
    #'./Onigmo/enc/iso_8859_8.c',
    './Onigmo/enc/shift_jis.c',
    './Onigmo/enc/utf_32be.c',
    #'./Onigmo/enc/windows_1252.c',
    './Onigmo/enc/euc_jp.c',
    #'./Onigmo/enc/gbk.c ',
    #'./Onigmo/enc/iso_8859_15.c',
    #'./Onigmo/enc/iso_8859_4.c',
    #'./Onigmo/enc/iso_8859_9.c',
    './Onigmo/enc/unicode.c',
    './Onigmo/enc/utf_32le.c'
    #'./Onigmo/enc/windows_1253.c',
]

onigmo_inc = [
  './Onigmo',
  './Onigmo/enc',
  './Onigmo/enc/jis',
  './Onigmo/enc/unicode'
]

# onigmo_dep = dependency('onigmo', required: true, disabler: true)

executable('tiny',
  'main.c',
  'tests/dump.c',
  cjson_files,
  onigmo_files,
  textmate_files,
  include_directories: [ onigmo_inc, cjson_inc, textmate_inc ],
  c_args: ['-Wno-incompatible-pointer-types', '-Wno-missing-field-initializers'],
  # dependencies: [ onigmo_dep ]
)

executable('code_pp',
  'code_pp.c',
  'tests/dump.c',
  cjson_files,
  onigmo_files,
  textmate_files,
  include_directories: [ onigmo_inc, cjson_inc, textmate_inc ],
  c_args: ['-Wno-incompatible-pointer-types', '-Wno-missing-field-initializers'],
  # dependencies: [ onigmo_dep ]
)

executable('sample_syntax',
  'tests/syntax.c',
  'tests/dump.c',
  cjson_files,
  onigmo_files,
  textmate_files,
  include_directories: [ onigmo_inc, cjson_inc, textmate_inc ],
  c_args: ['-Wno-incompatible-pointer-types', '-Wno-missing-field-initializers'],
  # dependencies: [ onigmo_dep ]
)

executable('sample_package',
  'tests/package.c',
  'tests/dump.c',
  cjson_files,
  onigmo_files,
  textmate_files,
  include_directories: [ onigmo_inc, cjson_inc, textmate_inc ],
  c_args: ['-Wno-incompatible-pointer-types', '-Wno-missing-field-initializers'],
  # dependencies: [ onigmo_dep ]
)

executable('sample_theme',
  'tests/theme.c',
  'tests/dump.c',
  cjson_files,
  onigmo_files,
  textmate_files,
  include_directories: [ onigmo_inc, cjson_inc, textmate_inc ],
  c_args: ['-Wno-incompatible-pointer-types', '-Wno-missing-field-initializers'],
  # dependencies: [ onigmo_dep ]
)

executable('sample_list',
  'tests/dump.c',
  'tests/list.c',
  cjson_files,
  onigmo_files,
  textmate_files,
  include_directories: [ onigmo_inc, cjson_inc, textmate_inc ],
  c_args: ['-Wno-incompatible-pointer-types', '-Wno-missing-field-initializers'],
  # dependencies: [ onigmo_dep ]
)

executable('sample_json',
  'tests/dump.c',
  'tests/json.c',
  cjson_files,
  onigmo_files,
  textmate_files,
  include_directories: [ onigmo_inc, cjson_inc, textmate_inc ],
  c_args: ['-Wno-incompatible-pointer-types', '-Wno-missing-field-initializers'],
  # dependencies: [ onigmo_dep ]
)

# executable('sample_regex',
#   'tests/regex.c',
#   c_args: ['-Wno-incompatible-pointer-types', '-Wno-missing-field-initializers'],
#   # dependencies: [ onigmo_dep ]
# )

# executable('tests',
#   'tests/tests.c',
#   'tests/dump.c',
#   munit_files,
#   cjson_files,
  # onigmo_files,
#   textmate_files,
#   include_directories: [ munit_inc, onigmo_inc, cjson_inc, textmate_inc ],
#   c_args: ['-Wno-incompatible-pointer-types', '-Wno-missing-field-initializers'],
#   # dependencies: [ onigmo_dep ]
# )
