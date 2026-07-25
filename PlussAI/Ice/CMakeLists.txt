file(GLOB_RECURSE AI_ICE_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/data/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/io/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/models/*.cpp
)

if(AI_ICE_SOURCES)
  target_sources(pluss PRIVATE ${AI_ICE_SOURCES})
  target_include_directories(pluss PRIVATE
      ${PROJECT_SOURCE_DIR}/include
  )
endif()
