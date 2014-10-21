require 'rbconfig'
require 'mkmf-rice'

OCE_INCLUDE_DIR = '/usr/include/oce'
OCE_LIB_DIR = '/usr/lib'


dir_config('TKG3d',    OCE_INCLUDE_DIR, OCE_LIB_DIR)
dir_config('TKBRep',   OCE_INCLUDE_DIR, OCE_LIB_DIR)
dir_config('TKPrim',   OCE_INCLUDE_DIR, OCE_LIB_DIR)
dir_config('TKOffset', OCE_INCLUDE_DIR, OCE_LIB_DIR)
dir_config('TKBO',     OCE_INCLUDE_DIR, OCE_LIB_DIR)
dir_config('TKSTL',    OCE_INCLUDE_DIR, OCE_LIB_DIR)
dir_config('qhull')


# HACK: modify compiled src so that test function can try to call main()
# despite it not having a prototype. we simply add the prototype.
def fixed_have_lib(*args)
  have_library(*args) { |src|
    # prepend prototype for main
    "int main(int argc, char **argv);\n" + src
  }
end

def have_oce_lib(name)
  fixed_have_lib('TK' + name)
end


have_oce_lib('G3d')    or raise
have_oce_lib('BRep')   or raise
have_oce_lib('Prim')   or raise
have_oce_lib('Offset') or raise
have_oce_lib('BO')     or raise
have_oce_lib('STL')    or raise
fixed_have_lib('qhull') or raise

create_makefile('rcad/_rcad')
