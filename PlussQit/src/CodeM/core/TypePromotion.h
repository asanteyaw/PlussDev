#pragma once
#include "CodeM/core/ScalarType.h"

namespace pluss::codem {

// Torch-inspired (but simplified) type promotion rules for CPU-only.
//
// - bool participates in arithmetic by promoting to int64 when mixed with integral;
//   and to float32 when mixed with floating.
// - integer + floating -> floating (float32/float64 depending on highest).
// - float16/bfloat16 are treated as "floating" but compute type is float32.
//
// NOTE: This does NOT implement the full Torch lattice (esp. complex + float8 nuances),
// but provides a single, auditable place to evolve toward parity.
ScalarType promote_types(ScalarType a, ScalarType b);

} // namespace pluss::codem
