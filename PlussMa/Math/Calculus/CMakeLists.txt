target_sources(pluss PRIVATE
    Function.cpp
    Operator.cpp
    PrimitiveRegistry.cpp
)

if(BUILD_TESTING AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test")
    add_subdirectory(test)
endif()
