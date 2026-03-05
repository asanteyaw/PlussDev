cmake_minimum_required(VERSION 3.28)
project(PlussPl_tests NONE)

find_package(Catch2 3 QUIET)

if(TARGET Catch2::Catch2WithMain)
    add_executable(plusspl_catch2
        ${CMAKE_CURRENT_SOURCE_DIR}/plusspl_catch2.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/plusspl_matlab_parity.cpp
    )
    target_link_libraries(plusspl_catch2 PRIVATE PlussPl Catch2::Catch2WithMain)
    target_compile_features(plusspl_catch2 PRIVATE cxx_std_20)
    include(${Catch2_DIR}/Catch.cmake)
    catch_discover_tests(plusspl_catch2)
else()
    add_executable(plusspl_core_smoke
        ${CMAKE_CURRENT_SOURCE_DIR}/plusspl_core_smoke.cpp
    )
    target_link_libraries(plusspl_core_smoke PRIVATE PlussPl)
    add_test(NAME plusspl_core_smoke COMMAND plusspl_core_smoke)

    add_executable(plusspl_pdf_text_smoke
        ${CMAKE_CURRENT_SOURCE_DIR}/plusspl_pdf_text_smoke.cpp
    )
    target_link_libraries(plusspl_pdf_text_smoke PRIVATE PlussPl)
    add_test(NAME plusspl_pdf_text_smoke COMMAND plusspl_pdf_text_smoke)
endif()
