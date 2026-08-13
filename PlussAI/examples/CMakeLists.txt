if(TARGET pluss)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/NodeB")
    add_subdirectory(NodeB)
  endif()

  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/NodeJ")
    add_subdirectory(NodeJ)
  endif()

  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/NodeK")
    add_subdirectory(NodeK)
  endif()

  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Ice")
    add_subdirectory(Ice)
  endif()

  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Procedure")
    add_subdirectory(Procedure)
  endif()

  add_executable(plussai_minimax_demo
    minimax_demo.cpp
  )
  target_link_libraries(plussai_minimax_demo PRIVATE pluss)
  target_compile_features(plussai_minimax_demo PRIVATE cxx_std_20)

  add_executable(plussai_minimax_multiblock_constraints_demo
    minimax_multiblock_constraints_demo.cpp
  )
  target_link_libraries(plussai_minimax_multiblock_constraints_demo PRIVATE pluss)
  target_compile_features(plussai_minimax_multiblock_constraints_demo PRIVATE cxx_std_20)
endif()
