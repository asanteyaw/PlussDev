if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_include_directories(pluss PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/PlussAI/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/PlussAI>
    )
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/NodeK/kernels")
    add_subdirectory(NodeK/kernels)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/NodeK/models")
    add_subdirectory(NodeK/models)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/NodeJ/core")
    add_subdirectory(NodeJ/core)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/NodeJ/optim")
    add_subdirectory(NodeJ/optim)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/NodeJ/classifiers")
    add_subdirectory(NodeJ/classifiers)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/NodeJ/regressors")
    add_subdirectory(NodeJ/regressors)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/NodeJ/data")
    add_subdirectory(NodeJ/data)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/NodeJ/prep")
    add_subdirectory(NodeJ/prep)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/NodeJ/split")
    add_subdirectory(NodeJ/split)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/NodeJ/metrics")
    add_subdirectory(NodeJ/metrics)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/NodeJ/compose")
    add_subdirectory(NodeJ/compose)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/NodeJ/tune")
    add_subdirectory(NodeJ/tune)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/NodeJ/online")
    add_subdirectory(NodeJ/online)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/NodeJ/io")
    add_subdirectory(NodeJ/io)
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
