if(NOT DEFINED TOP_LEVEL_BUILD_DIR)
  message(FATAL_ERROR "TOP_LEVEL_BUILD_DIR is required")
endif()

if(NOT DEFINED SDK_STAGE_DIR)
  message(FATAL_ERROR "SDK_STAGE_DIR is required")
endif()

if(NOT DEFINED SDK_CONSUMER_SOURCE_DIR)
  message(FATAL_ERROR "SDK_CONSUMER_SOURCE_DIR is required")
endif()

if(NOT DEFINED SDK_CONSUMER_BUILD_DIR)
  message(FATAL_ERROR "SDK_CONSUMER_BUILD_DIR is required")
endif()

execute_process(
  COMMAND ${CMAKE_COMMAND} --build ${TOP_LEVEL_BUILD_DIR} --target pluss_sdk_stage -j1
  RESULT_VARIABLE stage_result
)
if(NOT stage_result EQUAL 0)
  message(FATAL_ERROR "Failed to stage the pluss SDK")
endif()

execute_process(
  COMMAND ${CMAKE_COMMAND}
          -S ${SDK_CONSUMER_SOURCE_DIR}
          -B ${SDK_CONSUMER_BUILD_DIR}
          -DCMAKE_PREFIX_PATH=${SDK_STAGE_DIR}
  RESULT_VARIABLE configure_result
)
if(NOT configure_result EQUAL 0)
  message(FATAL_ERROR "Failed to configure the standalone SDK consumer")
endif()

execute_process(
  COMMAND ${CMAKE_COMMAND} --build ${SDK_CONSUMER_BUILD_DIR} -j1
  RESULT_VARIABLE build_result
)
if(NOT build_result EQUAL 0)
  message(FATAL_ERROR "Failed to build the standalone SDK consumer")
endif()

if(WIN32)
  set(sdk_consumer_binary ${SDK_CONSUMER_BUILD_DIR}/Debug/pluss_sdk_consumer.exe)
  if(NOT EXISTS ${sdk_consumer_binary})
    set(sdk_consumer_binary ${SDK_CONSUMER_BUILD_DIR}/pluss_sdk_consumer.exe)
  endif()
else()
  set(sdk_consumer_binary ${SDK_CONSUMER_BUILD_DIR}/pluss_sdk_consumer)
endif()

execute_process(
  COMMAND ${sdk_consumer_binary}
  RESULT_VARIABLE run_result
)
if(NOT run_result EQUAL 0)
  message(FATAL_ERROR "Standalone SDK consumer ran unsuccessfully")
endif()
