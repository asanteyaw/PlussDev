# CMake generated Testfile for 
# Source directory: /Users/yawasante/software/cxx/PlussDev
# Build directory: /Users/yawasante/software/cxx/PlussDev/build_cpp_env
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test([=[alias_registry_lint]=] "/opt/homebrew/bin/cmake" "-E" "env" "python3" "/Users/yawasante/software/cxx/PlussDev/tools/lint_alias_registry.py")
set_tests_properties([=[alias_registry_lint]=] PROPERTIES  WORKING_DIRECTORY "/Users/yawasante/software/cxx/PlussDev" _BACKTRACE_TRIPLES "/Users/yawasante/software/cxx/PlussDev/CMakeLists.txt;540;add_test;/Users/yawasante/software/cxx/PlussDev/CMakeLists.txt;0;")
add_test([=[pluss_sdk_consumer_smoke]=] "/opt/homebrew/bin/cmake" "-DTOP_LEVEL_BUILD_DIR=/Users/yawasante/software/cxx/PlussDev/build_cpp_env" "-DSDK_STAGE_DIR=/Users/yawasante/software/cxx/PlussDev/build_cpp_env/sdk/pluss-sdk-0.1.0-Darwin-arm64" "-DSDK_CONSUMER_SOURCE_DIR=/Users/yawasante/software/cxx/PlussDev/examples/sdk_consumer" "-DSDK_CONSUMER_BUILD_DIR=/Users/yawasante/software/cxx/PlussDev/build_cpp_env/sdk_consumer_smoke" "-P" "/Users/yawasante/software/cxx/PlussDev/cmake/RunSdkConsumerSmoke.cmake")
set_tests_properties([=[pluss_sdk_consumer_smoke]=] PROPERTIES  _BACKTRACE_TRIPLES "/Users/yawasante/software/cxx/PlussDev/CMakeLists.txt;546;add_test;/Users/yawasante/software/cxx/PlussDev/CMakeLists.txt;0;")
subdirs("PlussAI")
subdirs("PlussQit")
subdirs("PlussTils")
subdirs("PlussPl")
subdirs("PlussMa")
subdirs("PlussMic")
