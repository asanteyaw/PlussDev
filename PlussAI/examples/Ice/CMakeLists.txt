if(NOT TARGET pluss)
  return()
endif()

add_executable(plussai_ice_full_capability_demo
  ice_full_capability_demo.cpp
)
target_link_libraries(plussai_ice_full_capability_demo PRIVATE pluss $<TARGET_NAME_IF_EXISTS:PlussMic>)
target_include_directories(plussai_ice_full_capability_demo
  PRIVATE ${CMAKE_SOURCE_DIR}/PlussMic/include)
target_compile_features(plussai_ice_full_capability_demo PRIVATE cxx_std_20)
target_compile_definitions(plussai_ice_full_capability_demo
  PRIVATE PLUSSAI_SOURCE_DIR="${CMAKE_SOURCE_DIR}/PlussAI")
