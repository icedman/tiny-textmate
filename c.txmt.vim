syn off

syn region meta_block_switch_c start=/\(\(\(?>\(?:\(?:\(?>\(?<!\s\)\s+\)\|\(\/\*\)\(\(?>\(?:[^\*]\|\(?>\*+\)[^\/]\)*\)\(\(?>\*+\)\/\)\)\)+\|\(?:\(?:\(?:\(?:\|\(?<=\W\)\)\|\(?=\W\)\)\|\A\)\|\Z\)\)\)\)\(\(?<!\w\)switch\(?!\w\)\)\)/ end=/\(\(\(?>\(?:\(?:\(?>\(?<!\s\)\s+\)\|\(\/\*\)\(\(?>\(?:[^\*]\|\(?>\*+\)[^\/]\)*\)\(\(?>\*+\)\/\)\)\)+\|\(?:\(?:\(?:\(?:\|\(?<=\W\)\)\|\(?=\W\)\)\|\A\)\|\Z\)\)\)\)\(\(?<!\w\)switch\(?!\w\)\)\)/ contains=meta_head_switch_c,meta_body_switch_c,meta_tail_switch_c

syn match keyword_control_c /\(break\|continue\|do\|else\|for\|goto\|if\|_Pragma\|return\|while\)/
hi link keyword_control_c Keyword

syn match keyword_other_typedef_c /typedef/
hi link keyword_other_typedef_c Keyword

syn match storage_modifier_c /\(const\|extern\|register\|restrict\|static\|volatile\|inline\)/
hi link storage_modifier_c Type

syn match constant_other_variable_mac_classic_c /k[A-Z]\w*/
hi link constant_other_variable_mac_classic_c Variable

syn match variable_other_readwrite_global_mac_classic_c /g[A-Z]\w*/
hi link variable_other_readwrite_global_mac_classic_c Variable

syn match variable_other_readwrite_static_mac_classic_c /s[A-Z]\w*/
hi link variable_other_readwrite_static_mac_classic_c Variable

syn match constant_language_c /\(NULL\|true\|false\|TRUE\|FALSE\)/
hi link constant_language_c Constant

syn match constant_language_c /\(?<!\w\)\.?\d\(?:\(?:[0-9a-zA-Z_\.]\|'\)\|\(?<=[eEpP]\)[+-]\)*/
hi link constant_language_c Constant

syn region meta_preprocessor_macro_c start=/\(\(?:\(?:\(?>\s+\)\|\(\/\*\)\(\(?>\(?:[^\*]\|\(?>\*+\)[^\/]\)*\)\(\(?>\*+\)\/\)\)\)+?\|\(?:\(?:\(?:\(?:\|\(?<=\W\)\)\|\(?=\W\)\)\|\A\)\|\Z\)\)\)\(\(#\)\s*define\)\s+\(\(?<!\w\)[a-zA-Z_]\w*\(?!\w\)\)\(?:\(\\(\)\([^\(\)\\]+\)\(\\)\)\)?/ end=/\(\(?:\(?:\(?>\s+\)\|\(\/\*\)\(\(?>\(?:[^\*]\|\(?>\*+\)[^\/]\)*\)\(\(?>\*+\)\/\)\)\)+?\|\(?:\(?:\(?:\(?:\|\(?<=\W\)\)\|\(?=\W\)\)\|\A\)\|\Z\)\)\)\(\(#\)\s*define\)\s+\(\(?<!\w\)[a-zA-Z_]\w*\(?!\w\)\)\(?:\(\\(\)\([^\(\)\\]+\)\(\\)\)\)?/ contains=
hi link meta_preprocessor_macro_c Preprocessor

syn region meta_preprocessor_diagnostic_c start=/^\s*\(\(#\)\s*\(error\|warning\)\)\s*/ end=/^\s*\(\(#\)\s*\(error\|warning\)\)\s*/ contains=string_quoted_double_c,string_quoted_single_c,string_unquoted_single_c
hi link meta_preprocessor_diagnostic_c Preprocessor

syn region meta_preprocessor_include_c start=/^\s*\(\(#\)\s*\(include\(?:_next\)?\|import\)\)\s*/ end=/^\s*\(\(#\)\s*\(include\(?:_next\)?\|import\)\)\s*/ contains=string_quoted_double_include_c,string_quoted_other_lt_gt_include_c
hi link meta_preprocessor_include_c Preprocessor

syn match meta_section_c /^\s*\(\(\(#\)\s*pragma\s+mark\)\s+\(.*\)\)/

syn region meta_preprocessor_c start=/^\s*\(\(#\)\s*line\)/ end=/^\s*\(\(#\)\s*line\)/ contains=
hi link meta_preprocessor_c Preprocessor

syn region meta_preprocessor_c start=/^\s*\(?:\(\(#\)\s*undef\)\)/ end=/^\s*\(?:\(\(#\)\s*undef\)\)/ contains=entity_name_function_preprocessor_c,
hi link meta_preprocessor_c Preprocessor

syn region meta_preprocessor_pragma_c start=/^\s*\(?:\(\(#\)\s*pragma\)\)/ end=/^\s*\(?:\(\(#\)\s*pragma\)\)/ contains=entity_other_attribute_name_pragma_preprocessor_c,,
hi link meta_preprocessor_pragma_c Preprocessor

syn match support_type_sys_types_c /\(u_char\|u_short\|u_int\|u_long\|ushort\|uint\|u_quad_t\|quad_t\|qaddr_t\|caddr_t\|daddr_t\|div_t\|dev_t\|fixpt_t\|blkcnt_t\|blksize_t\|gid_t\|in_addr_t\|in_port_t\|ino_t\|key_t\|mode_t\|nlink_t\|id_t\|pid_t\|off_t\|segsz_t\|swblk_t\|uid_t\|id_t\|clock_t\|size_t\|ssize_t\|time_t\|useconds_t\|suseconds_t\)/

syn match support_type_pthread_c /\(pthread_attr_t\|pthread_cond_t\|pthread_condattr_t\|pthread_mutex_t\|pthread_mutexattr_t\|pthread_once_t\|pthread_rwlock_t\|pthread_rwlockattr_t\|pthread_t\|pthread_key_t\)/

syn match support_type_stdint_c /\(?x\) \(int8_t\|int16_t\|int32_t\|int64_t\|uint8_t\|uint16_t\|uint32_t\|uint64_t\|int_least8_t\|int_least16_t\|int_least32_t\|int_least64_t\|uint_least8_t\|uint_least16_t\|uint_least32_t\|uint_least64_t\|int_fast8_t\|int_fast16_t\|int_fast32_t\|int_fast64_t\|uint_fast8_t\|uint_fast16_t\|uint_fast32_t\|uint_fast64_t\|intptr_t\|uintptr_t\|intmax_t\|intmax_t\|uintmax_t\|uintmax_t\)/

syn match support_constant_mac_classic_c /\(noErr\|kNilOptions\|kInvalidID\|kVariableLengthArray\)/
hi link support_constant_mac_classic_c Constant

syn match support_type_mac_classic_c /\(?x\) \(AbsoluteTime\|Boolean\|Byte\|ByteCount\|ByteOffset\|BytePtr\|CompTimeValue\|ConstLogicalAddress\|ConstStrFileNameParam\|ConstStringPtr\|Duration\|Fixed\|FixedPtr\|Float32\|Float32Point\|Float64\|Float80\|Float96\|FourCharCode\|Fract\|FractPtr\|Handle\|ItemCount\|LogicalAddress\|OptionBits\|OSErr\|OSStatus\|OSType\|OSTypePtr\|PhysicalAddress\|ProcessSerialNumber\|ProcessSerialNumberPtr\|ProcHandle\|Ptr\|ResType\|ResTypePtr\|ShortFixed\|ShortFixedPtr\|SignedByte\|SInt16\|SInt32\|SInt64\|SInt8\|Size\|StrFileName\|StringHandle\|StringPtr\|TimeBase\|TimeRecord\|TimeScale\|TimeValue\|TimeValue64\|UInt16\|UInt32\|UInt64\|UInt8\|UniChar\|UniCharCount\|UniCharCountPtr\|UniCharPtr\|UnicodeScalarValue\|UniversalProcHandle\|UniversalProcPtr\|UnsignedFixed\|UnsignedFixedPtr\|UnsignedWide\|UTF16Char\|UTF32Char\|UTF8Char\)/

syn match support_type_posix_reserved_c /\([A-Za-z0-9_]+_t\)/

syn region meta_parens_c start=/\\(/ end=/\\(/ contains=C\

syn region meta_function_c start=/\(?<!\w\)\(?!\s*\(?:atomic_uint_least64_t\|atomic_uint_least16_t\|atomic_uint_least32_t\|atomic_uint_least8_t\|atomic_int_least16_t\|atomic_uint_fast64_t\|atomic_uint_fast32_t\|atomic_int_least64_t\|atomic_int_least32_t\|pthread_rwlockattr_t\|atomic_uint_fast16_t\|pthread_mutexattr_t\|atomic_int_fast16_t\|atomic_uint_fast8_t\|atomic_int_fast64_t\|atomic_int_least8_t\|atomic_int_fast32_t\|atomic_int_fast8_t\|pthread_condattr_t\|pthread_rwlock_t\|atomic_uintptr_t\|atomic_ptrdiff_t\|atomic_uintmax_t\|atomic_intmax_t\|atomic_char32_t\|atomic_intptr_t\|atomic_char16_t\|pthread_mutex_t\|pthread_cond_t\|atomic_wchar_t\|uint_least64_t\|uint_least32_t\|uint_least16_t\|pthread_once_t\|pthread_attr_t\|uint_least8_t\|int_least32_t\|int_least16_t\|pthread_key_t\|uint_fast32_t\|uint_fast64_t\|uint_fast16_t\|atomic_size_t\|atomic_ushort\|atomic_ullong\|int_least64_t\|atomic_ulong\|int_least8_t\|int_fast16_t\|int_fast32_t\|int_fast64_t\|uint_fast8_t\|memory_order\|atomic_schar\|atomic_uchar\|atomic_short\|atomic_llong\|thread_local\|atomic_bool\|atomic_uint\|atomic_long\|int_fast8_t\|suseconds_t\|atomic_char\|atomic_int\|useconds_t\|_Imaginary\|uintmax_t\|uintmax_t\|in_addr_t\|in_port_t\|_Noreturn\|blksize_t\|pthread_t\|uintptr_t\|volatile\|u_quad_t\|blkcnt_t\|intmax_t\|intptr_t\|_Complex\|uint16_t\|uint32_t\|uint64_t\|_Alignof\|_Alignas\|continue\|unsigned\|restrict\|intmax_t\|register\|int64_t\|qaddr_t\|segsz_t\|_Atomic\|alignas\|default\|caddr_t\|nlink_t\|typedef\|u_short\|fixpt_t\|clock_t\|swblk_t\|ssize_t\|alignof\|daddr_t\|int16_t\|int32_t\|uint8_t\|struct\|mode_t\|size_t\|time_t\|ushort\|u_long\|u_char\|int8_t\|double\|signed\|static\|extern\|inline\|return\|switch\|xor_eq\|and_eq\|bitand\|not_eq\|sizeof\|quad_t\|uid_t\|bitor\|union\|off_t\|key_t\|ino_t\|compl\|u_int\|short\|const\|false\|while\|float\|pid_t\|break\|_Bool\|or_eq\|div_t\|dev_t\|gid_t\|id_t\|long\|case\|goto\|else\|bool\|auto\|id_t\|enum\|uint\|true\|NULL\|void\|char\|for\|not\|int\|and\|xor\|do\|or\|if\)\s*\\(\)\(?=[a-zA-Z_]\w*\s*\\(\)/ end=/\(?<!\w\)\(?!\s*\(?:atomic_uint_least64_t\|atomic_uint_least16_t\|atomic_uint_least32_t\|atomic_uint_least8_t\|atomic_int_least16_t\|atomic_uint_fast64_t\|atomic_uint_fast32_t\|atomic_int_least64_t\|atomic_int_least32_t\|pthread_rwlockattr_t\|atomic_uint_fast16_t\|pthread_mutexattr_t\|atomic_int_fast16_t\|atomic_uint_fast8_t\|atomic_int_fast64_t\|atomic_int_least8_t\|atomic_int_fast32_t\|atomic_int_fast8_t\|pthread_condattr_t\|pthread_rwlock_t\|atomic_uintptr_t\|atomic_ptrdiff_t\|atomic_uintmax_t\|atomic_intmax_t\|atomic_char32_t\|atomic_intptr_t\|atomic_char16_t\|pthread_mutex_t\|pthread_cond_t\|atomic_wchar_t\|uint_least64_t\|uint_least32_t\|uint_least16_t\|pthread_once_t\|pthread_attr_t\|uint_least8_t\|int_least32_t\|int_least16_t\|pthread_key_t\|uint_fast32_t\|uint_fast64_t\|uint_fast16_t\|atomic_size_t\|atomic_ushort\|atomic_ullong\|int_least64_t\|atomic_ulong\|int_least8_t\|int_fast16_t\|int_fast32_t\|int_fast64_t\|uint_fast8_t\|memory_order\|atomic_schar\|atomic_uchar\|atomic_short\|atomic_llong\|thread_local\|atomic_bool\|atomic_uint\|atomic_long\|int_fast8_t\|suseconds_t\|atomic_char\|atomic_int\|useconds_t\|_Imaginary\|uintmax_t\|uintmax_t\|in_addr_t\|in_port_t\|_Noreturn\|blksize_t\|pthread_t\|uintptr_t\|volatile\|u_quad_t\|blkcnt_t\|intmax_t\|intptr_t\|_Complex\|uint16_t\|uint32_t\|uint64_t\|_Alignof\|_Alignas\|continue\|unsigned\|restrict\|intmax_t\|register\|int64_t\|qaddr_t\|segsz_t\|_Atomic\|alignas\|default\|caddr_t\|nlink_t\|typedef\|u_short\|fixpt_t\|clock_t\|swblk_t\|ssize_t\|alignof\|daddr_t\|int16_t\|int32_t\|uint8_t\|struct\|mode_t\|size_t\|time_t\|ushort\|u_long\|u_char\|int8_t\|double\|signed\|static\|extern\|inline\|return\|switch\|xor_eq\|and_eq\|bitand\|not_eq\|sizeof\|quad_t\|uid_t\|bitor\|union\|off_t\|key_t\|ino_t\|compl\|u_int\|short\|const\|false\|while\|float\|pid_t\|break\|_Bool\|or_eq\|div_t\|dev_t\|gid_t\|id_t\|long\|case\|goto\|else\|bool\|auto\|id_t\|enum\|uint\|true\|NULL\|void\|char\|for\|not\|int\|and\|xor\|do\|or\|if\)\s*\\(\)\(?=[a-zA-Z_]\w*\s*\\(\)/ contains=

syn region meta_bracket_square_access_c start=/\([a-zA-Z_][a-zA-Z_0-9]*\|\(?<=[\]\\)]\)\)?\(\[\)\(?!\]\)/ end=/\([a-zA-Z_][a-zA-Z_0-9]*\|\(?<=[\]\\)]\)\)?\(\[\)\(?!\]\)/ contains=

syn match storage_modifier_array_bracket_square_c /\[\s*\]/
hi link storage_modifier_array_bracket_square_c Type

syn match punctuation_terminator_statement_c /;/

syn match punctuation_separator_delimiter_c /,/

syn region meta_function_call_member_c start=/\([a-zA-Z_][a-zA-Z_0-9]*\|\(?<=[\]\\)]\)\)\s*\(?:\(\.\)\|\(->\)\)\(\(?:\(?:[a-zA-Z_][a-zA-Z_0-9]*\)\s*\(?:\(?:\.\)\|\(?:->\)\)\)*\)\s*\([a-zA-Z_][a-zA-Z_0-9]*\)\(\\(\)/ end=/\([a-zA-Z_][a-zA-Z_0-9]*\|\(?<=[\]\\)]\)\)\s*\(?:\(\.\)\|\(->\)\)\(\(?:\(?:[a-zA-Z_][a-zA-Z_0-9]*\)\s*\(?:\(?:\.\)\|\(?:->\)\)\)*\)\s*\([a-zA-Z_][a-zA-Z_0-9]*\)\(\\(\)/ contains= contained

syn match constant_character_escape_c /\(?x\)\\ \(\\			 \|[abefnprtv'"?]   \|[0-3][0-7]\{,2\}	 \|[4-7]\d?		\|x[a-fA-F0-9]\{,2\} \|u[a-fA-F0-9]\{,4\} \|U[a-fA-F0-9]\{,8\} \)/ contained
hi link constant_character_escape_c Constant

syn region meta_function_call_c start=/\(?x\)\(?!\(?:while\|for\|do\|if\|else\|switch\|catch\|enumerate\|return\|typeid\|alignof\|alignas\|sizeof\|[cr]?iterate\|and\|and_eq\|bitand\|bitor\|compl\|not\|not_eq\|or\|or_eq\|typeid\|xor\|xor_eq\|alignof\|alignas\)\s*\\(\)\(?=\(?:[A-Za-z_][A-Za-z0-9_]*+\|::\)++\s*\\(  # actual name\|\(?:\(?<=operator\)\(?:[-*&<>=+!]+\|\\(\\)\|\[\]\)\)\s*\\(\)/ end=/\(?x\)\(?!\(?:while\|for\|do\|if\|else\|switch\|catch\|enumerate\|return\|typeid\|alignof\|alignas\|sizeof\|[cr]?iterate\|and\|and_eq\|bitand\|bitor\|compl\|not\|not_eq\|or\|or_eq\|typeid\|xor\|xor_eq\|alignof\|alignas\)\s*\\(\)\(?=\(?:[A-Za-z_][A-Za-z0-9_]*+\|::\)++\s*\\(  # actual name\|\(?:\(?<=operator\)\(?:[-*&<>=+!]+\|\\(\\)\|\[\]\)\)\s*\\(\)/ contains= contained

syn region meta_conditional_case_c start=/\(\(?>\(?:\(?:\(?>\(?<!\s\)\s+\)\|\(\/\*\)\(\(?>\(?:[^\*]\|\(?>\*+\)[^\/]\)*\)\(\(?>\*+\)\/\)\)\)+\|\(?:\(?:\(?:\(?:\|\(?<=\W\)\)\|\(?=\W\)\)\|\A\)\|\Z\)\)\)\)\(\(?<!\w\)case\(?!\w\)\)/ end=/\(\(?>\(?:\(?:\(?>\(?<!\s\)\s+\)\|\(\/\*\)\(\(?>\(?:[^\*]\|\(?>\*+\)[^\/]\)*\)\(\(?>\*+\)\/\)\)\)+\|\(?:\(?:\(?:\(?:\|\(?<=\W\)\)\|\(?=\W\)\)\|\A\)\|\Z\)\)\)\)\(\(?<!\w\)case\(?!\w\)\)/ contains= contained

syn region meta_conditional_case_c start=/\(\(?>\(?:\(?:\(?>\(?<!\s\)\s+\)\|\(\/\*\)\(\(?>\(?:[^\*]\|\(?>\*+\)[^\/]\)*\)\(\(?>\*+\)\/\)\)\)+\|\(?:\(?:\(?:\(?:\|\(?<=\W\)\)\|\(?=\W\)\)\|\A\)\|\Z\)\)\)\)\(\(?<!\w\)default\(?!\w\)\)/ end=/\(\(?>\(?:\(?:\(?>\(?<!\s\)\s+\)\|\(\/\*\)\(\(?>\(?:[^\*]\|\(?>\*+\)[^\/]\)*\)\(\(?>\*+\)\/\)\)\)+\|\(?:\(?:\(?:\(?:\|\(?<=\W\)\)\|\(?=\W\)\)\|\A\)\|\Z\)\)\)\)\(\(?<!\w\)default\(?!\w\)\)/ contains= contained

syn region meta_conditional_case_c start=/^\s*#\s*if\(n?def\)?.*$/ end=/^\s*#\s*if\(n?def\)?.*$/ contains=meta_section_c contained

syn match meta_conditional_case_c /\(\/\*\)\(\(?>\(?:[^\*]\|\(?>\*+\)[^\/]\)*\)\(\(?>\*+\)\/\)\)/ contained

syn match meta_conditional_case_c /\(\(?:[a-zA-Z_]\w*\|\(?<=\]\|\\)\)\)\s*\)\(?:\(\(?:\.\*\|\.\)\)\|\(\(?:->\*\|->\)\)\)\(\(?:[a-zA-Z_]\w*\s*\(?:\(?:\(?:\.\*\|\.\)\)\|\(?:\(?:->\*\|->\)\)\)\s*\)*\)\s*\(\(?!\(?:atomic_uint_least64_t\|atomic_uint_least16_t\|atomic_uint_least32_t\|atomic_uint_least8_t\|atomic_int_least16_t\|atomic_uint_fast64_t\|atomic_uint_fast32_t\|atomic_int_least64_t\|atomic_int_least32_t\|pthread_rwlockattr_t\|atomic_uint_fast16_t\|pthread_mutexattr_t\|atomic_int_fast16_t\|atomic_uint_fast8_t\|atomic_int_fast64_t\|atomic_int_least8_t\|atomic_int_fast32_t\|atomic_int_fast8_t\|pthread_condattr_t\|atomic_uintptr_t\|atomic_ptrdiff_t\|pthread_rwlock_t\|atomic_uintmax_t\|pthread_mutex_t\|atomic_intmax_t\|atomic_intptr_t\|atomic_char32_t\|atomic_char16_t\|pthread_attr_t\|atomic_wchar_t\|uint_least64_t\|uint_least32_t\|uint_least16_t\|pthread_cond_t\|pthread_once_t\|uint_fast64_t\|uint_fast16_t\|atomic_size_t\|uint_least8_t\|int_least64_t\|int_least32_t\|int_least16_t\|pthread_key_t\|atomic_ullong\|atomic_ushort\|uint_fast32_t\|atomic_schar\|atomic_short\|uint_fast8_t\|int_fast64_t\|int_fast32_t\|int_fast16_t\|atomic_ulong\|atomic_llong\|int_least8_t\|atomic_uchar\|memory_order\|suseconds_t\|int_fast8_t\|atomic_bool\|atomic_char\|atomic_uint\|atomic_long\|atomic_int\|useconds_t\|_Imaginary\|blksize_t\|pthread_t\|in_addr_t\|uintptr_t\|in_port_t\|uintmax_t\|uintmax_t\|blkcnt_t\|uint16_t\|unsigned\|_Complex\|uint32_t\|intptr_t\|intmax_t\|intmax_t\|uint64_t\|u_quad_t\|int64_t\|int32_t\|ssize_t\|caddr_t\|clock_t\|uint8_t\|u_short\|swblk_t\|segsz_t\|int16_t\|fixpt_t\|daddr_t\|nlink_t\|qaddr_t\|size_t\|time_t\|mode_t\|signed\|quad_t\|ushort\|u_long\|u_char\|double\|int8_t\|ino_t\|uid_t\|pid_t\|_Bool\|float\|dev_t\|div_t\|short\|gid_t\|off_t\|u_int\|key_t\|id_t\|uint\|long\|void\|char\|bool\|id_t\|int\)\)[a-zA-Z_]\w*\(?!\\(\)\)/ contained

syn region meta_function_call_member_c start=/\(\(?:[a-zA-Z_]\w*\|\(?<=\]\|\\)\)\)\s*\)\(?:\(\(?:\.\*\|\.\)\)\|\(\(?:->\*\|->\)\)\)\(\(?:[a-zA-Z_]\w*\s*\(?:\(?:\(?:\.\*\|\.\)\)\|\(?:\(?:->\*\|->\)\)\)\s*\)*\)\s*\([a-zA-Z_]\w*\)\(\\(\)/ end=/\(\(?:[a-zA-Z_]\w*\|\(?<=\]\|\\)\)\)\s*\)\(?:\(\(?:\.\*\|\.\)\)\|\(\(?:->\*\|->\)\)\)\(\(?:[a-zA-Z_]\w*\s*\(?:\(?:\(?:\.\*\|\.\)\)\|\(?:\(?:->\*\|->\)\)\)\s*\)*\)\s*\([a-zA-Z_]\w*\)\(\\(\)/ contains= contained

syn match meta_function_call_member_c /\(?<!\w\)\.?\d\(?:\(?:[0-9a-zA-Z_\.]\|'\)\|\(?<=[eEpP]\)[+-]\)*/ contained

syn region meta_parens_c start=/\\(/ end=/\\(/ contains=C\ contained

syn region meta_parens_block_c start=/\\(/ end=/\\(/ contains=punctuation_range_based_c contained

syn match meta_section_c /^\s*\(\(\(#\)\s*pragma\s+mark\)\s+\(.*\)\)/ contained

syn region meta_section_c start=/^\s*\(\(#\)\s*elif\)\(?=\s*\\(*0+\\)*\s*\(?:$\|//\|/\*\)\)/ end=/^\s*\(\(#\)\s*elif\)\(?=\s*\\(*0+\\)*\s*\(?:$\|//\|/\*\)\)/ contains=meta_preprocessor_c,, contained

syn region meta_section_c start=/^\s*\(\(#\)\s*elif\)\(?=\s*\\(*0*1\\)*\s*\(?:$\|//\|/\*\)\)/ end=/^\s*\(\(#\)\s*elif\)\(?=\s*\\(*0*1\\)*\s*\(?:$\|//\|/\*\)\)/ contains=meta_preprocessor_c,, contained

syn region meta_section_c start=/^\s*\(\(#\)\s*elif\)\(?=\s*\\(*0*1\\)*\s*\(?:$\|//\|/\*\)\)/ end=/^\s*\(\(#\)\s*elif\)\(?=\s*\\(*0*1\\)*\s*\(?:$\|//\|/\*\)\)/ contains=meta_preprocessor_c,, contained

syn region meta_section_c start=/^\s*\(\(#\)\s*else\)/ end=/^\s*\(\(#\)\s*else\)/ contains=C^ contained

syn region meta_section_c start=/^\s*\(\(#\)\s*else\)/ end=/^\s*\(\(#\)\s*else\)/ contains= contained

syn match meta_section_c /\(?<=\(?:[a-zA-Z_0-9] \|[&*>\]\\)]\)\)\s*\([a-zA-Z_]\w*\)\s*\(?=\(?:\[\]\s*\)?\(?:,\|\\)\)\)/ contained

syn region meta_section_c start=/\(\(?>\(?:\(?:\(?>\(?<!\s\)\s+\)\|\(\/\*\)\(\(?>\(?:[^\*]\|\(?>\*+\)[^\/]\)*\)\(\(?>\*+\)\/\)\)\)+\|\(?:\(?:\(?:\(?:\|\(?<=\W\)\)\|\(?=\W\)\)\|\A\)\|\Z\)\)\)\)\(\(?<!\w\)static_assert\|_Static_assert\(?!\w\)\)\(\(?>\(?:\(?:\(?>\(?<!\s\)\s+\)\|\(\/\*\)\(\(?>\(?:[^\*]\|\(?>\*+\)[^\/]\)*\)\(\(?>\*+\)\/\)\)\)+\|\(?:\(?:\(?:\(?:\|\(?<=\W\)\)\|\(?=\W\)\)\|\A\)\|\Z\)\)\)\)\(\\(\)/ end=/\(\(?>\(?:\(?:\(?>\(?<!\s\)\s+\)\|\(\/\*\)\(\(?>\(?:[^\*]\|\(?>\*+\)[^\/]\)*\)\(\(?>\*+\)\/\)\)\)+\|\(?:\(?:\(?:\(?:\|\(?<=\W\)\)\|\(?=\W\)\)\|\A\)\|\Z\)\)\)\)\(\(?<!\w\)static_assert\|_Static_assert\(?!\w\)\)\(\(?>\(?:\(?:\(?>\(?<!\s\)\s+\)\|\(\/\*\)\(\(?>\(?:[^\*]\|\(?>\*+\)[^\/]\)*\)\(\(?>\*+\)\/\)\)\)+\|\(?:\(?:\(?:\(?:\|\(?<=\W\)\)\|\(?=\W\)\)\|\A\)\|\Z\)\)\)\)\(\\(\)/ contains=meta_static_assert_message_c, contained

syn region meta_conditional_switch_c start=/\(\(?>\(?:\(?:\(?>\(?<!\s\)\s+\)\|\(\/\*\)\(\(?>\(?:[^\*]\|\(?>\*+\)[^\/]\)*\)\(\(?>\*+\)\/\)\)\)+\|\(?:\(?:\(?:\(?:\|\(?<=\W\)\)\|\(?=\W\)\)\|\A\)\|\Z\)\)\)\)\(\\(\)/ end=/\(\(?>\(?:\(?:\(?>\(?<!\s\)\s+\)\|\(\/\*\)\(\(?>\(?:[^\*]\|\(?>\*+\)[^\/]\)*\)\(\(?>\*+\)\/\)\)\)+\|\(?:\(?:\(?:\(?:\|\(?<=\W\)\)\|\(?=\W\)\)\|\A\)\|\Z\)\)\)\)\(\\(\)/ contains= contained

syn region meta_block_switch_c start=/\(\(\(?>\(?:\(?:\(?>\(?<!\s\)\s+\)\|\(\/\*\)\(\(?>\(?:[^\*]\|\(?>\*+\)[^\/]\)*\)\(\(?>\*+\)\/\)\)\)+\|\(?:\(?:\(?:\(?:\|\(?<=\W\)\)\|\(?=\W\)\)\|\A\)\|\Z\)\)\)\)\(\(?<!\w\)switch\(?!\w\)\)\)/ end=/\(\(\(?>\(?:\(?:\(?>\(?<!\s\)\s+\)\|\(\/\*\)\(\(?>\(?:[^\*]\|\(?>\*+\)[^\/]\)*\)\(\(?>\*+\)\/\)\)\)+\|\(?:\(?:\(?:\(?:\|\(?<=\W\)\)\|\(?=\W\)\)\|\A\)\|\Z\)\)\)\)\(\(?<!\w\)switch\(?!\w\)\)\)/ contains=meta_head_switch_c,meta_body_switch_c,meta_tail_switch_c contained

syn match punctuation_vararg_ellipses_c /\(?<!\.\)\.\.\.\(?!\.\)/ contained
