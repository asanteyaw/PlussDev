#pragma once

#include "pluss/tils/aux/let.h"
#include "pluss/tils/aux/data_loader.h"
#include "pluss/tils/aux/output.h"
#include "pluss/tils/aux/pathing.h"
#include "pluss/tils/aux/scalar_cast.h"
#include "pluss/tils/aux/rng_seed.h"
#include "pluss/tils/aux/transmit.h"
#include "pluss/tils/aux/translate.h"

namespace pluss {
using aux::transmit;
using aux::transmit_exec;
using aux::transmit_into;
using aux::transmit_options;
using aux::transform_axis;
using aux::transform_axis_options;
using aux::suppose;
using aux::rng_seed;
using aux::OutputEngine;
using aux::OutputOptions;
using aux::OutputLevel;
using aux::to_scalar;
using aux::as_bool;
using aux::as_f32;
using aux::as_f64;
using aux::as_i16;
using aux::as_i32;
using aux::as_i64;
using aux::as_u16;
using aux::as_u32;
using aux::as_u64;
using aux::as_I16;
using aux::as_I32;
using aux::as_I64;
using aux::as_U16;
using aux::as_U32;
using aux::as_U64;
using DataLoader = loader::DataLoader;
} // namespace pluss

// Convenience unqualified aliases for end users.
using pluss::aux::suppose;
using pluss::f32_t;
using pluss::f64_t;
using pluss::i8_t;
using pluss::i16_t;
using pluss::i32_t;
using pluss::i64_t;
using pluss::u8_t;
using pluss::u16_t;
using pluss::u32_t;
using pluss::u64_t;
using pluss::to_scalar;
using pluss::as_bool;
using pluss::as_f32;
using pluss::as_f64;
using pluss::as_i16;
using pluss::as_i32;
using pluss::as_i64;
using pluss::as_u16;
using pluss::as_u32;
using pluss::as_u64;
using pluss::as_I16;
using pluss::as_I32;
using pluss::as_I64;
using pluss::as_U16;
using pluss::as_U32;
using pluss::as_U64;

// Optional pseudo-keyword for declarations:
//   let x = expr;  // expands to: auto x = expr;
//
// Constrained variant declaration helper:
//   let_as(int, float) price = 2.5f;
// `price` can hold only one of the listed types.
//
// To disable in a TU before including <pluss/pluss.h>, define:
//   #define PLUSS_DISABLE_LET_AUTO
#ifndef PLUSS_DISABLE_LET_AUTO
#ifndef let
#define let auto
#endif
#endif

#ifndef let_as
#define let_as(...) ::pluss::aux::let_one_of<__VA_ARGS__>
#endif
