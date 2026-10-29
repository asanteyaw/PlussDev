file(READ
    "${PLUSS_CALCULUS_INCLUDE_DIR}/pluss/subsp/math/function.h"
    function_header)
file(READ
    "${PLUSS_CALCULUS_INCLUDE_DIR}/pluss/subsp/math/operators.h"
    operator_header)
file(READ
    "${PLUSS_CALCULUS_INCLUDE_DIR}/pluss/ma/math.h"
    umbrella_header)

set(public_headers "${function_header}\n${operator_header}")
set(forbidden_types
    Variable
    Expression
    FunctionVector
    FunctionMatrix
    Differential
    Partial
    Gradient
    Jacobian
    Hessian
    Integral
    Shift
    Scale
    Multiplication
    Builder
    Measure
    MeasureSpec
    Path
    Filtration
    Process
    Connection
    Metric
    Tensor
    Brownian
    Manifold
    Transform
    Lagrangian
    Fractional
    Vector)

foreach(type_name IN LISTS forbidden_types)
    if(public_headers MATCHES "(class|struct)[ \t\r\n]+${type_name}([ \t\r\n:{]|$)")
        message(FATAL_ERROR
            "Forbidden public calculus type declared: ${type_name}")
    endif()
endforeach()

foreach(type_name IN LISTS forbidden_types)
    if(umbrella_header MATCHES
       "using[ \t\r\n]+${type_name}[ \t\r\n]*=")
        message(FATAL_ERROR
            "Forbidden root calculus type alias declared: ${type_name}")
    endif()
endforeach()

# Exactly three public classes: Function and Operator, plus the Trace builder in `detail`. Everything else
# (options, signatures, enums) is plain data.
set(public_classes "")
foreach(header_text IN ITEMS "${function_header}" "${operator_header}")
    string(REGEX MATCHALL "(^|\n)[ \t]*class[ \t]+[A-Za-z_]+[ \t]+final" found "${header_text}")
    foreach(match IN LISTS found)
        string(REGEX REPLACE ".*class[ \t]+([A-Za-z_]+)[ \t]+final" "\\1" name "${match}")
        list(APPEND public_classes "${name}")
    endforeach()
endforeach()
list(SORT public_classes)
set(expected_classes Function Operator Trace)
if(NOT public_classes STREQUAL expected_classes)
    message(FATAL_ERROR
        "The math and operators headers must declare exactly the classes ${expected_classes}; found: ${public_classes}")
endif()

# New factories must be plain functions in these namespaces, not types.
foreach(factory IN ITEMS divergence curl laplacian directional material adjoint convolution
                         malliavin_derivative skorokhod_integral functional_derivative euler_lagrange
                         inverse_transform lebesgue weighted atomic stieltjes)
    if(NOT operator_header MATCHES "Operator[ \t\r\n]+${factory}\\(")
        message(FATAL_ERROR "Expected public factory missing from operators.h: ${factory}")
    endif()
endforeach()
