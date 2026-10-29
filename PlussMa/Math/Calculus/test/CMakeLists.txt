find_package(Catch2 3 QUIET)

if(TARGET Catch2::Catch2WithMain)
    add_executable(plussma_calculus_contract_test
        calculus_contract_test.cpp
        function_execution_test.cpp
        operator_execution_test.cpp
        differential_execution_test.cpp
        multivariable_execution_test.cpp
        simplification_domain_test.cpp
        integral_execution_test.cpp
        system_integration_test.cpp
        production_closure_test.cpp
        tensor_primitives_test.cpp
        geometry_operators_test.cpp
        stochastic_operators_test.cpp
        taylor_operators_test.cpp
        fourier_series_operators_test.cpp
        integral_transform_operators_test.cpp
        measure_operators_test.cpp
        malliavin_operators_test.cpp
        extension_closure_test.cpp
        complex_calculus_test.cpp
    )
    target_link_libraries(plussma_calculus_contract_test
        PRIVATE
            pluss
            plussqit_codem
            Catch2::Catch2WithMain
    )
    target_compile_features(plussma_calculus_contract_test PRIVATE cxx_std_20)
    add_test(NAME plussma_calculus_contract COMMAND plussma_calculus_contract_test)
else()
    message(STATUS "Skipping plussma_calculus_contract_test: Catch2::Catch2WithMain not found")
endif()

add_test(
    NAME plussma_calculus_public_surface_audit
    COMMAND ${CMAKE_COMMAND}
        -DPLUSS_CALCULUS_INCLUDE_DIR=${PROJECT_SOURCE_DIR}/include
        -P ${CMAKE_CURRENT_SOURCE_DIR}/audit_public_surface.cmake
)
