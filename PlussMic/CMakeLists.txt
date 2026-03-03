cmake_minimum_required(VERSION 3.28)
project(PlussMic LANGUAGES CXX)

option(PLUSSMIC_ENABLE_GTK_VIEWER "Enable GTK3/WebKit DataFrame viewer when dependencies are available" ON)

add_library(PlussMic STATIC)

set(PLUSSMIC_HAVE_GTK_WEBKIT_VALUE 0)

target_include_directories(PlussMic PUBLIC
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/../PlussPl/include
    ${PROJECT_SOURCE_DIR}/../include
    ${PROJECT_SOURCE_DIR}/../PlussQit/include
    ${PROJECT_SOURCE_DIR}/../PlussQit/src
    ${PROJECT_SOURCE_DIR}/../ExLibs/fmt/include
)

target_compile_definitions(PlussMic PRIVATE FMT_HEADER_ONLY=1)

if(PLUSSMIC_ENABLE_GTK_VIEWER AND PkgConfig_FOUND)
    pkg_check_modules(PLUSSMIC_GTK3 QUIET gtk+-3.0)
    pkg_check_modules(PLUSSMIC_WEBKIT41 QUIET webkit2gtk-4.1)
    if(NOT PLUSSMIC_WEBKIT41_FOUND)
        pkg_check_modules(PLUSSMIC_WEBKIT40 QUIET webkit2gtk-4.0)
    endif()

    set(PLUSSMIC_WEBKIT_FOUND FALSE)
    set(PLUSSMIC_WEBKIT_INCLUDE_DIRS "")
    set(PLUSSMIC_WEBKIT_LIBRARIES "")
    set(PLUSSMIC_WEBKIT_LIBRARY_DIRS "")
    set(PLUSSMIC_WEBKIT_CFLAGS_OTHER "")

    if(PLUSSMIC_WEBKIT41_FOUND)
        set(PLUSSMIC_WEBKIT_FOUND TRUE)
        set(PLUSSMIC_WEBKIT_INCLUDE_DIRS ${PLUSSMIC_WEBKIT41_INCLUDE_DIRS})
        set(PLUSSMIC_WEBKIT_LIBRARIES ${PLUSSMIC_WEBKIT41_LIBRARIES})
        set(PLUSSMIC_WEBKIT_LIBRARY_DIRS ${PLUSSMIC_WEBKIT41_LIBRARY_DIRS})
        set(PLUSSMIC_WEBKIT_CFLAGS_OTHER ${PLUSSMIC_WEBKIT41_CFLAGS_OTHER})
    elseif(PLUSSMIC_WEBKIT40_FOUND)
        set(PLUSSMIC_WEBKIT_FOUND TRUE)
        set(PLUSSMIC_WEBKIT_INCLUDE_DIRS ${PLUSSMIC_WEBKIT40_INCLUDE_DIRS})
        set(PLUSSMIC_WEBKIT_LIBRARIES ${PLUSSMIC_WEBKIT40_LIBRARIES})
        set(PLUSSMIC_WEBKIT_LIBRARY_DIRS ${PLUSSMIC_WEBKIT40_LIBRARY_DIRS})
        set(PLUSSMIC_WEBKIT_CFLAGS_OTHER ${PLUSSMIC_WEBKIT40_CFLAGS_OTHER})
    endif()

    if(PLUSSMIC_GTK3_FOUND AND PLUSSMIC_WEBKIT_FOUND)
        set(PLUSSMIC_HAVE_GTK_WEBKIT_VALUE 1)
        target_include_directories(PlussMic SYSTEM PRIVATE
            ${PLUSSMIC_GTK3_INCLUDE_DIRS}
            ${PLUSSMIC_WEBKIT_INCLUDE_DIRS}
        )
        target_link_directories(PlussMic PRIVATE
            ${PLUSSMIC_GTK3_LIBRARY_DIRS}
            ${PLUSSMIC_WEBKIT_LIBRARY_DIRS}
        )
        target_compile_options(PlussMic PRIVATE
            ${PLUSSMIC_GTK3_CFLAGS_OTHER}
            ${PLUSSMIC_WEBKIT_CFLAGS_OTHER}
        )
        target_link_libraries(PlussMic PRIVATE
            ${PLUSSMIC_GTK3_LIBRARIES}
            ${PLUSSMIC_WEBKIT_LIBRARIES}
        )
    endif()
endif()

target_compile_definitions(PlussMic PUBLIC PLUSSMIC_HAVE_GTK_WEBKIT=${PLUSSMIC_HAVE_GTK_WEBKIT_VALUE})

target_sources(PlussMic PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/converters.cpp
)

add_subdirectory(src/loader)
add_subdirectory(src/kindle)
add_subdirectory(src/bundle)
add_subdirectory(src/table)

target_link_libraries(PlussMic PUBLIC
    PlussPl
    PlussQit
    plussqit_codeh
    plussqit_codem
    plussqit_coden
)

set_target_properties(PlussMic PROPERTIES CXX_STANDARD 20)

add_subdirectory(examples)
add_subdirectory(test)
