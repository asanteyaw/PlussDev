cmake_minimum_required(VERSION 3.28)

if(EXISTS "${CMAKE_SOURCE_DIR}/cmake/LocalConfig.cmake")
    include("${CMAKE_SOURCE_DIR}/cmake/LocalConfig.cmake")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(LIBZIP QUIET libzip)
endif()

function(pluss_register_sdk_zip_link ZIP_LIBRARY)
    if(TARGET pluss_sdk_zip_link)
        return()
    endif()

    if(NOT ZIP_LIBRARY OR NOT EXISTS "${ZIP_LIBRARY}")
        message(WARNING
            "pluss_register_sdk_zip_link called without a concrete libzip path. "
            "SDK export will not package libzip automatically.")
        return()
    endif()

    add_library(pluss_sdk_zip_link INTERFACE)
    target_link_libraries(pluss_sdk_zip_link INTERFACE
        $<BUILD_INTERFACE:${ZIP_LIBRARY}>
    )

    get_filename_component(_pluss_zip_realpath "${ZIP_LIBRARY}" REALPATH)
    get_filename_component(_pluss_zip_filename "${_pluss_zip_realpath}" NAME)
    install(FILES "${_pluss_zip_realpath}"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
    target_link_libraries(pluss_sdk_zip_link INTERFACE
        $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/${CMAKE_INSTALL_LIBDIR}/${_pluss_zip_filename}>
    )
    install(TARGETS pluss_sdk_zip_link EXPORT PlussTargets)
endfunction()

# Use the compiler shipped with the selected macOS SDK by default. Conda's
# compiler/resource headers and linker may lag behind an Xcode SDK upgrade.
# Explicit CMake compiler settings and CC/CXX environment choices take priority.
if(APPLE
   AND NOT DEFINED CMAKE_C_COMPILER
   AND NOT DEFINED CMAKE_CXX_COMPILER
   AND "$ENV{CC}" STREQUAL ""
   AND "$ENV{CXX}" STREQUAL "")
    set(CMAKE_C_COMPILER "/usr/bin/clang" CACHE FILEPATH "C compiler")
    set(CMAKE_CXX_COMPILER "/usr/bin/clang++" CACHE FILEPATH "CXX compiler")
    message(STATUS "Auto-selected Apple clang toolchain for the macOS SDK")
endif()

set(_pluss_is_conda_clang FALSE)
if(APPLE)
    if(CMAKE_CXX_COMPILER MATCHES ".*/envs/cpp_env/bin/clang\\+\\+$"
       OR CMAKE_CXX_COMPILER MATCHES ".*/envs/cpp_env/bin/arm64-apple-darwin[0-9.]+-clang\\+\\+$")
        set(_pluss_is_conda_clang TRUE)
    endif()
    if(DEFINED ENV{CONDA_PREFIX} AND NOT "$ENV{CONDA_PREFIX}" STREQUAL "")
        if(CMAKE_CXX_COMPILER MATCHES "^$ENV{CONDA_PREFIX}/bin/clang\\+\\+$"
           OR CMAKE_CXX_COMPILER MATCHES "^$ENV{CONDA_PREFIX}/bin/arm64-apple-darwin[0-9.]+-clang\\+\\+$")
            set(_pluss_is_conda_clang TRUE)
        endif()
    endif()
endif()

# CMake's compiler checks and try_compile() projects must use the same native
# link driver as the main build; a top-level link-rule override is not inherited.
if(_pluss_is_conda_clang AND EXISTS "/usr/bin/clang++")
    foreach(_pluss_lang CXX OBJCXX)
        if(NOT DEFINED CMAKE_USER_MAKE_RULES_OVERRIDE_${_pluss_lang})
            set(CMAKE_USER_MAKE_RULES_OVERRIDE_${_pluss_lang}
                "${CMAKE_CURRENT_SOURCE_DIR}/cmake/AppleCondaLinkRules.cmake")
        endif()
    endforeach()
    # Retry failures left in existing build trees by the old Conda link driver.
    if(NOT PLUSS_NATIVE_LINKER_PROBES_RESET STREQUAL "apple-driver-v1")
        foreach(_pluss_probe CMAKE_HAVE_LIBC_PTHREAD CMAKE_HAVE_PTHREADS_CREATE
                             CMAKE_HAVE_PTHREAD_CREATE THREADS_HAVE_PTHREAD_ARG)
            if(DEFINED ${_pluss_probe} AND NOT ${_pluss_probe})
                unset(${_pluss_probe} CACHE)
            endif()
        endforeach()
        set(PLUSS_NATIVE_LINKER_PROBES_RESET "apple-driver-v1" CACHE INTERNAL
            "Retried failed thread probes with the native macOS link driver")
    endif()
    unset(_pluss_lang)
    unset(_pluss_probe)
endif()

project(plussma VERSION 0.1.0 LANGUAGES CXX)

# Keep the explicitly selected Conda compiler usable with the active Xcode SDK.
# This is a compile-toolchain repair, not a public header requirement.
if(_pluss_is_conda_clang)
    add_compile_options(
        "$<$<COMPILE_LANGUAGE:CXX,OBJCXX>:-include${CMAKE_CURRENT_SOURCE_DIR}/cmake/AppleCondaMathCompat.h>"
    )
endif()

if(APPLE)
    enable_language(OBJCXX)
    if(NOT CMAKE_OBJCXX_COMPILE_OBJECT)
        set(CMAKE_OBJCXX_COMPILE_OBJECT
            "<CMAKE_OBJCXX_COMPILER> <DEFINES> <INCLUDES> <FLAGS> -x objective-c++ -o <OBJECT> -c <SOURCE>")
    endif()
endif()
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(CTest)

set(PLUSS_INSTALL_DOCDIR "${CMAKE_INSTALL_DATAROOTDIR}/doc/plussii-cpp"
    CACHE PATH "Documentation directory for the PlussII C++ distribution")

# Conda clang toolchains can fail to link Objective-C/Apple frameworks
# (Foundation/Cocoa/Metal/Accelerate) even when compile succeeds. Keep
# compilation toolchain unchanged, but force Apple clang as the link driver.

if(APPLE
   AND _pluss_is_conda_clang
   AND EXISTS "/usr/bin/clang++")
    message(STATUS "Using /usr/bin/clang++ as link driver for macOS framework parity with Conda clang compile toolchain.")
    set(CMAKE_CXX_LINK_EXECUTABLE
        "/usr/bin/clang++ <FLAGS> <CMAKE_CXX_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")
    if(CMAKE_OBJCXX_COMPILER)
        set(CMAKE_OBJCXX_LINK_EXECUTABLE
            "/usr/bin/clang++ <FLAGS> <CMAKE_OBJCXX_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")
    endif()
endif()
unset(_pluss_is_conda_clang)

# PlussQit core numeric/time primitives are foundational for all higher layers
# (including PlussMic). Keep them always enabled at the top-level.
set(PLUSSQIT_BUILD_CODEM ON CACHE BOOL "Build CodeM (required by PlussMa libs)" FORCE)
set(PLUSSQIT_BUILD_CODEH ON CACHE BOOL "Build CodeH (required by PlussMa libs)" FORCE)

# Provide a top-level 'pluss' library so subdirectories can attach sources
add_library(pluss STATIC)
target_include_directories(pluss PUBLIC
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/generated>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/PlussMic/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
if(EXISTS "${CMAKE_SOURCE_DIR}/include")
    target_include_directories(pluss PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    )
endif()
set_target_properties(pluss PROPERTIES CXX_STANDARD 20)

# Provide an alias so older CMakeLists that expect a `PlussQit` target still work
add_library(PlussQit ALIAS pluss)

# Define a simple EXCLUDE macro expected by some internal CMakeLists
# Removes items in ARGN from the list named by VAR
macro(EXCLUDE VAR)
    foreach(_itm IN LISTS ARGN)
        list(REMOVE_ITEM ${VAR} ${_itm})
    endforeach()
endmacro()

# Minimal append_filelist used by internal CMakeLists (stub for configure)
macro(append_filelist NAME VAR)
    set(_cur ${${NAME}})
    set(_add ${${VAR}})
    foreach(_i IN LISTS _add)
        list(APPEND _cur ${_i})
    endforeach()
    set(${NAME} ${_cur} PARENT_SCOPE)
endmacro()

# lowercase alias used in some files
macro(exclude) 
    _EXCLUDE_CALL(${ARGV0})
endmacro()

macro(_EXCLUDE_CALL VAR)
    foreach(_itm IN LISTS ARGN)
        list(REMOVE_ITEM ${VAR} ${_itm})
    endforeach()
endmacro()

# ==============================
# Global configuration
# ==============================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Single-config generators (Ninja/Unix Makefiles): default to RelWithDebInfo
# so users get optimized binaries with useful file/line stack traces.
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo" "MinSizeRel")

option(
    PLUSS_TRACE_FRIENDLY_RELEASE
    "Keep frame pointers and minimal line tables in Release builds for readable stack traces"
    ON
)

if(PLUSS_TRACE_FRIENDLY_RELEASE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "AppleClang|Clang|GNU")
        add_compile_options($<$<OR:$<COMPILE_LANGUAGE:C>,$<COMPILE_LANGUAGE:CXX>>:-fno-omit-frame-pointer>)
        if(CMAKE_CXX_COMPILER_ID MATCHES "AppleClang|Clang")
            add_compile_options($<$<AND:$<CONFIG:Release>,$<OR:$<COMPILE_LANGUAGE:C>,$<COMPILE_LANGUAGE:CXX>>>:-gline-tables-only>)
        elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            add_compile_options($<$<AND:$<CONFIG:Release>,$<OR:$<COMPILE_LANGUAGE:C>,$<COMPILE_LANGUAGE:CXX>>>:-g1>)
        endif()
    endif()
endif()

option(
    PLUSS_FORCE_GLIBCXX_ABI0
    "Force legacy libstdc++ ABI (_GLIBCXX_USE_CXX11_ABI=0) for old prebuilt binary compatibility"
    OFF
)
if(PLUSS_FORCE_GLIBCXX_ABI0)
    add_compile_definitions(_GLIBCXX_USE_CXX11_ABI=0)
    message(STATUS "PLUSS forcing legacy libstdc++ ABI (_GLIBCXX_USE_CXX11_ABI=0)")
endif()

# Optional cuDNN wiring for CUDA hosts (cluster-friendly defaults, fully overrideable).
set(PLUSS_CUDNN_ROOT "" CACHE PATH "Root directory for cuDNN (contains include/ and lib or lib64)")
set(PLUSS_CUDNN_LIBRARY "" CACHE FILEPATH "Absolute path to libcudnn shared library")

if(NOT PLUSS_CUDNN_ROOT)
    foreach(_pluss_cudnn_candidate
            "$ENV{PLUSS_CUDNN_ROOT}"
            "$ENV{CUDNN_ROOT}"
            "$ENV{CUDNN_PATH}"
            "/global/software/cudnn/9.7.1-cuda12")
        if(_pluss_cudnn_candidate AND EXISTS "${_pluss_cudnn_candidate}/include")
            set(PLUSS_CUDNN_ROOT "${_pluss_cudnn_candidate}" CACHE PATH "Root directory for cuDNN (contains include/ and lib or lib64)" FORCE)
            break()
        endif()
    endforeach()
endif()

set(PLUSS_CUDNN_INCLUDE_DIR "")
set(PLUSS_CUDNN_LIBRARY_DIR "")
if(PLUSS_CUDNN_ROOT)
    if(EXISTS "${PLUSS_CUDNN_ROOT}/include")
        set(PLUSS_CUDNN_INCLUDE_DIR "${PLUSS_CUDNN_ROOT}/include")
    endif()
    if(EXISTS "${PLUSS_CUDNN_ROOT}/lib64")
        set(PLUSS_CUDNN_LIBRARY_DIR "${PLUSS_CUDNN_ROOT}/lib64")
    elseif(EXISTS "${PLUSS_CUDNN_ROOT}/lib")
        set(PLUSS_CUDNN_LIBRARY_DIR "${PLUSS_CUDNN_ROOT}/lib")
    endif()
endif()

if(NOT PLUSS_CUDNN_LIBRARY)
    set(_pluss_cudnn_hint_paths "")
    if(PLUSS_CUDNN_LIBRARY_DIR)
        list(APPEND _pluss_cudnn_hint_paths "${PLUSS_CUDNN_LIBRARY_DIR}")
    endif()
    find_library(_pluss_cudnn_library
        NAMES cudnn libcudnn.so.9 libcudnn.so.8
        HINTS ${_pluss_cudnn_hint_paths}
    )
    if(_pluss_cudnn_library)
        set(PLUSS_CUDNN_LIBRARY "${_pluss_cudnn_library}" CACHE FILEPATH "Absolute path to libcudnn shared library" FORCE)
    endif()
endif()
unset(_pluss_cudnn_library CACHE)

if(PLUSS_CUDNN_LIBRARY AND NOT PLUSS_CUDNN_LIBRARY_DIR)
    get_filename_component(PLUSS_CUDNN_LIBRARY_DIR "${PLUSS_CUDNN_LIBRARY}" DIRECTORY)
endif()

if(PLUSS_CUDNN_LIBRARY_DIR)
    list(APPEND CMAKE_BUILD_RPATH "${PLUSS_CUDNN_LIBRARY_DIR}")
    list(REMOVE_DUPLICATES CMAKE_BUILD_RPATH)
endif()

if(PLUSS_CUDNN_ROOT)
    message(STATUS "PLUSS cuDNN root: ${PLUSS_CUDNN_ROOT}")
endif()
if(PLUSS_CUDNN_INCLUDE_DIR)
    message(STATUS "PLUSS cuDNN include: ${PLUSS_CUDNN_INCLUDE_DIR}")
endif()
if(PLUSS_CUDNN_LIBRARY)
    message(STATUS "PLUSS cuDNN library: ${PLUSS_CUDNN_LIBRARY}")
endif()

# ==============================
# Create libraries
# ==============================

add_subdirectory(PlussAI)
add_subdirectory(PlussQit)
add_subdirectory(PlussTils)
if(NOT TARGET plussqit_coden)
    message(FATAL_ERROR "plussqit_coden (CodeN) must be built for PlussMa.")
endif()
if(NOT TARGET plussqit_codem)
    message(FATAL_ERROR "plussqit_codem (CodeM) must be built for PlussMa.")
endif()
if(NOT TARGET plussqit_codeh)
    message(FATAL_ERROR "plussqit_codeh (CodeH) must be built for PlussMa.")
endif()
target_link_libraries(pluss PUBLIC
    pluss::plussqit_codem
    pluss::plussqit_coden
    pluss::plussqit_codeh
)
add_subdirectory(PlussPl)
add_subdirectory(PlussMa)
add_subdirectory(PlussMic)

if(TARGET pluss AND PLUSS_CUDNN_INCLUDE_DIR)
    target_include_directories(pluss PRIVATE "${PLUSS_CUDNN_INCLUDE_DIR}")
endif()
if(TARGET pluss AND PLUSS_CUDNN_LIBRARY)
    target_compile_definitions(pluss PRIVATE PLUSS_CUDNN_LIBRARY_DEFAULT="${PLUSS_CUDNN_LIBRARY}")
endif()
if(TARGET plussqit_codem AND PLUSS_CUDNN_LIBRARY)
    target_compile_definitions(plussqit_codem PRIVATE PLUSS_CUDNN_LIBRARY_DEFAULT="${PLUSS_CUDNN_LIBRARY}")
endif()

if(TARGET plusstils_view_window_registry_demo AND TARGET PlussMic)
    target_link_libraries(plusstils_view_window_registry_demo PRIVATE PlussMic)
    target_compile_definitions(plusstils_view_window_registry_demo PRIVATE
        PLUSSMIC_SOURCE_DIR="${CMAKE_SOURCE_DIR}/PlussMic"
    )
endif()
if(TARGET plusstils_view_plussmic_inhouse_showcase AND TARGET PlussMic)
    target_link_libraries(plusstils_view_plussmic_inhouse_showcase PRIVATE PlussMic)
endif()

# Top-level aggregate target for downstream consumers that want the full stack.
# Use together with `#include <pluss/pluss.h>`.
add_library(pluss_sdk INTERFACE)
add_library(Pluss::SDK ALIAS pluss_sdk)
add_library(Pluss::PlussII ALIAS pluss_sdk)

if(TARGET PlussQit)
    target_link_libraries(pluss_sdk INTERFACE PlussQit)
endif()
if(TARGET PlussPl)
    target_link_libraries(pluss_sdk INTERFACE PlussPl)
endif()
if(TARGET PlussMic)
    target_link_libraries(pluss_sdk INTERFACE PlussMic)
endif()
if(TARGET PlussTils)
    target_link_libraries(pluss_sdk INTERFACE PlussTils)
endif()
if(TARGET PlussTiq)
    target_link_libraries(pluss_sdk INTERFACE PlussTiq)
endif()

# Propagate traceback-friendly flags to SDK consumers so user-code frames are
# visible without requiring manual build-flag tuning in downstream projects.
if(PLUSS_TRACE_FRIENDLY_RELEASE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "AppleClang|Clang|GNU")
        target_compile_options(pluss_sdk INTERFACE
            $<$<OR:$<COMPILE_LANGUAGE:C>,$<COMPILE_LANGUAGE:CXX>>:-fno-omit-frame-pointer>
        )
        if(CMAKE_CXX_COMPILER_ID MATCHES "AppleClang|Clang")
            target_compile_options(pluss_sdk INTERFACE
                $<$<AND:$<CONFIG:Release>,$<OR:$<COMPILE_LANGUAGE:C>,$<COMPILE_LANGUAGE:CXX>>>:-gline-tables-only>
            )
        elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            target_compile_options(pluss_sdk INTERFACE
                $<$<AND:$<CONFIG:Release>,$<OR:$<COMPILE_LANGUAGE:C>,$<COMPILE_LANGUAGE:CXX>>>:-g1>
            )
        endif()
    endif()
endif()

install(TARGETS pluss
    EXPORT PlussTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/PlussAI/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "*.hh"
    PATTERN "pluss/ai/lymph/quick.h" EXCLUDE
    PATTERN ".DS_Store" EXCLUDE
)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/PlussAI/NodeK/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/NodeK
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "*.hh"
    PATTERN ".DS_Store" EXCLUDE
)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/PlussMa/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "*.hh"
    PATTERN ".DS_Store" EXCLUDE
)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/PlussMic/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "*.hh"
    PATTERN ".DS_Store" EXCLUDE
)

# PlussMic public umbrellas include legacy top-level paths like
# "bundle/Bundle.h", "loader/Loader.h", and "table/Table.h".
# Stage those header trees so <pluss/pluss.h> remains self-contained for SDK users.
install(DIRECTORY ${CMAKE_SOURCE_DIR}/PlussMic/src/bundle/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/bundle
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "*.hh"
    PATTERN ".DS_Store" EXCLUDE
)

# Bundle and kindle expose relationship-aware operations whose declarations
# share the relational contracts. Keep that public include dependency intact
# in staged SDKs just as the neighboring PlussMic header trees are kept intact.
install(DIRECTORY ${CMAKE_SOURCE_DIR}/PlussMic/src/relational/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/relational
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "*.hh"
    PATTERN ".DS_Store" EXCLUDE
)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/PlussMic/src/loader/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/loader
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "*.hh"
    PATTERN ".DS_Store" EXCLUDE
)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/PlussMic/src/table/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/table
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "*.hh"
    PATTERN ".DS_Store" EXCLUDE
)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/PlussMic/src/kindle/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/kindle
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "*.hh"
    PATTERN ".DS_Store" EXCLUDE
)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/PlussMic/src/scribble/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/scribble
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "*.hh"
    PATTERN ".DS_Store" EXCLUDE
)

# bundle/ and kindle/ headers (Axel.h, registry.h, Archive.h, catalog.h) include
# "common/StringViewNameRange.h" by its quoted include-tree path. Stage it
# alongside its sibling header trees so those #includes resolve for SDK
# consumers, the same way bundle/loader/table/kindle already do.
install(DIRECTORY ${CMAKE_SOURCE_DIR}/PlussMic/src/common/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/common
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "*.hh"
    PATTERN ".DS_Store" EXCLUDE
)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/PlussMa/Math/Numerical/optimization/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/PlussMa/Math/Numerical/optimization
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "*.hh"
    PATTERN ".DS_Store" EXCLUDE
    PATTERN "docs" EXCLUDE
    PATTERN "test" EXCLUDE
)

install(FILES
    ${CMAKE_SOURCE_DIR}/docs/sdk_install.md
    DESTINATION ${PLUSS_INSTALL_DOCDIR}
    RENAME installation.md
)

install(FILES
    ${CMAKE_SOURCE_DIR}/LICENSE
    ${CMAKE_SOURCE_DIR}/LICENSE_PYTORCH
    ${CMAKE_SOURCE_DIR}/NOTICE.md
    DESTINATION ${PLUSS_INSTALL_DOCDIR}
)

install(FILES
    ${CMAKE_SOURCE_DIR}/ExLibs/fmt/LICENSE
    DESTINATION ${PLUSS_INSTALL_DOCDIR}/third_party/fmt
)

configure_package_config_file(
    ${CMAKE_SOURCE_DIR}/cmake/PlussConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/PlussConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Pluss
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/PlussConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(EXPORT PlussTargets
    NAMESPACE Pluss::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Pluss
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/PlussConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/PlussConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Pluss
)

set(CPACK_PACKAGE_NAME "plussii-cpp")
set(CPACK_PACKAGE_VENDOR "PlussII")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_FILE_NAME "plussii-cpp-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
set(CPACK_GENERATOR "TGZ;ZIP")

set(PLUSS_SDK_STAGING_DIR
    "${CMAKE_BINARY_DIR}/sdk/${CPACK_PACKAGE_FILE_NAME}"
    CACHE PATH "Directory used by the pluss_sdk_stage target"
)

set(PLUSS_SDK_CONSUMER_SOURCE_DIR
    "${CMAKE_SOURCE_DIR}/examples/sdk_consumer"
    CACHE PATH "Source directory for the standalone SDK consumer smoke test"
)

set(PLUSS_SDK_CONSUMER_BUILD_DIR
    "${CMAKE_BINARY_DIR}/sdk_consumer_smoke"
    CACHE PATH "Build directory used by the standalone SDK consumer smoke test"
)

add_custom_target(pluss_sdk_stage
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${PLUSS_SDK_STAGING_DIR}"
    COMMAND ${CMAKE_COMMAND} --install "${CMAKE_BINARY_DIR}" --prefix "${PLUSS_SDK_STAGING_DIR}"
    DEPENDS
        pluss
        PlussPl
        PlussMic
        PlussTils
    USES_TERMINAL
    COMMENT "Staging pluss SDK at ${PLUSS_SDK_STAGING_DIR}"
)

add_custom_target(pluss_sdk_archive
    COMMAND ${CMAKE_COMMAND} --build "${CMAKE_BINARY_DIR}" --target pluss_sdk_stage -j1
    COMMAND ${CMAKE_COMMAND} -E rm -f "${CMAKE_BINARY_DIR}/${CPACK_PACKAGE_FILE_NAME}.zip"
    COMMAND ${CMAKE_COMMAND} -E rm -f "${CMAKE_BINARY_DIR}/${CPACK_PACKAGE_FILE_NAME}.tar.gz"
    COMMAND ${CMAKE_COMMAND} -E chdir "${CMAKE_BINARY_DIR}/sdk"
            zip -qry "${CMAKE_BINARY_DIR}/${CPACK_PACKAGE_FILE_NAME}.zip" "${CPACK_PACKAGE_FILE_NAME}"
    COMMAND ${CMAKE_COMMAND} -E chdir "${CMAKE_BINARY_DIR}/sdk"
            ${CMAKE_COMMAND} -E tar "cfz" "${CMAKE_BINARY_DIR}/${CPACK_PACKAGE_FILE_NAME}.tar.gz" "${CPACK_PACKAGE_FILE_NAME}"
    DEPENDS pluss_sdk_stage
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    USES_TERMINAL
    COMMENT "Creating pluss SDK archives"
)

add_custom_target(pluss_sdk_consumer_smoke
    COMMAND ${CMAKE_COMMAND}
        -DTOP_LEVEL_BUILD_DIR=${CMAKE_BINARY_DIR}
        -DSDK_STAGE_DIR=${PLUSS_SDK_STAGING_DIR}
        -DSDK_CONSUMER_SOURCE_DIR=${PLUSS_SDK_CONSUMER_SOURCE_DIR}
        -DSDK_CONSUMER_BUILD_DIR=${PLUSS_SDK_CONSUMER_BUILD_DIR}
        -DArrow_DIR=${Arrow_DIR}
        -DParquet_DIR=${Parquet_DIR}
        -DTorch_DIR=${Torch_DIR}
        -P ${CMAKE_SOURCE_DIR}/cmake/RunSdkConsumerSmoke.cmake
    DEPENDS pluss
    USES_TERMINAL
    COMMENT "Building and running the standalone SDK consumer smoke test"
)

add_custom_target(alias_lint
    COMMAND ${CMAKE_COMMAND} -E env python3 ${CMAKE_SOURCE_DIR}/tools/lint_alias_registry.py
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    USES_TERMINAL
    COMMENT "Validating alias registry consistency and collisions"
)

if(BUILD_TESTING)
    add_test(
        NAME alias_registry_lint
        COMMAND ${CMAKE_COMMAND} -E env python3 ${CMAKE_SOURCE_DIR}/tools/lint_alias_registry.py
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )

    add_test(
        NAME pluss_sdk_consumer_smoke
        COMMAND ${CMAKE_COMMAND}
            -DTOP_LEVEL_BUILD_DIR=${CMAKE_BINARY_DIR}
            -DSDK_STAGE_DIR=${PLUSS_SDK_STAGING_DIR}
            -DSDK_CONSUMER_SOURCE_DIR=${PLUSS_SDK_CONSUMER_SOURCE_DIR}
            -DSDK_CONSUMER_BUILD_DIR=${PLUSS_SDK_CONSUMER_BUILD_DIR}
            -DArrow_DIR=${Arrow_DIR}
            -DParquet_DIR=${Parquet_DIR}
            -DTorch_DIR=${Torch_DIR}
            -P ${CMAKE_SOURCE_DIR}/cmake/RunSdkConsumerSmoke.cmake
    )
endif()

include(CPack)
