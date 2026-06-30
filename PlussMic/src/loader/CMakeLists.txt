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

# Prefer libzip discovered at the top-level via pkg-config, then fall back to
# direct lookup with conda-friendly hints.
set(PLUSSMIC_LIBZIP_INCLUDE_DIR "")
set(PLUSSMIC_LIBZIP_LIBRARY "")

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
if(PLUSSMIC_LIBZIP_INCLUDE_DIR AND PLUSSMIC_LIBZIP_LIBRARY)
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

# Optional native parquet-cpp support (Apache Arrow + Parquet C++).
set(PLUSSMIC_PARQUET_TARGET "")
set(PLUSSMIC_ARROW_TARGET "")

find_package(Arrow CONFIG QUIET)
find_package(Parquet CONFIG QUIET)

if(TARGET Arrow::arrow)
  set(PLUSSMIC_ARROW_TARGET Arrow::arrow)
elseif(TARGET Arrow::arrow_shared)
  set(PLUSSMIC_ARROW_TARGET Arrow::arrow_shared)
endif()

if(TARGET Parquet::parquet)
  set(PLUSSMIC_PARQUET_TARGET Parquet::parquet)
elseif(TARGET Parquet::parquet_shared)
  set(PLUSSMIC_PARQUET_TARGET Parquet::parquet_shared)
endif()

if(PLUSSMIC_ARROW_TARGET AND PLUSSMIC_PARQUET_TARGET)
  target_link_libraries(PlussMic PRIVATE ${PLUSSMIC_ARROW_TARGET} ${PLUSSMIC_PARQUET_TARGET})
  target_compile_definitions(PlussMic PRIVATE PLUSSMIC_HAS_PARQUET_CPP=1)
else()
  target_compile_definitions(PlussMic PRIVATE PLUSSMIC_HAS_PARQUET_CPP=0)
endif()

# Optional URL loading support for CSV/TXT/Excel loaders.
option(PLUSSMIC_FETCH_CURL "Allow CMake to FetchContent libcurl when unavailable" ON)

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

if(PLUSSMIC_CURL_TARGET)
  target_compile_definitions(PlussMic PRIVATE PLUSSMIC_HAS_CURL=1)

  # Export a stable SDK-owned curl link surface so downstream consumers do not
  # see raw unresolved "CURL::libcurl" tokens at link time.
  add_library(plussmic_curl_link INTERFACE)
  target_link_libraries(plussmic_curl_link INTERFACE
    $<BUILD_INTERFACE:${PLUSSMIC_CURL_TARGET}>
  )

  set(_plussmic_curl_install_link "")

  # If curl came from FetchContent (non-imported target), install/export it with
  # the SDK. If curl came from find_package(CURL), copy the resolved artifact
  # into the SDK and link to that installed file in install interface.
  get_target_property(_plussmic_curl_imported ${PLUSSMIC_CURL_TARGET} IMPORTED)
  if(_plussmic_curl_imported)
    set(_plussmic_curl_location "")
    foreach(_plussmic_curl_prop
        IMPORTED_LOCATION
        IMPORTED_LOCATION_RELEASE
        IMPORTED_LOCATION_RELWITHDEBINFO
        IMPORTED_LOCATION_MINSIZEREL
        IMPORTED_LOCATION_DEBUG)
      get_target_property(_plussmic_curl_candidate ${PLUSSMIC_CURL_TARGET} ${_plussmic_curl_prop})
      if(_plussmic_curl_candidate AND EXISTS "${_plussmic_curl_candidate}")
        set(_plussmic_curl_location "${_plussmic_curl_candidate}")
        break()
      endif()
    endforeach()

    if(NOT _plussmic_curl_location)
      if(DEFINED CURL_LIBRARY_RELEASE AND EXISTS "${CURL_LIBRARY_RELEASE}")
        set(_plussmic_curl_location "${CURL_LIBRARY_RELEASE}")
      elseif(DEFINED CURL_LIBRARY AND EXISTS "${CURL_LIBRARY}")
        set(_plussmic_curl_location "${CURL_LIBRARY}")
      elseif(DEFINED CURL_LIBRARIES)
        foreach(_plussmic_curl_candidate IN LISTS CURL_LIBRARIES)
          if(EXISTS "${_plussmic_curl_candidate}")
            set(_plussmic_curl_location "${_plussmic_curl_candidate}")
            break()
          endif()
        endforeach()
      endif()
    endif()

    if(_plussmic_curl_location)
      # Resolve symlinks so we install the concrete dylib/so artifact, not a
      # dangling symlink without its target.
      get_filename_component(_plussmic_curl_realpath "${_plussmic_curl_location}" REALPATH)
      get_filename_component(_plussmic_curl_filename "${_plussmic_curl_realpath}" NAME)
      install(FILES "${_plussmic_curl_realpath}"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}
      )
      set(_plussmic_curl_install_link
        $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/${CMAKE_INSTALL_LIBDIR}/${_plussmic_curl_filename}>
      )
    else()
      message(WARNING
        "PlussMic loader: could not resolve a concrete curl library file from "
        "${PLUSSMIC_CURL_TARGET}. SDK export will reference ${PLUSSMIC_CURL_TARGET} directly, "
        "so downstream must provide curl via find_package(CURL).")
      set(_plussmic_curl_install_link
        $<INSTALL_INTERFACE:${PLUSSMIC_CURL_TARGET}>
      )
    endif()
  else()
    install(TARGETS ${PLUSSMIC_CURL_TARGET}
      EXPORT PlussTargets
      ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
      LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
    set(_plussmic_curl_install_link
      $<INSTALL_INTERFACE:Pluss::${PLUSSMIC_CURL_TARGET}>
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
