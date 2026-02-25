#pragma once

#include "aux/let.h"
#include "aux/transmit.h"
#include "aux/translate.h"

namespace pluss {
using aux::transmit;
using aux::transmit_exec;
using aux::transmit_into;
using aux::transmit_options;
using aux::transform_axis;
using aux::transform_axis_options;
using aux::suppose;
} // namespace pluss

// Convenience unqualified aliases for end users.
using pluss::aux::suppose;

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
