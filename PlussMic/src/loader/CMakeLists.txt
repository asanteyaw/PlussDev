cmake_minimum_required(VERSION 3.28)
project(PlussMic_loader NONE)
if(POLICY CMP0079)
  cmake_policy(SET CMP0079 NEW)
endif()

target_sources(PlussMic PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/Config.h
  ${CMAKE_CURRENT_SOURCE_DIR}/Loader.h
  ${CMAKE_CURRENT_SOURCE_DIR}/Loader.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/SourceResolver.h
  ${CMAKE_CURRENT_SOURCE_DIR}/SourceResolver.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/CsvLoader.h
  ${CMAKE_CURRENT_SOURCE_DIR}/CsvLoader.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/parallel_csv_reader.h
  ${CMAKE_CURRENT_SOURCE_DIR}/parallel_csv_reader.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ExcelLoader.h
  ${CMAKE_CURRENT_SOURCE_DIR}/ExcelLoader.cpp
)

# Release builds set these so a missing Excel or Parquet backend stops the
# configuration instead of silently producing a library without that support.
option(PLUSSMIC_REQUIRE_EXCEL "Fail configuration if Excel (libzip) support cannot be built" OFF)
option(PLUSSMIC_REQUIRE_PARQUET "Fail configuration if Parquet (Arrow + Parquet C++) support cannot be built" OFF)
# Link libzip, Arrow and Parquet statically and carry them inside the installed
# distribution, so consumers install nothing for Excel or Parquet.
option(PLUSSMIC_BUNDLE_NATIVE_DEPS "Bundle static libzip/Arrow/Parquet into the installed distribution" OFF)

set(PLUSSMIC_NATIVE_BUNDLE_TARGETS "")

# Prefer libzip discovered at the top-level via pkg-config, then fall back to
# direct lookup with conda-friendly hints.
set(PLUSSMIC_LIBZIP_INCLUDE_DIR "")
set(PLUSSMIC_LIBZIP_LIBRARY "")
set(PLUSSMIC_LIBZIP_TARGET "")

if(PLUSSMIC_BUNDLE_NATIVE_DEPS)
  find_package(libzip CONFIG QUIET)
  if(TARGET libzip::zip)
    set(PLUSSMIC_LIBZIP_TARGET libzip::zip)
  endif()
endif()

if(DEFINED LIBZIP_INCLUDE_DIRS AND LIBZIP_INCLUDE_DIRS)
  list(GET LIBZIP_INCLUDE_DIRS 0 PLUSSMIC_LIBZIP_INCLUDE_DIR)
endif()

if(DEFINED pkgcfg_lib_LIBZIP_zip AND EXISTS "${pkgcfg_lib_LIBZIP_zip}")
  set(PLUSSMIC_LIBZIP_LIBRARY "${pkgcfg_lib_LIBZIP_zip}")
endif()

if(NOT PLUSSMIC_LIBZIP_INCLUDE_DIR)
  find_path(PLUSSMIC_LIBZIP_INCLUDE_DIR zip.h
    HINTS
      ${LIBZIP_INCLUDE_DIRS}
      $ENV{CONDA_PREFIX}/include
  )
endif()

if(NOT PLUSSMIC_LIBZIP_LIBRARY)
  find_library(PLUSSMIC_LIBZIP_LIBRARY
    NAMES zip libzip
    HINTS
      ${LIBZIP_LIBRARY_DIRS}
      $ENV{CONDA_PREFIX}/lib
  )
endif()
if(PLUSSMIC_LIBZIP_TARGET)
  # Linked through plussmic_native_link below so the export carries the bundle.
  list(APPEND PLUSSMIC_NATIVE_BUNDLE_TARGETS ${PLUSSMIC_LIBZIP_TARGET})
  target_link_libraries(PlussMic PRIVATE $<BUILD_INTERFACE:${PLUSSMIC_LIBZIP_TARGET}>)
  target_compile_definitions(PlussMic PRIVATE PLUSSMIC_HAS_LIBZIP=1)
elseif(PLUSSMIC_BUNDLE_NATIVE_DEPS)
  target_compile_definitions(PlussMic PRIVATE PLUSSMIC_HAS_LIBZIP=0)
elseif(PLUSSMIC_LIBZIP_INCLUDE_DIR AND PLUSSMIC_LIBZIP_LIBRARY)
  target_include_directories(PlussMic PRIVATE ${PLUSSMIC_LIBZIP_INCLUDE_DIR})
  pluss_register_sdk_zip_link("${PLUSSMIC_LIBZIP_LIBRARY}")
  if(TARGET pluss_sdk_zip_link)
    target_link_libraries(PlussMic PUBLIC pluss_sdk_zip_link)
  else()
    target_link_libraries(PlussMic PRIVATE ${PLUSSMIC_LIBZIP_LIBRARY})
  endif()
  target_compile_definitions(PlussMic PRIVATE PLUSSMIC_HAS_LIBZIP=1)
else()
  target_compile_definitions(PlussMic PRIVATE PLUSSMIC_HAS_LIBZIP=0)
endif()
if(PLUSSMIC_REQUIRE_EXCEL AND NOT PLUSSMIC_LIBZIP_TARGET
   AND NOT (PLUSSMIC_LIBZIP_INCLUDE_DIR AND PLUSSMIC_LIBZIP_LIBRARY))
  message(FATAL_ERROR "PlussMic Excel support requires libzip, but it was not found")
endif()

# Optional native parquet-cpp support (Apache Arrow + Parquet C++).
set(PLUSSMIC_PARQUET_TARGET "")
set(PLUSSMIC_ARROW_TARGET "")

find_package(Arrow CONFIG QUIET)
find_package(Parquet CONFIG QUIET)

if(PLUSSMIC_BUNDLE_NATIVE_DEPS)
  # A bundled distribution links Arrow and Parquet statically.
  if(TARGET Arrow::arrow_static)
    set(PLUSSMIC_ARROW_TARGET Arrow::arrow_static)
  endif()
  if(TARGET Parquet::parquet_static)
    set(PLUSSMIC_PARQUET_TARGET Parquet::parquet_static)
  endif()
  if(NOT PLUSSMIC_ARROW_TARGET AND TARGET Arrow::arrow)
    set(PLUSSMIC_ARROW_TARGET Arrow::arrow)
  endif()
  if(NOT PLUSSMIC_PARQUET_TARGET AND TARGET Parquet::parquet)
    set(PLUSSMIC_PARQUET_TARGET Parquet::parquet)
  endif()
elseif(TARGET Arrow::arrow)
  set(PLUSSMIC_ARROW_TARGET Arrow::arrow)
elseif(TARGET Arrow::arrow_shared)
  set(PLUSSMIC_ARROW_TARGET Arrow::arrow_shared)
endif()

if(NOT PLUSSMIC_BUNDLE_NATIVE_DEPS)
  if(TARGET Parquet::parquet)
    set(PLUSSMIC_PARQUET_TARGET Parquet::parquet)
  elseif(TARGET Parquet::parquet_shared)
    set(PLUSSMIC_PARQUET_TARGET Parquet::parquet_shared)
  endif()
endif()

if(PLUSSMIC_ARROW_TARGET AND PLUSSMIC_PARQUET_TARGET)
  if(PLUSSMIC_BUNDLE_NATIVE_DEPS)
    target_link_libraries(PlussMic PRIVATE
      $<BUILD_INTERFACE:${PLUSSMIC_PARQUET_TARGET}> $<BUILD_INTERFACE:${PLUSSMIC_ARROW_TARGET}>)
    list(APPEND PLUSSMIC_NATIVE_BUNDLE_TARGETS ${PLUSSMIC_PARQUET_TARGET} ${PLUSSMIC_ARROW_TARGET})
  else()
    target_link_libraries(PlussMic PRIVATE ${PLUSSMIC_ARROW_TARGET} ${PLUSSMIC_PARQUET_TARGET})
  endif()
  target_compile_definitions(PlussMic PRIVATE PLUSSMIC_HAS_PARQUET_CPP=1)
else()
  target_compile_definitions(PlussMic PRIVATE PLUSSMIC_HAS_PARQUET_CPP=0)
endif()
if(PLUSSMIC_REQUIRE_PARQUET AND NOT (PLUSSMIC_ARROW_TARGET AND PLUSSMIC_PARQUET_TARGET))
  message(FATAL_ERROR "PlussMic Parquet support requires Arrow and Parquet C++, but their CMake targets were not found")
endif()

# Optional URL loading support for CSV/TXT/Excel loaders.
option(PLUSSMIC_FETCH_CURL "Allow CMake to FetchContent libcurl when unavailable" ON)
option(PLUSSMIC_REQUIRE_CURL "Fail configuration if URL loading cannot be built" OFF)

find_package(CURL QUIET)
set(PLUSSMIC_CURL_TARGET "")

if(TARGET CURL::libcurl)
  set(PLUSSMIC_CURL_TARGET CURL::libcurl)
elseif(TARGET libcurl)
  set(PLUSSMIC_CURL_TARGET libcurl)
elseif(TARGET libcurl_static)
  set(PLUSSMIC_CURL_TARGET libcurl_static)
elseif(TARGET libcurl_shared)
  set(PLUSSMIC_CURL_TARGET libcurl_shared)
endif()

if(NOT PLUSSMIC_CURL_TARGET AND PLUSSMIC_FETCH_CURL)
  include(FetchContent)
  set(BUILD_CURL_EXE OFF CACHE BOOL "Disable curl CLI when fetching libcurl" FORCE)
  set(BUILD_LIBCURL_DOCS OFF CACHE BOOL "Disable libcurl docs when fetching libcurl" FORCE)
  set(BUILD_MISC_DOCS OFF CACHE BOOL "Disable misc docs when fetching libcurl" FORCE)
  set(ENABLE_CURL_MANUAL OFF CACHE BOOL "Disable curl manpage generation when fetching libcurl" FORCE)

  FetchContent_Declare(
    curl
    GIT_REPOSITORY https://github.com/curl/curl.git
    GIT_TAG curl-8_8_0
  )
  FetchContent_MakeAvailable(curl)

  if(TARGET CURL::libcurl)
    set(PLUSSMIC_CURL_TARGET CURL::libcurl)
  elseif(TARGET libcurl)
    set(PLUSSMIC_CURL_TARGET libcurl)
  elseif(TARGET libcurl_static)
    set(PLUSSMIC_CURL_TARGET libcurl_static)
  elseif(TARGET libcurl_shared)
    set(PLUSSMIC_CURL_TARGET libcurl_shared)
  endif()
endif()

if(PLUSSMIC_REQUIRE_CURL AND NOT PLUSSMIC_CURL_TARGET)
  message(FATAL_ERROR "PlussMic URL loading requires libcurl, but CURL::libcurl was not found")
endif()

# Windows has no system libcurl for a downloaded archive to find, so a static curl (the vcpkg static triplet the
# release uses) goes into the native bundle with the other static libraries; PlussConfig.cmake then provides
# CURL::libcurl from the bundle. Elsewhere curl is resolved on the consumer's machine, as before.
if(WIN32 AND PLUSSMIC_BUNDLE_NATIVE_DEPS AND PLUSSMIC_CURL_TARGET)
  get_target_property(_plussmic_curl_alias ${PLUSSMIC_CURL_TARGET} ALIASED_TARGET)
  if(_plussmic_curl_alias)
    set(_plussmic_curl_bundle_target ${_plussmic_curl_alias})
  else()
    set(_plussmic_curl_bundle_target ${PLUSSMIC_CURL_TARGET})
  endif()
  get_target_property(_plussmic_curl_bundle_type ${_plussmic_curl_bundle_target} TYPE)
  if(_plussmic_curl_bundle_type STREQUAL "STATIC_LIBRARY" OR _plussmic_curl_bundle_type STREQUAL "UNKNOWN_LIBRARY")
    list(APPEND PLUSSMIC_NATIVE_BUNDLE_TARGETS ${_plussmic_curl_bundle_target})
  endif()
endif()

# Carry the static native libraries inside the distribution.
if(PLUSSMIC_BUNDLE_NATIVE_DEPS AND PLUSSMIC_NATIVE_BUNDLE_TARGETS)
  include("${CMAKE_SOURCE_DIR}/cmake/PlussNativeBundle.cmake")
  pluss_native_bundle(NativeBundle TARGETS ${PLUSSMIC_NATIVE_BUNDLE_TARGETS})
  add_library(plussmic_native_link INTERFACE)
  target_link_libraries(plussmic_native_link INTERFACE $<INSTALL_INTERFACE:Pluss::NativeBundle>)
  install(TARGETS plussmic_native_link EXPORT PlussTargets)
  target_link_libraries(PlussMic PUBLIC plussmic_native_link)
endif()

if(PLUSSMIC_CURL_TARGET)
  target_compile_definitions(PlussMic PRIVATE PLUSSMIC_HAS_CURL=1)

  # Export a stable PlussII curl link surface so downstream consumers do not
  # see raw unresolved "CURL::libcurl" tokens at link time.
  add_library(plussmic_curl_link INTERFACE)
  set(_plussmic_curl_build_link "${PLUSSMIC_CURL_TARGET}")
  if(APPLE AND CURL_INCLUDE_DIR STREQUAL "${CMAKE_OSX_SYSROOT}/usr/include")
    # SDK headers are supplied by -isysroot. Passing this directory through
    # curl's imported target would put C headers ahead of libc++ wrappers.
    set(_plussmic_curl_build_link "$<LINK_ONLY:${PLUSSMIC_CURL_TARGET}>")
  endif()
  target_link_libraries(plussmic_curl_link INTERFACE
    $<BUILD_INTERFACE:${_plussmic_curl_build_link}>
  )

  set(_plussmic_curl_install_link "")

  # Export a logical dependency. A downloaded package must resolve curl on the
  # consumer machine rather than copy a library built for the runner's system.
  get_target_property(_plussmic_curl_aliased_target ${PLUSSMIC_CURL_TARGET} ALIASED_TARGET)
  if(_plussmic_curl_aliased_target)
    set(_plussmic_curl_real_target ${_plussmic_curl_aliased_target})
  else()
    set(_plussmic_curl_real_target ${PLUSSMIC_CURL_TARGET})
  endif()
  get_target_property(_plussmic_curl_imported ${_plussmic_curl_real_target} IMPORTED)
  if(_plussmic_curl_imported)
    set(_plussmic_curl_install_link $<INSTALL_INTERFACE:CURL::libcurl>)
  else()
    install(TARGETS ${_plussmic_curl_real_target}
      EXPORT PlussTargets
      ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
      LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
    set(_plussmic_curl_install_link
      $<INSTALL_INTERFACE:Pluss::${_plussmic_curl_real_target}>
    )
  endif()

  if(_plussmic_curl_install_link)
    target_link_libraries(plussmic_curl_link INTERFACE ${_plussmic_curl_install_link})
  endif()

  target_link_libraries(PlussMic PUBLIC plussmic_curl_link)
  install(TARGETS plussmic_curl_link EXPORT PlussTargets)
else()
  message(STATUS "PlussMic loader: URL loading disabled (libcurl not found and fetch unavailable).")
  target_compile_definitions(PlussMic PRIVATE PLUSSMIC_HAS_CURL=0)
endif()
