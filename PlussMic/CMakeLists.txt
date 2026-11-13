cmake_minimum_required(VERSION 3.28)
project(PlussMic LANGUAGES CXX)
find_package(SQLite3 REQUIRED)

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
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_sources(PlussMic PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/converters.cpp
)

add_subdirectory(src/loader)
add_subdirectory(src/relational)
add_subdirectory(src/kindle)
add_subdirectory(src/bundle)
add_subdirectory(src/scribble)
add_subdirectory(src/table)

target_link_libraries(PlussMic PUBLIC
    PlussPl
    PlussTils
    PlussQit
    plussqit_codeh
    plussqit_codem
    plussqit_coden
)
# SDK headers are already found through -isysroot on macOS. Injecting the
# SDK's usr/include as an explicit system path puts C headers ahead of the
# libc++ wrappers and breaks <cstddef>/<cmath> with recent Xcode toolchains.
if(APPLE AND SQLite3_INCLUDE_DIR STREQUAL "${CMAKE_OSX_SYSROOT}/usr/include")
    target_link_libraries(PlussMic PRIVATE "$<LINK_ONLY:SQLite::SQLite3>")
else()
    target_link_libraries(PlussMic PRIVATE SQLite::SQLite3)
endif()

set_target_properties(PlussMic PROPERTIES CXX_STANDARD 20)

install(TARGETS PlussMic
    EXPORT PlussTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

add_subdirectory(examples)
add_subdirectory(test)
