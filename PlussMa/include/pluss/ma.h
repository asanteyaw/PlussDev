#pragma once

// PlussMa umbrella include across Math/Stats/Eng/NatSci subspaces.
#include "pluss/ma/math.h"
#include "pluss/ma/stats.h"
#include "pluss/ma/eng.h"
#include "pluss/ma/natsci.h"
#include "pluss/ma/actuarial.h"

// Unqualified scalar helpers (as_f64(series), as_i32(series), ...) and the f64_t / i64_t typedefs, as in the full SDK.
#if __has_include("pluss/tils/auxiliary/scalar_cast_compat.h")
#include "pluss/tils/auxiliary/scalar_cast_compat.h"
#endif
