add_subdirectory(kernels)
add_subdirectory(models)
add_subdirectory(optim)
add_subdirectory(io)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test")
    add_subdirectory(test)
endif()