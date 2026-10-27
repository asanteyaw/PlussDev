target_sources(pluss PRIVATE fluid.cpp transport.cpp)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test")
  add_subdirectory(test)
endif()
