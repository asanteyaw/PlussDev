cmake_minimum_required(VERSION 3.0)
project(PlussMic_loader NONE)
if(POLICY CMP0079)
  cmake_policy(SET CMP0079 NEW)
endif()

target_sources(PlussMic PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/Config.h
  ${CMAKE_CURRENT_SOURCE_DIR}/Loader.h
  ${CMAKE_CURRENT_SOURCE_DIR}/Loader.cpp
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
  target_link_libraries(PlussMic PRIVATE ${PLUSSMIC_LIBZIP_LIBRARY})
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
