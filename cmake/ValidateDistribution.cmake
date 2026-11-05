# Test the downloadable archive itself, without using headers or libraries from
# the source tree. The example is configured after the archive is extracted.
foreach(required IN ITEMS DISTRIBUTION_ARCHIVE CONSUMER_SOURCE_DIR VALIDATION_WORK_DIR)
  if(NOT DEFINED ${required} OR "${${required}}" STREQUAL "")
    message(FATAL_ERROR "${required} is required")
  endif()
endforeach()

if(NOT EXISTS "${DISTRIBUTION_ARCHIVE}")
  message(FATAL_ERROR "Distribution archive not found: ${DISTRIBUTION_ARCHIVE}")
endif()

file(REMOVE_RECURSE "${VALIDATION_WORK_DIR}")
set(extract_dir "${VALIDATION_WORK_DIR}/extract")
set(consumer_build_dir "${VALIDATION_WORK_DIR}/consumer-build")
file(MAKE_DIRECTORY "${extract_dir}")
file(ARCHIVE_EXTRACT INPUT "${DISTRIBUTION_ARCHIVE}" DESTINATION "${extract_dir}")

file(GLOB package_configs "${extract_dir}/*/lib/cmake/Pluss/PlussConfig.cmake")
list(LENGTH package_configs config_count)
if(NOT config_count EQUAL 1)
  message(FATAL_ERROR "Expected one PlussConfig.cmake in the archive; found ${config_count}")
endif()

list(GET package_configs 0 package_config)
get_filename_component(package_dir "${package_config}" DIRECTORY)
get_filename_component(cmake_dir "${package_dir}" DIRECTORY)
get_filename_component(lib_dir "${cmake_dir}" DIRECTORY)
get_filename_component(distribution_dir "${lib_dir}" DIRECTORY)

set(consumer_prefix_path "${distribution_dir}")
if(DEFINED DISTRIBUTION_DEPENDENCY_PREFIX_PATH
   AND NOT "${DISTRIBUTION_DEPENDENCY_PREFIX_PATH}" STREQUAL "")
  list(APPEND consumer_prefix_path "${DISTRIBUTION_DEPENDENCY_PREFIX_PATH}")
endif()
list(JOIN consumer_prefix_path ";" consumer_prefix_path_arg)

execute_process(
  COMMAND "${CMAKE_COMMAND}" -S "${CONSUMER_SOURCE_DIR}" -B "${consumer_build_dir}"
          "-DCMAKE_PREFIX_PATH=${consumer_prefix_path_arg}"
          -DCMAKE_BUILD_TYPE=Release
  RESULT_VARIABLE configure_result
)
if(NOT configure_result EQUAL 0)
  message(FATAL_ERROR "Could not configure a consumer against ${DISTRIBUTION_ARCHIVE}")
endif()

execute_process(
  COMMAND "${CMAKE_COMMAND}" --build "${consumer_build_dir}" --config Release --parallel 2
  RESULT_VARIABLE build_result
)
if(NOT build_result EQUAL 0)
  message(FATAL_ERROR "Could not build a consumer against ${DISTRIBUTION_ARCHIVE}")
endif()

if(WIN32)
  set(consumer_binary "${consumer_build_dir}/Release/pluss_sdk_consumer.exe")
  if(NOT EXISTS "${consumer_binary}")
    set(consumer_binary "${consumer_build_dir}/pluss_sdk_consumer.exe")
  endif()
else()
  set(consumer_binary "${consumer_build_dir}/pluss_sdk_consumer")
endif()

execute_process(COMMAND "${consumer_binary}" RESULT_VARIABLE run_result)
if(NOT run_result EQUAL 0)
  message(FATAL_ERROR "Consumer failed after linking the distribution archive")
endif()

message(STATUS "Validated PlussII C++ distribution: ${DISTRIBUTION_ARCHIVE}")
