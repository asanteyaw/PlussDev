cmake_minimum_required(VERSION 3.28)
project(PlussPl LANGUAGES CXX)

option(PLUSSPL_ENABLE_GTK_VIEWER "Enable GTK3/WebKit plot viewer when dependencies are available" ON)

if(NOT PkgConfig_FOUND)
    find_package(PkgConfig QUIET)
endif()

add_library(PlussPl STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/plotism/core/theme.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/plotism/core/lens_trace.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/plotism/core/pane.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/plotism/core/studio.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/plotism/render/renderer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/plotism/render/export_frontend.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/plotism/render/PlatformHtmlViewer.h
)

add_library(Pluss::PlussPl ALIAS PlussPl)

set(PLUSSPL_HAVE_GTK_WEBKIT_VALUE 0)
set(PLUSSPL_HAVE_MAC_WEBKIT_VALUE 0)
set(PLUSSPL_HAVE_WIN_WEBVIEW2_VALUE 0)

if(PLUSSPL_ENABLE_GTK_VIEWER AND PkgConfig_FOUND)
    pkg_check_modules(PLUSSPL_GTK3 QUIET gtk+-3.0)
    pkg_check_modules(PLUSSPL_WEBKIT41 QUIET webkit2gtk-4.1)
    if(NOT PLUSSPL_WEBKIT41_FOUND)
        pkg_check_modules(PLUSSPL_WEBKIT40 QUIET webkit2gtk-4.0)
    endif()

    set(PLUSSPL_WEBKIT_FOUND FALSE)
    set(PLUSSPL_WEBKIT_INCLUDE_DIRS "")
    set(PLUSSPL_WEBKIT_LIBRARIES "")
    set(PLUSSPL_WEBKIT_LIBRARY_DIRS "")
    set(PLUSSPL_WEBKIT_CFLAGS_OTHER "")

    if(PLUSSPL_WEBKIT41_FOUND)
        set(PLUSSPL_WEBKIT_FOUND TRUE)
        set(PLUSSPL_WEBKIT_INCLUDE_DIRS ${PLUSSPL_WEBKIT41_INCLUDE_DIRS})
        set(PLUSSPL_WEBKIT_LIBRARIES ${PLUSSPL_WEBKIT41_LIBRARIES})
        set(PLUSSPL_WEBKIT_LIBRARY_DIRS ${PLUSSPL_WEBKIT41_LIBRARY_DIRS})
        set(PLUSSPL_WEBKIT_CFLAGS_OTHER ${PLUSSPL_WEBKIT41_CFLAGS_OTHER})
    elseif(PLUSSPL_WEBKIT40_FOUND)
        set(PLUSSPL_WEBKIT_FOUND TRUE)
        set(PLUSSPL_WEBKIT_INCLUDE_DIRS ${PLUSSPL_WEBKIT40_INCLUDE_DIRS})
        set(PLUSSPL_WEBKIT_LIBRARIES ${PLUSSPL_WEBKIT40_LIBRARIES})
        set(PLUSSPL_WEBKIT_LIBRARY_DIRS ${PLUSSPL_WEBKIT40_LIBRARY_DIRS})
        set(PLUSSPL_WEBKIT_CFLAGS_OTHER ${PLUSSPL_WEBKIT40_CFLAGS_OTHER})
    endif()

    if(PLUSSPL_GTK3_FOUND AND PLUSSPL_WEBKIT_FOUND)
        set(PLUSSPL_HAVE_GTK_WEBKIT_VALUE 1)
        target_include_directories(PlussPl SYSTEM PRIVATE
            ${PLUSSPL_GTK3_INCLUDE_DIRS}
            ${PLUSSPL_WEBKIT_INCLUDE_DIRS}
        )
        target_link_directories(PlussPl PRIVATE
            ${PLUSSPL_GTK3_LIBRARY_DIRS}
            ${PLUSSPL_WEBKIT_LIBRARY_DIRS}
        )
        target_compile_options(PlussPl PRIVATE
            ${PLUSSPL_GTK3_CFLAGS_OTHER}
            ${PLUSSPL_WEBKIT_CFLAGS_OTHER}
        )
        target_link_libraries(PlussPl PRIVATE
            ${PLUSSPL_GTK3_LIBRARIES}
            ${PLUSSPL_WEBKIT_LIBRARIES}
        )
    endif()
endif()

if(APPLE)
    enable_language(OBJCXX)
    set(PLUSSPL_HAVE_MAC_WEBKIT_VALUE 1)
    target_sources(PlussPl PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/plotism/render/MacHtmlViewer.mm
    )
    set_source_files_properties(
        ${CMAKE_CURRENT_SOURCE_DIR}/src/plotism/render/MacHtmlViewer.mm
        PROPERTIES COMPILE_OPTIONS "-fobjc-arc"
    )
    target_link_libraries(PlussPl PRIVATE
        "-framework Cocoa"
        "-framework UniformTypeIdentifiers"
        "-framework WebKit"
        "-framework QuartzCore"
    )
endif()

if(WIN32)
    option(PLUSSPL_ENABLE_WINDOWS_WEBVIEW2 "Enable WebView2 plot viewer on Windows when SDK is available" ON)
    set(PLUSSPL_WEBVIEW2_SDK_DIR "" CACHE PATH "Path to the Microsoft.Web.WebView2 SDK root")

    if(PLUSSPL_ENABLE_WINDOWS_WEBVIEW2)
        set(PLUSSPL_WEBVIEW2_HINTS ${PLUSSPL_WEBVIEW2_SDK_DIR})
        if(DEFINED ENV{WEBVIEW2_SDK_DIR} AND NOT "$ENV{WEBVIEW2_SDK_DIR}" STREQUAL "")
            list(APPEND PLUSSPL_WEBVIEW2_HINTS "$ENV{WEBVIEW2_SDK_DIR}")
        endif()

        if(DEFINED ENV{USERPROFILE} AND NOT "$ENV{USERPROFILE}" STREQUAL "")
            file(GLOB PLUSSPL_WEBVIEW2_NUGET_DIRS
                 "$ENV{USERPROFILE}/.nuget/packages/microsoft.web.webview2/*")
            list(REVERSE PLUSSPL_WEBVIEW2_NUGET_DIRS)
            list(APPEND PLUSSPL_WEBVIEW2_HINTS ${PLUSSPL_WEBVIEW2_NUGET_DIRS})
        endif()

        find_path(PLUSSPL_WEBVIEW2_INCLUDE_DIR
                  NAMES WebView2.h
                  HINTS ${PLUSSPL_WEBVIEW2_HINTS}
                  PATH_SUFFIXES build/native/include include)

        set(PLUSSPL_WEBVIEW2_ARCH_HINTS x64)
        if(CMAKE_SYSTEM_PROCESSOR MATCHES "^[Aa][Rr][Mm]64$")
            set(PLUSSPL_WEBVIEW2_ARCH_HINTS arm64 x64)
        elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
            set(PLUSSPL_WEBVIEW2_ARCH_HINTS x86)
        endif()

        unset(PLUSSPL_WEBVIEW2_LOADER CACHE)
        foreach(PLUSSPL_WEBVIEW2_ARCH IN LISTS PLUSSPL_WEBVIEW2_ARCH_HINTS)
            if(NOT PLUSSPL_WEBVIEW2_LOADER)
                find_library(PLUSSPL_WEBVIEW2_LOADER
                             NAMES WebView2LoaderStatic WebView2Loader
                             HINTS ${PLUSSPL_WEBVIEW2_HINTS}
                             PATH_SUFFIXES build/native/${PLUSSPL_WEBVIEW2_ARCH} ${PLUSSPL_WEBVIEW2_ARCH} lib)
            endif()
        endforeach()

        if(PLUSSPL_WEBVIEW2_INCLUDE_DIR AND PLUSSPL_WEBVIEW2_LOADER)
            set(PLUSSPL_HAVE_WIN_WEBVIEW2_VALUE 1)
            target_include_directories(PlussPl SYSTEM PRIVATE ${PLUSSPL_WEBVIEW2_INCLUDE_DIR})
            target_sources(PlussPl PRIVATE
                ${CMAKE_CURRENT_SOURCE_DIR}/src/plotism/render/WindowsHtmlViewer.cpp
            )
            target_link_libraries(PlussPl PRIVATE ${PLUSSPL_WEBVIEW2_LOADER} ole32 comdlg32 shlwapi)
        endif()
    endif()
endif()

target_compile_definitions(PlussPl PUBLIC PLUSSPL_HAVE_GTK_WEBKIT=${PLUSSPL_HAVE_GTK_WEBKIT_VALUE})
target_compile_definitions(PlussPl PUBLIC PLUSSPL_HAVE_MAC_WEBKIT=${PLUSSPL_HAVE_MAC_WEBKIT_VALUE})
target_compile_definitions(PlussPl PUBLIC PLUSSPL_HAVE_WIN_WEBVIEW2=${PLUSSPL_HAVE_WIN_WEBVIEW2_VALUE})

target_include_directories(PlussPl PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../PlussQit/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../PlussQit/src>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(PlussPl PUBLIC
    PlussQit
    plussqit_codem
)

set_target_properties(PlussPl PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

install(TARGETS PlussPl
    EXPORT PlussTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "*.hh"
    PATTERN ".DS_Store" EXCLUDE
)

if(BUILD_TESTING)
    add_subdirectory(test)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples")
    add_subdirectory(examples)
endif()
