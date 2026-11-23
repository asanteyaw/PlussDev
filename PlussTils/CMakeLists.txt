cmake_minimum_required(VERSION 3.28)
project(PlussTils LANGUAGES CXX)
include(GNUInstallDirs)

add_subdirectory(view)
add_subdirectory(src/PlussBI)

# The table of documentation pages that pluss::tils::doc searches, written from the documentation
# sources when this is configured. Without Python, or without the sources (a source archive),
# it is empty and doc() says no page was found.
find_package(Python3 COMPONENTS Interpreter QUIET)
set(PLUSS_DOC_INDEX_DIR ${CMAKE_BINARY_DIR}/generated/pluss/tils)
file(MAKE_DIRECTORY ${PLUSS_DOC_INDEX_DIR})
if(Python3_Interpreter_FOUND AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_doc_index.py)
    execute_process(
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_doc_index.py ${CMAKE_SOURCE_DIR} ${PLUSS_DOC_INDEX_DIR}/doc_index.inc
        RESULT_VARIABLE _pluss_doc_index_result
        OUTPUT_QUIET ERROR_QUIET
    )
    if(NOT _pluss_doc_index_result EQUAL 0)
        message(WARNING "Could not write the documentation index; doc() will find no pages")
    endif()
endif()
if(NOT EXISTS ${PLUSS_DOC_INDEX_DIR}/doc_index.inc)
    file(WRITE ${PLUSS_DOC_INDEX_DIR}/doc_index.inc "")
endif()

add_library(PlussTilsCore STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/env/env.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/doc/doc.cpp
)
target_include_directories(PlussTilsCore PRIVATE ${PLUSS_DOC_INDEX_DIR} ${CMAKE_SOURCE_DIR}/PlussQit/include)

# The documentation, packed into one file beside the libraries, so doc() works with no network.
option(PLUSS_BUNDLE_DOCS "Install the documentation with the libraries for pluss::tils::doc" ON)
if(PLUSS_BUNDLE_DOCS AND Python3_Interpreter_FOUND AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tools/bundle_docs.py)
    execute_process(
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/bundle_docs.py ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR}/docs/docs.pack
        RESULT_VARIABLE _pluss_docs_pack_result
        OUTPUT_QUIET ERROR_QUIET
    )
    if(_pluss_docs_pack_result EQUAL 0 AND EXISTS ${CMAKE_BINARY_DIR}/docs/docs.pack)
        install(FILES ${CMAKE_BINARY_DIR}/docs/docs.pack DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pluss/docs)
    else()
        message(WARNING "Could not pack the documentation; doc() will download pages instead")
    endif()
endif()
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

# The `pluss` command (run, new, doctor). It needs only the standard library and the generated
# version header, so it stays small and is installed beside the libraries in bin/.
option(PLUSS_BUILD_CLI "Build the pluss command-line tool" ON)
if(PLUSS_BUILD_CLI)
    add_executable(pluss_cli ${CMAKE_CURRENT_SOURCE_DIR}/tools/pluss_cli/main.cpp)
    set_target_properties(pluss_cli PROPERTIES
        OUTPUT_NAME pluss
        PDB_NAME pluss_cli
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
    )
    target_include_directories(pluss_cli PRIVATE ${CMAKE_BINARY_DIR}/generated)
    install(TARGETS pluss_cli RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

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

    add_executable(plusstils_core_info_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/test/core_info_tests.cpp
    )
    target_link_libraries(plusstils_core_info_tests PRIVATE PlussTils)
    target_compile_features(plusstils_core_info_tests PRIVATE cxx_std_20)
    add_test(NAME plusstils_core_info COMMAND plusstils_core_info_tests)

    add_executable(plusstils_core_settings_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/test/core_settings_tests.cpp
    )
    target_link_libraries(plusstils_core_settings_tests PRIVATE PlussTils)
    target_compile_features(plusstils_core_settings_tests PRIVATE cxx_std_20)
    add_test(NAME plusstils_core_settings COMMAND plusstils_core_settings_tests)

    add_executable(plusstils_core_cache_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/test/core_cache_tests.cpp
    )
    target_link_libraries(plusstils_core_cache_tests PRIVATE PlussTils)
    target_compile_features(plusstils_core_cache_tests PRIVATE cxx_std_20)
    add_test(NAME plusstils_core_cache COMMAND plusstils_core_cache_tests)

    add_executable(plusstils_core_doc_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/test/core_doc_tests.cpp
    )
    target_link_libraries(plusstils_core_doc_tests PRIVATE PlussTilsCore)
    target_compile_features(plusstils_core_doc_tests PRIVATE cxx_std_20)
    add_test(NAME plusstils_core_doc COMMAND plusstils_core_doc_tests)

    add_executable(plusstils_scalar_cast_compat_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/test/scalar_cast_compat_tests.cpp
    )
    target_link_libraries(plusstils_scalar_cast_compat_tests PRIVATE PlussTils)
    target_compile_features(plusstils_scalar_cast_compat_tests PRIVATE cxx_std_20)
    add_test(NAME plusstils_scalar_cast_compat COMMAND plusstils_scalar_cast_compat_tests)

    add_executable(plusstils_format_parity_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/test/format_parity_tests.cpp
    )
    target_link_libraries(plusstils_format_parity_tests PRIVATE PlussTils)
    target_compile_features(plusstils_format_parity_tests PRIVATE cxx_std_20)
    add_test(NAME plusstils_format_parity COMMAND plusstils_format_parity_tests)

    add_test(NAME plusstils_format_compile_checks
        COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/test/format_compile_checks.sh
                ${CMAKE_CXX_COMPILER} ${CMAKE_CURRENT_SOURCE_DIR}/include
                ${CMAKE_SOURCE_DIR}/PlussQit/include ${CMAKE_SOURCE_DIR}/PlussQit/src ${CMAKE_BINARY_DIR}/generated
                ${CMAKE_OSX_SYSROOT})
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
