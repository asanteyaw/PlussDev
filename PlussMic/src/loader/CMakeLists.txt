cmake_minimum_required(VERSION 3.0)
project(PlussMic_loader NONE)

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

find_path(PLUSSMIC_LIBZIP_INCLUDE_DIR zip.h)
find_library(PLUSSMIC_LIBZIP_LIBRARY zip)
if(PLUSSMIC_LIBZIP_INCLUDE_DIR AND PLUSSMIC_LIBZIP_LIBRARY)
  target_include_directories(PlussMic PRIVATE ${PLUSSMIC_LIBZIP_INCLUDE_DIR})
  target_link_libraries(PlussMic PRIVATE ${PLUSSMIC_LIBZIP_LIBRARY})
  target_compile_definitions(PlussMic PRIVATE PLUSSMIC_HAS_LIBZIP=1)
else()
  target_compile_definitions(PlussMic PRIVATE PLUSSMIC_HAS_LIBZIP=0)
endif()
