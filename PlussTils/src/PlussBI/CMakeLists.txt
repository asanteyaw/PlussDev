cmake_minimum_required(VERSION 3.28)
project(PlussBI LANGUAGES CXX)
include(GNUInstallDirs)

add_library(PlussBI STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/BrowserExport.cpp
)
add_library(Pluss::BI ALIAS PlussBI)

target_include_directories(PlussBI PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

set_target_properties(PlussBI PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

install(TARGETS PlussBI
    EXPORT PlussTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
