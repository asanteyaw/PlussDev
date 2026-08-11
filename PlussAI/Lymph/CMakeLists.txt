# Lymph convenience layer (built on top of NodeJ)
#
# Layout mirrors PlussMic's own module/src split (PlussMic/src/table/*.cpp
# backs PlussMic/include/pluss/table/*.h): this directory holds docs +
# CMakeLists, src/ holds the .cpp implementation files that back the
# per-concern headers in include/pluss/ai/lymph/*.h (prep.cpp <-> prep.h,
# workflow_dsl.cpp <-> workflow_dsl.h, ...). Previously flat directly under
# Lymph/, with an unused, un-globbed Lymph/src/ sitting empty alongside them
# -- moved here so src/ actually has the role its name implies instead of
# being dead weight.

file(GLOB AI_LYMPH_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)
list(FILTER AI_LYMPH_SOURCES EXCLUDE REGEX ".*/quick\\.cpp$")

target_sources(pluss PRIVATE ${AI_LYMPH_SOURCES})

target_include_directories(pluss PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)

# Lymph's own tests -- lymph_*_test.cpp lives in Lymph/test/, not
# PlussAI/test/, as of this project's standing convention (every lymph test
# belongs next to the module it tests, matching src/ above).
add_subdirectory(test)

# User-facing Lymph demos. Targets are intentionally excluded from the default
# build so normal developer builds do not pick up heavier end-to-end examples,
# but each demo remains buildable by target name.
add_subdirectory(demos)
