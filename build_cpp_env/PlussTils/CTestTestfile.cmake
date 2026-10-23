# CMake generated Testfile for 
# Source directory: /Users/yawasante/software/cxx/PlussDev/PlussTils
# Build directory: /Users/yawasante/software/cxx/PlussDev/build_cpp_env/PlussTils
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test([=[plusstils_core_env]=] "/Users/yawasante/software/cxx/PlussDev/build_cpp_env/PlussTils/plusstils_core_env_tests")
set_tests_properties([=[plusstils_core_env]=] PROPERTIES  _BACKTRACE_TRIPLES "/Users/yawasante/software/cxx/PlussDev/PlussTils/CMakeLists.txt;59;add_test;/Users/yawasante/software/cxx/PlussDev/PlussTils/CMakeLists.txt;0;")
add_test([=[plusstils_core_seed]=] "/Users/yawasante/software/cxx/PlussDev/build_cpp_env/PlussTils/plusstils_core_seed_tests")
set_tests_properties([=[plusstils_core_seed]=] PROPERTIES  _BACKTRACE_TRIPLES "/Users/yawasante/software/cxx/PlussDev/PlussTils/CMakeLists.txt;66;add_test;/Users/yawasante/software/cxx/PlussDev/PlussTils/CMakeLists.txt;0;")
add_test([=[plusstils_scalar_cast_compat]=] "/Users/yawasante/software/cxx/PlussDev/build_cpp_env/PlussTils/plusstils_scalar_cast_compat_tests")
set_tests_properties([=[plusstils_scalar_cast_compat]=] PROPERTIES  _BACKTRACE_TRIPLES "/Users/yawasante/software/cxx/PlussDev/PlussTils/CMakeLists.txt;73;add_test;/Users/yawasante/software/cxx/PlussDev/PlussTils/CMakeLists.txt;0;")
subdirs("view")
subdirs("src/PlussBI")
