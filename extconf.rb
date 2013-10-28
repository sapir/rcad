require 'mkmf-rice'

OCE_INCLUDE_DIR = '/usr/local/include/oce'
OCE_LIB_DIR = '/usr/local/lib'

dir_config('Prim', OCE_INCLUDE_DIR, OCE_LIB_DIR)

create_makefile('_yrcad')
