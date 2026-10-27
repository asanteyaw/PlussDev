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
    Builder)

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
