# Package the staged distribution with CMake so this verification works on
# runners that do not provide a separate zip command.
foreach(required IN ITEMS SDK_BUILD_DIR SDK_SOURCE_DIR)
  if(NOT DEFINED ${required} OR "${${required}}" STREQUAL "")
    message(FATAL_ERROR "${required} is required")
  endif()
endforeach()

file(GLOB staged_dirs LIST_DIRECTORIES true "${SDK_BUILD_DIR}/sdk/plussii-cpp-*")
list(LENGTH staged_dirs staged_count)
if(NOT staged_count EQUAL 1)
  message(FATAL_ERROR "Expected one staged PlussII distribution, found ${staged_count}")
endif()

list(GET staged_dirs 0 staged_dir)
get_filename_component(staged_name "${staged_dir}" NAME)
set(DISTRIBUTION_ARCHIVE "${SDK_BUILD_DIR}/${staged_name}.zip")
execute_process(
  COMMAND "${CMAKE_COMMAND}" -E tar cf "${DISTRIBUTION_ARCHIVE}" --format=zip "${staged_name}"
  WORKING_DIRECTORY "${SDK_BUILD_DIR}/sdk"
  RESULT_VARIABLE archive_result
)
if(NOT archive_result EQUAL 0)
  message(FATAL_ERROR "Could not package ${staged_dir}")
endif()

set(CONSUMER_SOURCE_DIR "${SDK_SOURCE_DIR}/examples/sdk_consumer")
set(VALIDATION_WORK_DIR "${SDK_BUILD_DIR}/distribution-validation")
include("${CMAKE_CURRENT_LIST_DIR}/ValidateDistribution.cmake")
