file(GLOB_RECURSE AI_NODEB_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/embed/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/eval/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/io/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/pipeline/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/registry/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tokenize/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tasks/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/vectorize/*.cpp
)

if(AI_NODEB_SOURCES)
  target_sources(pluss PRIVATE ${AI_NODEB_SOURCES})
  target_include_directories(pluss PRIVATE
      ${PROJECT_SOURCE_DIR}/include
  )
endif()
