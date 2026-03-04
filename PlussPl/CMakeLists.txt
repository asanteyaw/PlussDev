cmake_minimum_required(VERSION 3.28)
project(PlussPl LANGUAGES CXX)

option(PLUSSPL_ENABLE_INTERACTIVE_SHOW "Enable graphical viewer launch from Figure::show()" OFF)
option(PLUSSPL_ENABLE_POPUP_VIEWER "Enable embedded popup viewer backends for plots when dependencies are available" ON)
option(PLUSSPL_BUILD_EXAMPLES "Build PlussPl demos" ON)
option(PLUSSPL_BUILD_TESTS "Build PlussPl tests" ON)

add_library(PlussPl STATIC)

set(PLUSSPL_HAVE_GTK_WEBKIT_VALUE 0)
set(PLUSSPL_HAVE_MAC_WEBKIT_VALUE 0)

target_include_directories(PlussPl PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../PlussQit/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../PlussQit/src
    ${CMAKE_BINARY_DIR}/generated
    ${CMAKE_CURRENT_SOURCE_DIR}/../ExLibs/fmt/include
)

set(PLUSSPL_QIT_DEPS "")
if(TARGET PlussQit::CodeM)
    list(APPEND PLUSSPL_QIT_DEPS PlussQit::CodeM)
endif()
if(TARGET PlussQit::CodeN)
    list(APPEND PLUSSPL_QIT_DEPS PlussQit::CodeN)
endif()
if(TARGET PlussQit::CodeH)
    list(APPEND PLUSSPL_QIT_DEPS PlussQit::CodeH)
endif()

if(PLUSSPL_QIT_DEPS)
    target_link_libraries(PlussPl PUBLIC ${PLUSSPL_QIT_DEPS})
elseif(TARGET PlussQit)
    target_link_libraries(PlussPl PUBLIC PlussQit)
endif()

target_sources(PlussPl PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/plot/viewer/PopupViewer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/plot/viewer/PlatformPopupViewer.h
)

set_target_properties(PlussPl PROPERTIES CXX_STANDARD 20)
target_compile_features(PlussPl PUBLIC cxx_std_20)
target_compile_definitions(PlussPl PUBLIC FMT_HEADER_ONLY=1)

if(PLUSSPL_ENABLE_POPUP_VIEWER AND PkgConfig_FOUND)
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

if(APPLE AND PLUSSPL_ENABLE_POPUP_VIEWER)
    enable_language(OBJCXX)
    set(PLUSSPL_HAVE_MAC_WEBKIT_VALUE 1)
    target_sources(PlussPl PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/plot/viewer/MacPopupViewer.mm
    )
    set_source_files_properties(
        ${CMAKE_CURRENT_SOURCE_DIR}/src/plot/viewer/MacPopupViewer.mm
        PROPERTIES COMPILE_OPTIONS "-fobjc-arc"
    )
    target_link_libraries(PlussPl PRIVATE
        "-framework Cocoa"
        "-framework UniformTypeIdentifiers"
        "-framework WebKit"
        "-framework QuartzCore"
    )
endif()

target_compile_definitions(PlussPl PUBLIC PLUSSPL_HAVE_GTK_WEBKIT=${PLUSSPL_HAVE_GTK_WEBKIT_VALUE})
target_compile_definitions(PlussPl PUBLIC PLUSSPL_HAVE_MAC_WEBKIT=${PLUSSPL_HAVE_MAC_WEBKIT_VALUE})

if(PLUSSPL_ENABLE_INTERACTIVE_SHOW)
    target_compile_definitions(PlussPl PUBLIC PLUSSPL_ENABLE_INTERACTIVE_SHOW=1)
    if(TARGET pluss)
        target_compile_definitions(pluss PUBLIC PLUSSPL_ENABLE_INTERACTIVE_SHOW=1)
    endif()
else()
    target_compile_definitions(PlussPl PUBLIC PLUSSPL_ENABLE_INTERACTIVE_SHOW=0)
    if(TARGET pluss)
        target_compile_definitions(pluss PUBLIC PLUSSPL_ENABLE_INTERACTIVE_SHOW=0)
    endif()
endif()

if(PLUSSPL_BUILD_EXAMPLES AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples/CMakeLists.txt")
    add_subdirectory(examples)
endif()

if(PLUSSPL_BUILD_TESTS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
    add_subdirectory(tests)
endif()
