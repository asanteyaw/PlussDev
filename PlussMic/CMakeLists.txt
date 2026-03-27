cmake_minimum_required(VERSION 3.28)
project(PlussMic LANGUAGES CXX)

add_library(PlussMic STATIC)
add_library(Pluss::PlussMic ALIAS PlussMic)

target_include_directories(PlussMic PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/../PlussPl/include>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/../include>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/../PlussQit/include>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/../PlussQit/src>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/../ExLibs/fmt/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_definitions(PlussMic PRIVATE FMT_HEADER_ONLY=1)

target_sources(PlussMic PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/converters.cpp
)

add_subdirectory(src/loader)
add_subdirectory(src/kindle)
add_subdirectory(src/bundle)
add_subdirectory(src/table)

target_link_libraries(PlussMic PUBLIC
    PlussPl
    PlussTils
    PlussQit
    plussqit_codeh
    plussqit_codem
    plussqit_coden
)

set_target_properties(PlussMic PROPERTIES CXX_STANDARD 20)

install(TARGETS PlussMic
    EXPORT PlussTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

add_subdirectory(examples)
add_subdirectory(test)
