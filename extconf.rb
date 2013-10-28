require 'mkmf-rice'

OCE_INCLUDE_DIR = '/usr/local/include/oce'
OCE_LIB_DIR = '/usr/local/lib'

dir_config('TKG3d', OCE_INCLUDE_DIR, OCE_LIB_DIR)
dir_config('TKBRep', OCE_INCLUDE_DIR, OCE_LIB_DIR)
dir_config('TKPrim', OCE_INCLUDE_DIR, OCE_LIB_DIR)
dir_config('TKOffset', OCE_INCLUDE_DIR, OCE_LIB_DIR)
dir_config('TKBO', OCE_INCLUDE_DIR, OCE_LIB_DIR)
dir_config('TKSTL', OCE_INCLUDE_DIR, OCE_LIB_DIR)

have_library('TKG3d') or raise
have_library('TKBRep') or raise
have_library('TKPrim') or raise
have_library('TKOffset') or raise
have_library('TKBO') or raise
have_library('TKSTL') or raise

create_makefile('_yrcad')
