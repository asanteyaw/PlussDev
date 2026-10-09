# Loaded during language initialization, including nested try_compile builds.
# Keep Conda Clang for C++20 compilation and use Apple's driver for SDK linking.
foreach(_pluss_link_language CXX OBJCXX)
    set(CMAKE_${_pluss_link_language}_LINK_EXECUTABLE
        "/usr/bin/clang++ <FLAGS> <CMAKE_${_pluss_link_language}_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")
endforeach()
unset(_pluss_link_language)
