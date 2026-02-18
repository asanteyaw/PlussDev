#pragma once

// This header is included *inside* class pluss::codem::Series.
// It carries the bulk "op surface" (generated wrappers + operator sugar).
//
// If you want to ensure a TU never accidentally drags this surface in,
// define CODEM_ASSERT_NO_SERIES_SURFACE before including Series.h, or include
// SeriesSlim.h which disables this surface entirely.

#ifdef CODEM_ASSERT_NO_SERIES_SURFACE
#error "Series op surface is disabled for this translation unit. Include CodeM/core/SeriesBase.h or CodeM/core/SeriesSlim.h instead of Series.h, or undef CODEM_ASSERT_NO_SERIES_SURFACE."
#endif

// Generated Torch-style templated wrappers (collect args and dispatch via _compat::invoke).
#include "CodeM/generated/SeriesMethodsGen.h"

// ---- C++ convenience operators (ergonomic sugar) ----
// Thin wrappers around the generated method surface.
// NOTE: We only provide Series-vs-Series operators here. Scalar operators
// can be added later once CodeM scalar semantics are finalized.

// Out-of-place arithmetic
Series operator+(const Series& other) const { return add(other); }
Series operator-(const Series& other) const { return sub(other); }
Series operator*(const Series& other) const { return mul(other); }
Series operator/(const Series& other) const { return div(other); }

// In-place arithmetic
Series& operator+=(const Series& other) { add_(other); return *this; }
Series& operator-=(const Series& other) { sub_(other); return *this; }
Series& operator*=(const Series& other) { mul_(other); return *this; }
Series& operator/=(const Series& other) { div_(other); return *this; }

// In-place bitwise
Series& operator&=(const Series& other) { bitwise_and_(other); return *this; }
Series& operator|=(const Series& other) { bitwise_or_(other); return *this; }
Series& operator^=(const Series& other) { bitwise_xor_(other); return *this; }

// Comparison operators (mask-producing, Torch-like)
Series operator==(const Series& other) const { return eq(other); }
Series operator!=(const Series& other) const { return ne(other); }
Series operator<(const Series& other) const { return lt(other); }
Series operator<=(const Series& other) const { return le(other); }
Series operator>(const Series& other) const { return gt(other); }
Series operator>=(const Series& other) const { return ge(other); }
