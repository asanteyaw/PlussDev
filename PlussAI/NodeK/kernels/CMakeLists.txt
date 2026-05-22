# AI kernels: numerical / low-level components

file(GLOB AI_KERNEL_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

target_sources(pluss PRIVATE ${AI_KERNEL_SOURCES})

if(APPLE)
    enable_language(OBJCXX)
    if(NOT CMAKE_OBJCXX_COMPILE_OBJECT)
        set(CMAKE_OBJCXX_COMPILE_OBJECT
            "<CMAKE_OBJCXX_COMPILER> <DEFINES> <INCLUDES> <FLAGS> -x objective-c++ -o <OBJECT> -c <SOURCE>")
    endif()
    target_sources(pluss PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/mps_rnn.mm
        ${CMAKE_CURRENT_SOURCE_DIR}/mps_conv.mm
    )
    target_compile_options(pluss PRIVATE $<$<COMPILE_LANGUAGE:OBJCXX>:-fobjc-arc>)
endif()

target_include_directories(pluss PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)
