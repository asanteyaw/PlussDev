# Lymph convenience layer (built on top of NodeJ)

file(GLOB AI_LYMPH_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)
list(FILTER AI_LYMPH_SOURCES EXCLUDE REGEX ".*/quick\\.cpp$")

target_sources(pluss PRIVATE ${AI_LYMPH_SOURCES})

target_include_directories(pluss PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)
