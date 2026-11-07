#pragma once

#include "pluss/tils/kit/scalar_cast.h"

namespace pluss {
using tils::as_bool;
using tils::as_c128;
using tils::as_c64;
using tils::as_dtype;
using tils::as_f32;
using tils::as_f64;
using tils::as_i16;
using tils::as_i32;
using tils::as_i64;
using tils::as_i8;
using tils::as_u16;
using tils::as_u32;
using tils::as_u64;
using tils::as_u8;
using tils::to_scalar;
} // namespace pluss

// Compatibility conveniences for code written against the original scalar
// helper surface. The implementations and overload sets remain centralized in
// pluss::tils.
using pluss::as_bool;
using pluss::as_c128;
using pluss::as_c64;
using pluss::as_dtype;
using pluss::as_f32;
using pluss::as_f64;
using pluss::as_i16;
using pluss::as_i32;
using pluss::as_i64;
using pluss::as_i8;
using pluss::as_u16;
using pluss::as_u32;
using pluss::as_u64;
using pluss::as_u8;
using pluss::to_scalar;
