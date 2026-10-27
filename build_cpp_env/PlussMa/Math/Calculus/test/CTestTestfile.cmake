# CMake generated Testfile for 
# Source directory: /Users/yawasante/software/cxx/PlussDev/PlussMa/Math/Calculus/test
# Build directory: /Users/yawasante/software/cxx/PlussDev/build_cpp_env/PlussMa/Math/Calculus/test
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test([=[plussma_calculus_contract]=] "/Users/yawasante/software/cxx/PlussDev/build_cpp_env/PlussMa/Math/Calculus/test/plussma_calculus_contract_test")
set_tests_properties([=[plussma_calculus_contract]=] PROPERTIES  _BACKTRACE_TRIPLES "/Users/yawasante/software/cxx/PlussDev/PlussMa/Math/Calculus/test/CMakeLists.txt;19;add_test;/Users/yawasante/software/cxx/PlussDev/PlussMa/Math/Calculus/test/CMakeLists.txt;0;")
add_test([=[plussma_calculus_public_surface_audit]=] "/opt/homebrew/bin/cmake" "-DPLUSS_CALCULUS_INCLUDE_DIR=/Users/yawasante/software/cxx/PlussDev/PlussMa/include" "-P" "/Users/yawasante/software/cxx/PlussDev/PlussMa/Math/Calculus/test/audit_public_surface.cmake")
set_tests_properties([=[plussma_calculus_public_surface_audit]=] PROPERTIES  _BACKTRACE_TRIPLES "/Users/yawasante/software/cxx/PlussDev/PlussMa/Math/Calculus/test/CMakeLists.txt;24;add_test;/Users/yawasante/software/cxx/PlussDev/PlussMa/Math/Calculus/test/CMakeLists.txt;0;")
