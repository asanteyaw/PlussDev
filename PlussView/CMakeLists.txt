cmake_minimum_required(VERSION 3.28)
project(PlussView LANGUAGES CXX)

option(PLUSSVIEW_ENABLE_GTK_VIEWER "Enable GTK3/WebKit HTML window support in PlussView" ON)
option(PLUSSVIEW_ENABLE_EXPERIMENTAL_NATIVE_SHOWCASE
       "Build native popup showcase demo before full cross-platform window parity"
       OFF)

add_library(PlussView STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/html_window.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/session.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/window/core/runtime.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/window/core/view_session.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/window/core/window_registry.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/window/shell/null_shell.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/window/shell/shell_factory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/window/render/canvas.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/window/export/export.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/window/render/software_renderer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/window/render/text_raster.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/window/scene/scene.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/window/scene/plot_builder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/window/scene/table_builder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/window/html/html_document.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/window/html/html_layout.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/window/html/html_mapping.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/window/text/text_layout.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/InHouseHtmlViewer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/PlatformHtmlViewer.h
)

add_library(Pluss::View ALIAS PlussView)

if(NOT PkgConfig_FOUND)
    find_package(PkgConfig QUIET)
endif()

set(PLUSSVIEW_HAVE_GTK_WEBKIT_VALUE 0)
set(PLUSSVIEW_HAVE_MAC_WEBKIT_VALUE 0)
set(PLUSSVIEW_HAVE_WIN_WEBVIEW2_VALUE 0)

if(UNIX AND NOT APPLE AND PLUSSVIEW_ENABLE_GTK_VIEWER AND PkgConfig_FOUND)
    pkg_check_modules(PLUSSVIEW_GTK3 QUIET gtk+-3.0)
    pkg_check_modules(PLUSSVIEW_WEBKIT41 QUIET webkit2gtk-4.1)
    if(NOT PLUSSVIEW_WEBKIT41_FOUND)
        pkg_check_modules(PLUSSVIEW_WEBKIT40 QUIET webkit2gtk-4.0)
    endif()

    set(PLUSSVIEW_WEBKIT_FOUND FALSE)
    set(PLUSSVIEW_WEBKIT_INCLUDE_DIRS "")
    set(PLUSSVIEW_WEBKIT_LIBRARIES "")
    set(PLUSSVIEW_WEBKIT_LIBRARY_DIRS "")
    set(PLUSSVIEW_WEBKIT_CFLAGS_OTHER "")

    if(PLUSSVIEW_WEBKIT41_FOUND)
        set(PLUSSVIEW_WEBKIT_FOUND TRUE)
        set(PLUSSVIEW_WEBKIT_INCLUDE_DIRS ${PLUSSVIEW_WEBKIT41_INCLUDE_DIRS})
        set(PLUSSVIEW_WEBKIT_LIBRARIES ${PLUSSVIEW_WEBKIT41_LIBRARIES})
        set(PLUSSVIEW_WEBKIT_LIBRARY_DIRS ${PLUSSVIEW_WEBKIT41_LIBRARY_DIRS})
        set(PLUSSVIEW_WEBKIT_CFLAGS_OTHER ${PLUSSVIEW_WEBKIT41_CFLAGS_OTHER})
    elseif(PLUSSVIEW_WEBKIT40_FOUND)
        set(PLUSSVIEW_WEBKIT_FOUND TRUE)
        set(PLUSSVIEW_WEBKIT_INCLUDE_DIRS ${PLUSSVIEW_WEBKIT40_INCLUDE_DIRS})
        set(PLUSSVIEW_WEBKIT_LIBRARIES ${PLUSSVIEW_WEBKIT40_LIBRARIES})
        set(PLUSSVIEW_WEBKIT_LIBRARY_DIRS ${PLUSSVIEW_WEBKIT40_LIBRARY_DIRS})
        set(PLUSSVIEW_WEBKIT_CFLAGS_OTHER ${PLUSSVIEW_WEBKIT40_CFLAGS_OTHER})
    endif()

    if(PLUSSVIEW_GTK3_FOUND AND PLUSSVIEW_WEBKIT_FOUND)
        set(PLUSSVIEW_HAVE_GTK_WEBKIT_VALUE 1)
        target_sources(PlussView PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/LinuxHtmlViewer.cpp
        )
        target_include_directories(PlussView SYSTEM PRIVATE
            ${PLUSSVIEW_GTK3_INCLUDE_DIRS}
            ${PLUSSVIEW_WEBKIT_INCLUDE_DIRS}
        )
        target_link_directories(PlussView PRIVATE
            ${PLUSSVIEW_GTK3_LIBRARY_DIRS}
            ${PLUSSVIEW_WEBKIT_LIBRARY_DIRS}
        )
        target_compile_options(PlussView PRIVATE
            ${PLUSSVIEW_GTK3_CFLAGS_OTHER}
            ${PLUSSVIEW_WEBKIT_CFLAGS_OTHER}
        )
        target_link_libraries(PlussView PRIVATE
            ${PLUSSVIEW_GTK3_LIBRARIES}
            ${PLUSSVIEW_WEBKIT_LIBRARIES}
        )
    endif()
endif()

if(APPLE)
    enable_language(OBJCXX)
    set(PLUSSVIEW_HAVE_MAC_WEBKIT_VALUE 1)
    target_sources(PlussView PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/MacHtmlViewer.mm
        ${CMAKE_CURRENT_SOURCE_DIR}/src/window/shell/CocoaWindowShell.mm
        ${CMAKE_CURRENT_SOURCE_DIR}/src/window/render/text_raster_native_macos.mm
    )
    set_source_files_properties(
        ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/MacHtmlViewer.mm
        ${CMAKE_CURRENT_SOURCE_DIR}/src/window/shell/CocoaWindowShell.mm
        ${CMAKE_CURRENT_SOURCE_DIR}/src/window/render/text_raster_native_macos.mm
        PROPERTIES COMPILE_OPTIONS "-fobjc-arc"
    )
    target_link_libraries(PlussView PRIVATE
        "-framework Cocoa"
        "-framework UniformTypeIdentifiers"
        "-framework WebKit"
        "-framework QuartzCore"
        "-framework CoreText"
    )
endif()

if(WIN32)
    option(PLUSSVIEW_ENABLE_WINDOWS_WEBVIEW2 "Enable WebView2 HTML window support in PlussView" ON)
    set(PLUSSVIEW_WEBVIEW2_SDK_DIR "" CACHE PATH "Path to the Microsoft.Web.WebView2 SDK root")

    if(PLUSSVIEW_ENABLE_WINDOWS_WEBVIEW2)
        set(PLUSSVIEW_WEBVIEW2_HINTS ${PLUSSVIEW_WEBVIEW2_SDK_DIR})
        if(DEFINED ENV{WEBVIEW2_SDK_DIR} AND NOT "$ENV{WEBVIEW2_SDK_DIR}" STREQUAL "")
            list(APPEND PLUSSVIEW_WEBVIEW2_HINTS "$ENV{WEBVIEW2_SDK_DIR}")
        endif()

        if(DEFINED ENV{USERPROFILE} AND NOT "$ENV{USERPROFILE}" STREQUAL "")
            file(GLOB PLUSSVIEW_WEBVIEW2_NUGET_DIRS
                 "$ENV{USERPROFILE}/.nuget/packages/microsoft.web.webview2/*")
            list(REVERSE PLUSSVIEW_WEBVIEW2_NUGET_DIRS)
            list(APPEND PLUSSVIEW_WEBVIEW2_HINTS ${PLUSSVIEW_WEBVIEW2_NUGET_DIRS})
        endif()

        find_path(PLUSSVIEW_WEBVIEW2_INCLUDE_DIR
                  NAMES WebView2.h
                  HINTS ${PLUSSVIEW_WEBVIEW2_HINTS}
                  PATH_SUFFIXES build/native/include include)

        set(PLUSSVIEW_WEBVIEW2_ARCH_HINTS x64)
        if(CMAKE_SYSTEM_PROCESSOR MATCHES "^[Aa][Rr][Mm]64$")
            set(PLUSSVIEW_WEBVIEW2_ARCH_HINTS arm64 x64)
        elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
            set(PLUSSVIEW_WEBVIEW2_ARCH_HINTS x86)
        endif()

        unset(PLUSSVIEW_WEBVIEW2_LOADER CACHE)
        foreach(PLUSSVIEW_WEBVIEW2_ARCH IN LISTS PLUSSVIEW_WEBVIEW2_ARCH_HINTS)
            if(NOT PLUSSVIEW_WEBVIEW2_LOADER)
                find_library(PLUSSVIEW_WEBVIEW2_LOADER
                             NAMES WebView2LoaderStatic WebView2Loader
                             HINTS ${PLUSSVIEW_WEBVIEW2_HINTS}
                             PATH_SUFFIXES build/native/${PLUSSVIEW_WEBVIEW2_ARCH} ${PLUSSVIEW_WEBVIEW2_ARCH} lib)
            endif()
        endforeach()

        if(PLUSSVIEW_WEBVIEW2_INCLUDE_DIR AND PLUSSVIEW_WEBVIEW2_LOADER)
            set(PLUSSVIEW_HAVE_WIN_WEBVIEW2_VALUE 1)
            target_include_directories(PlussView SYSTEM PRIVATE ${PLUSSVIEW_WEBVIEW2_INCLUDE_DIR})
            target_sources(PlussView PRIVATE
                ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/WindowsHtmlViewer.cpp
            )
            target_link_libraries(PlussView PRIVATE ${PLUSSVIEW_WEBVIEW2_LOADER} ole32 comdlg32 shlwapi)
        endif()
    endif()
endif()

target_compile_definitions(PlussView PUBLIC
    PLUSSVIEW_HAVE_GTK_WEBKIT=${PLUSSVIEW_HAVE_GTK_WEBKIT_VALUE}
    PLUSSVIEW_HAVE_MAC_WEBKIT=${PLUSSVIEW_HAVE_MAC_WEBKIT_VALUE}
    PLUSSVIEW_HAVE_WIN_WEBVIEW2=${PLUSSVIEW_HAVE_WIN_WEBVIEW2_VALUE}
)

target_include_directories(PlussView PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_include_directories(PlussView PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

set_target_properties(PlussView PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

add_executable(plussview_window_runtime_demo
    ${CMAKE_CURRENT_SOURCE_DIR}/examples/window/runtime_demo.cpp
)
target_link_libraries(plussview_window_runtime_demo PRIVATE PlussView)
set_target_properties(plussview_window_runtime_demo PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

add_executable(plussview_window_first_plot_demo
    ${CMAKE_CURRENT_SOURCE_DIR}/examples/window/first_plot_demo.cpp
)
target_link_libraries(plussview_window_first_plot_demo PRIVATE PlussView)
set_target_properties(plussview_window_first_plot_demo PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

add_executable(plussview_window_interaction_core_demo
    ${CMAKE_CURRENT_SOURCE_DIR}/examples/window/interaction_core_demo.cpp
)
target_link_libraries(plussview_window_interaction_core_demo PRIVATE PlussView)
set_target_properties(plussview_window_interaction_core_demo PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

add_executable(plussview_window_session_demo
    ${CMAKE_CURRENT_SOURCE_DIR}/examples/window/session_demo.cpp
)
target_link_libraries(plussview_window_session_demo PRIVATE PlussView)
set_target_properties(plussview_window_session_demo PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

add_executable(plussview_window_registry_demo
    ${CMAKE_CURRENT_SOURCE_DIR}/examples/window/registry_demo.cpp
)
target_link_libraries(plussview_window_registry_demo PRIVATE PlussView)
set_target_properties(plussview_window_registry_demo PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

add_executable(plussview_window_table_demo
    ${CMAKE_CURRENT_SOURCE_DIR}/examples/window/table_demo.cpp
)
target_link_libraries(plussview_window_table_demo PRIVATE PlussView)
set_target_properties(plussview_window_table_demo PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

add_executable(plussview_window_export_smoke_demo
    ${CMAKE_CURRENT_SOURCE_DIR}/examples/window/export_smoke_demo.cpp
)
target_link_libraries(plussview_window_export_smoke_demo PRIVATE PlussView)
set_target_properties(plussview_window_export_smoke_demo PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

add_executable(plussview_window_shell_backend_demo
    ${CMAKE_CURRENT_SOURCE_DIR}/examples/window/shell_backend_demo.cpp
)
target_link_libraries(plussview_window_shell_backend_demo PRIVATE PlussView)
set_target_properties(plussview_window_shell_backend_demo PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

add_executable(plussview_inhouse_html_render_demo
    ${CMAKE_CURRENT_SOURCE_DIR}/examples/window/inhouse_html_render_demo.cpp
)
target_link_libraries(plussview_inhouse_html_render_demo PRIVATE PlussView)
set_target_properties(plussview_inhouse_html_render_demo PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

if(PLUSSVIEW_ENABLE_EXPERIMENTAL_NATIVE_SHOWCASE)
    add_executable(plussview_window_popup_showcase_demo
        ${CMAKE_CURRENT_SOURCE_DIR}/examples/window/popup_showcase_demo.cpp
    )
    target_link_libraries(plussview_window_popup_showcase_demo PRIVATE PlussView)
    set_target_properties(plussview_window_popup_showcase_demo PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
    )
endif()

install(TARGETS PlussView
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
