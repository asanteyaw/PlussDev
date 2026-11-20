#pragma once

// The single definition of "which cells are NaN, which are infinite" for the table, kindle and scribble types.
//
// Three different things can sit in a numeric cell and they mean three different things:
//   NULL         the cell has no value (the validity mask says so). isnull / notnull / dropnull / fillnull.
//   NaN          the cell has a value and it is "not a number" (0/0, x % 0, sqrt(-1)). isnan / notnan / dropnan / fillnan.
//   +inf, -inf   the cell has a value and it is unbounded (5/0, log(0)). isinf / notinf / dropinf / fillinf.
// A NULL cell is none of NaN or inf (it holds no value to be one), and a column that cannot hold NaN or inf (integers, bool,
// text, time) has no NaN or inf cell. Shelf wraps these over a Series; DataFrame applies them to a matrix of same-typed
// columns at once; every other type delegates to one of those two, so they all agree by construction.

#include "pluss/pluss.h"

#include <cstdint>
#include <optional>

namespace pluss::table::nonfinite {

enum class Kind : std::uint8_t { Nan, Inf, PosInf, NegInf };  // Inf is +inf or -inf; PosInf / NegInf pick one sign

// Whether a Series can hold NaN or infinity at all (floating point and complex).
[[nodiscard]] inline bool can_hold(const pluss::Series& values) {
  const auto dtype = values.dtype();
  return dtype.is_floating() || dtype.is_complex();
}

// Bool Series of the shape of `values`: true where the cell is NaN (Kind::Nan) or +-inf (Kind::Inf).
// `present` is the validity mask (same shape, Bool) or empty when no cell is null; a null cell is never flagged.
[[nodiscard]] inline pluss::Series flags(const pluss::Series& values,
                                         Kind kind,
                                         const std::optional<pluss::Series>& present) {
  if (!can_hold(values)) {
    return pluss::Series::zeros(values.sizes(), pluss::codem::ScalarType::Bool, values.platform());
  }
  auto out = [&] {
    switch (kind) {
      case Kind::Nan: return values.isnan();
      case Kind::Inf: return values.isinf();
      case Kind::PosInf: return values.isposinf();
      case Kind::NegInf: return values.isneginf();
    }
    return values.isnan();
  }();
  if (present.has_value()) {
    out = out.logical_and(present->platform() == out.platform() ? *present : present->to(out.platform()));
  }
  return out;
}

// `values` with every flagged cell replaced by `value`. A Series that cannot hold NaN or infinity is returned as it is.
[[nodiscard]] inline pluss::Series fill(const pluss::Series& values, const pluss::Series& flagged, double value) {
  if (!can_hold(values)) {
    return values;
  }
  return pluss::series::where(flagged, value, values);
}

} // namespace pluss::table::nonfinite
