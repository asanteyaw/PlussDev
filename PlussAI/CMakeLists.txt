if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_include_directories(pluss PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/NodeK/kernels")
    add_subdirectory(NodeK/kernels)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/NodeK/models")
    add_subdirectory(NodeK/models)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/NodeJ/optim")
    add_subdirectory(NodeJ/optim)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/NodeK/NN/optim")
    add_subdirectory(NodeK/NN/optim)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tools")
    add_subdirectory(tools)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/utils")
    add_subdirectory(utils)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples")
    add_subdirectory(examples)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test")
    add_subdirectory(test)
endif()
