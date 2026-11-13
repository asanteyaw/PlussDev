# Package the staged distribution with CMake so this verification works on
# runners that do not provide a separate zip command.
foreach(required IN ITEMS DISTRIBUTION_BUILD_DIR DISTRIBUTION_SOURCE_DIR)
  if(NOT DEFINED ${required} OR "${${required}}" STREQUAL "")
    message(FATAL_ERROR "${required} is required")
  endif()
endforeach()

file(GLOB staged_dirs LIST_DIRECTORIES true "${DISTRIBUTION_BUILD_DIR}/plussii-cpp/plussii-cpp-*")
list(LENGTH staged_dirs staged_count)
if(NOT staged_count EQUAL 1)
  message(FATAL_ERROR "Expected one staged PlussII distribution, found ${staged_count}")
endif()

list(GET staged_dirs 0 staged_dir)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
execute_process(
  COMMAND "${Python3_EXECUTABLE}"
      "${DISTRIBUTION_SOURCE_DIR}/tools/check_distribution_relocation.py"
      "${staged_dir}" "${DISTRIBUTION_SOURCE_DIR}" "${DISTRIBUTION_BUILD_DIR}"
  RESULT_VARIABLE relocation_result
)
if(NOT relocation_result EQUAL 0)
  message(FATAL_ERROR "Staged PlussII distribution contains build-host paths")
endif()

get_filename_component(staged_name "${staged_dir}" NAME)
set(DISTRIBUTION_ARCHIVE "${DISTRIBUTION_BUILD_DIR}/${staged_name}.zip")
execute_process(
  COMMAND "${CMAKE_COMMAND}" -E tar cf "${DISTRIBUTION_ARCHIVE}" --format=zip "${staged_name}"
  WORKING_DIRECTORY "${DISTRIBUTION_BUILD_DIR}/plussii-cpp"
  RESULT_VARIABLE archive_result
)
if(NOT archive_result EQUAL 0)
  message(FATAL_ERROR "Could not package ${staged_dir}")
endif()

set(CONSUMER_SOURCE_DIR "${DISTRIBUTION_SOURCE_DIR}/examples/sdk_consumer")
set(VALIDATION_WORK_DIR "${DISTRIBUTION_BUILD_DIR}/distribution-validation")

# Reuse the toolset and dependency toolchain that built the archive. This is
# essential for Windows ClangCL archives and their static vcpkg dependencies.
set(build_cache "${DISTRIBUTION_BUILD_DIR}/CMakeCache.txt")
foreach(cache_name IN ITEMS CMAKE_GENERATOR CMAKE_GENERATOR_TOOLSET
                            CMAKE_TOOLCHAIN_FILE VCPKG_TARGET_TRIPLET)
  file(STRINGS "${build_cache}" cache_entry REGEX "^${cache_name}:")
  if(cache_entry)
    string(REGEX REPLACE "^[^=]*=" "" cache_value "${cache_entry}")
    set("DISTRIBUTION_CONSUMER_${cache_name}" "${cache_value}")
  endif()
endforeach()

include("${CMAKE_CURRENT_LIST_DIR}/ValidateDistribution.cmake")
