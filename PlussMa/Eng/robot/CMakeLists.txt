target_sources(pluss PRIVATE robot.cpp)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test")
  add_subdirectory(test)
endif()
