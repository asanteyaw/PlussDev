cmake_minimum_required(VERSION 3.28)

if(EXISTS "${CMAKE_SOURCE_DIR}/cmake/LocalConfig.cmake")
    include("${CMAKE_SOURCE_DIR}/cmake/LocalConfig.cmake")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(LIBZIP QUIET libzip)
endif()
project(plussma VERSION 0.1.0 LANGUAGES CXX)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(CTest)

# PlussQit core numeric/time primitives are foundational for all higher layers
# (including PlussMic). Keep them always enabled at the top-level.
set(PLUSSQIT_BUILD_CODEM ON CACHE BOOL "Build CodeM (required by PlussMa libs)" FORCE)
set(PLUSSQIT_BUILD_CODEH ON CACHE BOOL "Build CodeH (required by PlussMa libs)" FORCE)

# Provide a top-level 'pluss' library so subdirectories can attach sources
add_library(pluss STATIC)
if(EXISTS "${CMAKE_SOURCE_DIR}/include")
    target_include_directories(pluss PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
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
set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type")

add_compile_definitions(_GLIBCXX_USE_CXX11_ABI=0)

# ==============================
# Create libraries
# ==============================

add_subdirectory(PlussAI)
add_subdirectory(PlussQit)
add_subdirectory(PlussView)
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

if(TARGET pluss AND TARGET PlussMic AND TARGET plussqit_codem)
    add_executable(plussma_math_finance_calibrate_heston_demo
        ${CMAKE_SOURCE_DIR}/PlussMa/Math/examples/Finance/calibrate_heston_demo.cpp
    )
    target_compile_features(plussma_math_finance_calibrate_heston_demo PRIVATE cxx_std_20)
    target_link_libraries(plussma_math_finance_calibrate_heston_demo PRIVATE pluss plussqit_codem PlussMic)
    target_compile_definitions(plussma_math_finance_calibrate_heston_demo PRIVATE
        PLUSSMIC_SOURCE_DIR="${CMAKE_SOURCE_DIR}/PlussMic"
    )
    set_target_properties(plussma_math_finance_calibrate_heston_demo PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/PlussMa/Math/examples/Finance"
    )

    add_executable(plussma_math_finance_calibrate_bates_demo
        ${CMAKE_SOURCE_DIR}/PlussMa/Math/examples/Finance/calibrate_bates_demo.cpp
    )
    target_compile_features(plussma_math_finance_calibrate_bates_demo PRIVATE cxx_std_20)
    target_link_libraries(plussma_math_finance_calibrate_bates_demo PRIVATE pluss plussqit_codem PlussMic)
    target_compile_definitions(plussma_math_finance_calibrate_bates_demo PRIVATE
        PLUSSMIC_SOURCE_DIR="${CMAKE_SOURCE_DIR}/PlussMic"
    )
    set_target_properties(plussma_math_finance_calibrate_bates_demo PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/PlussMa/Math/examples/Finance"
    )
endif()

if(TARGET plussview_window_registry_demo AND TARGET PlussMic)
    target_link_libraries(plussview_window_registry_demo PRIVATE PlussMic)
    target_compile_definitions(plussview_window_registry_demo PRIVATE
        PLUSSMIC_SOURCE_DIR="${CMAKE_SOURCE_DIR}/PlussMic"
    )
endif()

# Top-level aggregate target for downstream consumers that want the full stack.
# Use together with `#include <pluss/pluss.h>`.
add_library(pluss_sdk INTERFACE)
add_library(Pluss::SDK ALIAS pluss_sdk)

if(TARGET PlussQit)
    target_link_libraries(pluss_sdk INTERFACE PlussQit)
endif()
if(TARGET PlussPl)
    target_link_libraries(pluss_sdk INTERFACE PlussPl)
endif()
if(TARGET PlussMic)
    target_link_libraries(pluss_sdk INTERFACE PlussMic)
endif()
if(TARGET PlussView)
    target_link_libraries(pluss_sdk INTERFACE PlussView)
endif()
if(TARGET PlussTiq)
    target_link_libraries(pluss_sdk INTERFACE PlussTiq)
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
    DESTINATION ${CMAKE_INSTALL_DOCDIR}
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

set(CPACK_PACKAGE_NAME "pluss-sdk")
set(CPACK_PACKAGE_VENDOR "Pluss")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_FILE_NAME "pluss-sdk-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
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
            -P ${CMAKE_SOURCE_DIR}/cmake/RunSdkConsumerSmoke.cmake
    )
endif()

include(CPack)
