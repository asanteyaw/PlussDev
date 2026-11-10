cmake_minimum_required(VERSION 3.28)
project(PlussTils LANGUAGES CXX)
include(GNUInstallDirs)

add_subdirectory(view)
add_subdirectory(src/PlussBI)

add_library(PlussTilsCore STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/env/env.cpp
)
add_library(Pluss::TilsCore ALIAS PlussTilsCore)
target_include_directories(PlussTilsCore PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
set_target_properties(PlussTilsCore PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

add_library(PlussTils INTERFACE)
add_library(Pluss::Tils ALIAS PlussTils)

target_link_libraries(PlussTils INTERFACE PlussTilsCore)
if(TARGET PlussTilsView)
    target_link_libraries(PlussTils INTERFACE PlussTilsView)
endif()
if(TARGET PlussBI)
    target_link_libraries(PlussTils INTERFACE PlussBI)
endif()
if(TARGET PlussQit)
    target_link_libraries(PlussTils INTERFACE PlussQit)
endif()

target_include_directories(PlussTils INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

add_executable(plusstils_core_env_reference
    ${CMAKE_CURRENT_SOURCE_DIR}/examples/tils/core_env_reference.cpp
)
target_link_libraries(plusstils_core_env_reference PRIVATE PlussTilsCore)
target_compile_features(plusstils_core_env_reference PRIVATE cxx_std_20)

add_executable(plusstils_core_seed_reference
    ${CMAKE_CURRENT_SOURCE_DIR}/examples/tils/core_seed_reference.cpp
)
target_link_libraries(plusstils_core_seed_reference PRIVATE PlussTilsCore)
target_compile_features(plusstils_core_seed_reference PRIVATE cxx_std_20)

add_executable(plusstils_suppose_checked
    ${CMAKE_CURRENT_SOURCE_DIR}/examples/tils/suppose_checked.cpp
)
target_link_libraries(plusstils_suppose_checked PRIVATE PlussTils)
target_compile_features(plusstils_suppose_checked PRIVATE cxx_std_20)

if(BUILD_TESTING)
    add_executable(plusstils_suppose_reasoning_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/test/suppose_reasoning_tests.cpp
    )
    target_include_directories(plusstils_suppose_reasoning_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_compile_features(plusstils_suppose_reasoning_tests PRIVATE cxx_std_20)
    add_test(NAME plusstils_suppose_reasoning COMMAND plusstils_suppose_reasoning_tests)

    add_executable(plusstils_suppose_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/test/suppose_tests.cpp
    )
    target_link_libraries(plusstils_suppose_tests PRIVATE PlussTils)
    target_compile_features(plusstils_suppose_tests PRIVATE cxx_std_20)
    add_test(NAME plusstils_suppose COMMAND plusstils_suppose_tests)

    add_executable(plusstils_core_env_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/test/core_env_tests.cpp
    )
    target_link_libraries(plusstils_core_env_tests PRIVATE PlussTilsCore)
    target_compile_features(plusstils_core_env_tests PRIVATE cxx_std_20)
    add_test(NAME plusstils_core_env COMMAND plusstils_core_env_tests)

    add_executable(plusstils_core_seed_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/test/core_seed_tests.cpp
    )
    target_link_libraries(plusstils_core_seed_tests PRIVATE PlussTilsCore)
    target_compile_features(plusstils_core_seed_tests PRIVATE cxx_std_20)
    add_test(NAME plusstils_core_seed COMMAND plusstils_core_seed_tests)

    add_executable(plusstils_scalar_cast_compat_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/test/scalar_cast_compat_tests.cpp
    )
    target_link_libraries(plusstils_scalar_cast_compat_tests PRIVATE PlussTils)
    target_compile_features(plusstils_scalar_cast_compat_tests PRIVATE cxx_std_20)
    add_test(NAME plusstils_scalar_cast_compat COMMAND plusstils_scalar_cast_compat_tests)
endif()

install(TARGETS PlussTilsCore PlussTils
    EXPORT PlussTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "*.hh"
    PATTERN ".DS_Store" EXCLUDE
)
