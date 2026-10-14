# Physics: astro

file(GLOB ASTRO_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

target_sources(pluss PRIVATE ${ASTRO_SOURCES})

target_include_directories(pluss PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test")
    add_subdirectory(test)
endif()

if(BUILD_BENCHMARKS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench")
    add_subdirectory(bench)
endif()
