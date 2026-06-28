cmake_minimum_required(VERSION 3.28)
project(PlussTilsView LANGUAGES CXX)
include(GNUInstallDirs)

option(PLUSSTILS_VIEW_ENABLE_GTK_VIEWER "Enable GTK3/WebKit HTML window support in PlussTilsView" ON)
option(PLUSSTILS_VIEW_ENABLE_EXPERIMENTAL_NATIVE_SHOWCASE
       "Build native popup showcase demo before full cross-platform window parity"
       OFF)

add_library(PlussTilsView STATIC
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
    ${CMAKE_CURRENT_SOURCE_DIR}/src/window/html/html_scroll.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/window/text/text_layout.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/InHouseHtmlViewer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/PlatformHtmlViewer.h
)

add_library(Pluss::TilsView ALIAS PlussTilsView)

if(NOT PkgConfig_FOUND)
    find_package(PkgConfig QUIET)
endif()

set(PLUSSTILS_VIEW_HAVE_GTK_WEBKIT_VALUE 0)
set(PLUSSTILS_VIEW_HAVE_MAC_WEBKIT_VALUE 0)
set(PLUSSTILS_VIEW_HAVE_WIN_WEBVIEW2_VALUE 0)

if(UNIX AND NOT APPLE AND PLUSSTILS_VIEW_ENABLE_GTK_VIEWER AND PkgConfig_FOUND)
    pkg_check_modules(PLUSSTILS_VIEW_GTK3 QUIET gtk+-3.0)
    pkg_check_modules(PLUSSTILS_VIEW_WEBKIT41 QUIET webkit2gtk-4.1)
    if(NOT PLUSSTILS_VIEW_WEBKIT41_FOUND)
        pkg_check_modules(PLUSSTILS_VIEW_WEBKIT40 QUIET webkit2gtk-4.0)
    endif()

    set(PLUSSTILS_VIEW_WEBKIT_FOUND FALSE)
    set(PLUSSTILS_VIEW_WEBKIT_INCLUDE_DIRS "")
    set(PLUSSTILS_VIEW_WEBKIT_LIBRARIES "")
    set(PLUSSTILS_VIEW_WEBKIT_LIBRARY_DIRS "")
    set(PLUSSTILS_VIEW_WEBKIT_CFLAGS_OTHER "")

    if(PLUSSTILS_VIEW_WEBKIT41_FOUND)
        set(PLUSSTILS_VIEW_WEBKIT_FOUND TRUE)
        set(PLUSSTILS_VIEW_WEBKIT_INCLUDE_DIRS ${PLUSSTILS_VIEW_WEBKIT41_INCLUDE_DIRS})
        set(PLUSSTILS_VIEW_WEBKIT_LIBRARIES ${PLUSSTILS_VIEW_WEBKIT41_LIBRARIES})
        set(PLUSSTILS_VIEW_WEBKIT_LIBRARY_DIRS ${PLUSSTILS_VIEW_WEBKIT41_LIBRARY_DIRS})
        set(PLUSSTILS_VIEW_WEBKIT_CFLAGS_OTHER ${PLUSSTILS_VIEW_WEBKIT41_CFLAGS_OTHER})
    elseif(PLUSSTILS_VIEW_WEBKIT40_FOUND)
        set(PLUSSTILS_VIEW_WEBKIT_FOUND TRUE)
        set(PLUSSTILS_VIEW_WEBKIT_INCLUDE_DIRS ${PLUSSTILS_VIEW_WEBKIT40_INCLUDE_DIRS})
        set(PLUSSTILS_VIEW_WEBKIT_LIBRARIES ${PLUSSTILS_VIEW_WEBKIT40_LIBRARIES})
        set(PLUSSTILS_VIEW_WEBKIT_LIBRARY_DIRS ${PLUSSTILS_VIEW_WEBKIT40_LIBRARY_DIRS})
        set(PLUSSTILS_VIEW_WEBKIT_CFLAGS_OTHER ${PLUSSTILS_VIEW_WEBKIT40_CFLAGS_OTHER})
    endif()

    if(PLUSSTILS_VIEW_GTK3_FOUND AND PLUSSTILS_VIEW_WEBKIT_FOUND)
        set(PLUSSTILS_VIEW_HAVE_GTK_WEBKIT_VALUE 1)
        target_sources(PlussTilsView PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/LinuxHtmlViewer.cpp
        )
        target_include_directories(PlussTilsView SYSTEM PRIVATE
            ${PLUSSTILS_VIEW_GTK3_INCLUDE_DIRS}
            ${PLUSSTILS_VIEW_WEBKIT_INCLUDE_DIRS}
        )
        target_link_directories(PlussTilsView PRIVATE
            ${PLUSSTILS_VIEW_GTK3_LIBRARY_DIRS}
            ${PLUSSTILS_VIEW_WEBKIT_LIBRARY_DIRS}
        )
        target_compile_options(PlussTilsView PRIVATE
            ${PLUSSTILS_VIEW_GTK3_CFLAGS_OTHER}
            ${PLUSSTILS_VIEW_WEBKIT_CFLAGS_OTHER}
        )
        target_link_libraries(PlussTilsView PRIVATE
            ${PLUSSTILS_VIEW_GTK3_LIBRARIES}
            ${PLUSSTILS_VIEW_WEBKIT_LIBRARIES}
        )
    endif()
endif()

if(APPLE)
    enable_language(OBJCXX)
    set(PLUSSTILS_VIEW_HAVE_MAC_WEBKIT_VALUE 1)
    target_sources(PlussTilsView PRIVATE
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
    target_link_libraries(PlussTilsView PRIVATE
        "-framework Cocoa"
        "-framework UniformTypeIdentifiers"
        "-framework WebKit"
        "-framework QuartzCore"
        "-framework CoreText"
        "-framework CoreFoundation"
        "-framework CoreGraphics"
    )
endif()

if(WIN32)
    option(PLUSSTILS_VIEW_ENABLE_WINDOWS_WEBVIEW2 "Enable WebView2 HTML window support in PlussTilsView" ON)
    set(PLUSSTILS_VIEW_WEBVIEW2_SDK_DIR "" CACHE PATH "Path to the Microsoft.Web.WebView2 SDK root")

    if(PLUSSTILS_VIEW_ENABLE_WINDOWS_WEBVIEW2)
        set(PLUSSTILS_VIEW_WEBVIEW2_HINTS ${PLUSSTILS_VIEW_WEBVIEW2_SDK_DIR})
        if(DEFINED ENV{WEBVIEW2_SDK_DIR} AND NOT "$ENV{WEBVIEW2_SDK_DIR}" STREQUAL "")
            list(APPEND PLUSSTILS_VIEW_WEBVIEW2_HINTS "$ENV{WEBVIEW2_SDK_DIR}")
        endif()

        if(DEFINED ENV{USERPROFILE} AND NOT "$ENV{USERPROFILE}" STREQUAL "")
            file(GLOB PLUSSTILS_VIEW_WEBVIEW2_NUGET_DIRS
                 "$ENV{USERPROFILE}/.nuget/packages/microsoft.web.webview2/*")
            list(REVERSE PLUSSTILS_VIEW_WEBVIEW2_NUGET_DIRS)
            list(APPEND PLUSSTILS_VIEW_WEBVIEW2_HINTS ${PLUSSTILS_VIEW_WEBVIEW2_NUGET_DIRS})
        endif()

        find_path(PLUSSTILS_VIEW_WEBVIEW2_INCLUDE_DIR
                  NAMES WebView2.h
                  HINTS ${PLUSSTILS_VIEW_WEBVIEW2_HINTS}
                  PATH_SUFFIXES build/native/include include)

        set(PLUSSTILS_VIEW_WEBVIEW2_ARCH_HINTS x64)
        if(CMAKE_SYSTEM_PROCESSOR MATCHES "^[Aa][Rr][Mm]64$")
            set(PLUSSTILS_VIEW_WEBVIEW2_ARCH_HINTS arm64 x64)
        elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
            set(PLUSSTILS_VIEW_WEBVIEW2_ARCH_HINTS x86)
        endif()

        unset(PLUSSTILS_VIEW_WEBVIEW2_LOADER CACHE)
        foreach(PLUSSTILS_VIEW_WEBVIEW2_ARCH IN LISTS PLUSSTILS_VIEW_WEBVIEW2_ARCH_HINTS)
            if(NOT PLUSSTILS_VIEW_WEBVIEW2_LOADER)
                find_library(PLUSSTILS_VIEW_WEBVIEW2_LOADER
                             NAMES WebView2LoaderStatic WebView2Loader
                             HINTS ${PLUSSTILS_VIEW_WEBVIEW2_HINTS}
                             PATH_SUFFIXES build/native/${PLUSSTILS_VIEW_WEBVIEW2_ARCH} ${PLUSSTILS_VIEW_WEBVIEW2_ARCH} lib)
            endif()
        endforeach()

        if(PLUSSTILS_VIEW_WEBVIEW2_INCLUDE_DIR AND PLUSSTILS_VIEW_WEBVIEW2_LOADER)
            set(PLUSSTILS_VIEW_HAVE_WIN_WEBVIEW2_VALUE 1)
            target_include_directories(PlussTilsView SYSTEM PRIVATE ${PLUSSTILS_VIEW_WEBVIEW2_INCLUDE_DIR})
            target_sources(PlussTilsView PRIVATE
                ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/WindowsHtmlViewer.cpp
            )
            target_link_libraries(PlussTilsView PRIVATE ${PLUSSTILS_VIEW_WEBVIEW2_LOADER} ole32 comdlg32 shlwapi)
        endif()
    endif()
endif()

target_compile_definitions(PlussTilsView PUBLIC
    PLUSSTILS_VIEW_HAVE_GTK_WEBKIT=${PLUSSTILS_VIEW_HAVE_GTK_WEBKIT_VALUE}
    PLUSSTILS_VIEW_HAVE_MAC_WEBKIT=${PLUSSTILS_VIEW_HAVE_MAC_WEBKIT_VALUE}
    PLUSSTILS_VIEW_HAVE_WIN_WEBVIEW2=${PLUSSTILS_VIEW_HAVE_WIN_WEBVIEW2_VALUE}
)

target_include_directories(PlussTilsView PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_include_directories(PlussTilsView PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

set_target_properties(PlussTilsView PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

add_executable(plusstils_view_window_runtime_demo
    ${CMAKE_CURRENT_SOURCE_DIR}/examples/window/runtime_demo.cpp
)
target_link_libraries(plusstils_view_window_runtime_demo PRIVATE PlussTilsView)
set_target_properties(plusstils_view_window_runtime_demo PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

add_executable(plusstils_view_window_first_plot_demo
    ${CMAKE_CURRENT_SOURCE_DIR}/examples/window/first_plot_demo.cpp
)
target_link_libraries(plusstils_view_window_first_plot_demo PRIVATE PlussTilsView)
set_target_properties(plusstils_view_window_first_plot_demo PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

add_executable(plusstils_view_window_interaction_core_demo
    ${CMAKE_CURRENT_SOURCE_DIR}/examples/window/interaction_core_demo.cpp
)
target_link_libraries(plusstils_view_window_interaction_core_demo PRIVATE PlussTilsView)
set_target_properties(plusstils_view_window_interaction_core_demo PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

add_executable(plusstils_view_window_session_demo
    ${CMAKE_CURRENT_SOURCE_DIR}/examples/window/session_demo.cpp
)
target_link_libraries(plusstils_view_window_session_demo PRIVATE PlussTilsView)
set_target_properties(plusstils_view_window_session_demo PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

add_executable(plusstils_view_window_registry_demo
    ${CMAKE_CURRENT_SOURCE_DIR}/examples/window/registry_demo.cpp
)
target_link_libraries(plusstils_view_window_registry_demo PRIVATE PlussTilsView)
set_target_properties(plusstils_view_window_registry_demo PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

add_executable(plusstils_view_window_table_demo
    ${CMAKE_CURRENT_SOURCE_DIR}/examples/window/table_demo.cpp
)
target_link_libraries(plusstils_view_window_table_demo PRIVATE PlussTilsView)
set_target_properties(plusstils_view_window_table_demo PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

add_executable(plusstils_view_window_export_smoke_demo
    ${CMAKE_CURRENT_SOURCE_DIR}/examples/window/export_smoke_demo.cpp
)
target_link_libraries(plusstils_view_window_export_smoke_demo PRIVATE PlussTilsView)
set_target_properties(plusstils_view_window_export_smoke_demo PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

add_executable(plusstils_view_window_shell_backend_demo
    ${CMAKE_CURRENT_SOURCE_DIR}/examples/window/shell_backend_demo.cpp
)
target_link_libraries(plusstils_view_window_shell_backend_demo PRIVATE PlussTilsView)
set_target_properties(plusstils_view_window_shell_backend_demo PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

add_executable(plusstils_view_inhouse_html_render_demo
    ${CMAKE_CURRENT_SOURCE_DIR}/examples/window/inhouse_html_render_demo.cpp
)
target_link_libraries(plusstils_view_inhouse_html_render_demo PRIVATE PlussTilsView)
set_target_properties(plusstils_view_inhouse_html_render_demo PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

add_executable(plusstils_view_inhouse_scroll_showcase
    ${CMAKE_CURRENT_SOURCE_DIR}/examples/window/inhouse_scroll_showcase.cpp
)
target_link_libraries(plusstils_view_inhouse_scroll_showcase PRIVATE PlussTilsView)
set_target_properties(plusstils_view_inhouse_scroll_showcase PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

add_executable(plusstils_view_plussmic_inhouse_showcase
    ${CMAKE_CURRENT_SOURCE_DIR}/examples/window/plussmic_inhouse_showcase.cpp
)
target_link_libraries(plusstils_view_plussmic_inhouse_showcase PRIVATE PlussTilsView)
set_target_properties(plusstils_view_plussmic_inhouse_showcase PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

if(PLUSSTILS_VIEW_ENABLE_EXPERIMENTAL_NATIVE_SHOWCASE)
    add_executable(plusstils_view_window_popup_showcase_demo
        ${CMAKE_CURRENT_SOURCE_DIR}/examples/window/popup_showcase_demo.cpp
    )
    target_link_libraries(plusstils_view_window_popup_showcase_demo PRIVATE PlussTilsView)
    set_target_properties(plusstils_view_window_popup_showcase_demo PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
    )
endif()

if(BUILD_TESTING)
    add_executable(plussview_html_scroll_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/test/html_scroll_tests.cpp
    )
    target_link_libraries(plussview_html_scroll_tests PRIVATE PlussTilsView)
    set_target_properties(plussview_html_scroll_tests PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
    )
    add_test(NAME plussview_html_scroll_tests COMMAND plussview_html_scroll_tests)

    if(APPLE)
        add_executable(plussview_macos_input_driver
            ${CMAKE_CURRENT_SOURCE_DIR}/test/macos_input_driver.mm
        )
        target_link_libraries(plussview_macos_input_driver PRIVATE
            "-framework AppKit"
            "-framework CoreGraphics"
        )
        set_target_properties(plussview_macos_input_driver PROPERTIES
            CXX_STANDARD 20
            CXX_STANDARD_REQUIRED ON
            CXX_EXTENSIONS OFF
        )
    endif()
endif()

install(TARGETS PlussTilsView
    EXPORT PlussTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
