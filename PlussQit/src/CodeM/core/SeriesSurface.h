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

// Policy-only guards: textual overloads are explicitly rejected.
// Concrete numeric overloads are generated in SeriesMethodsGen.h.
Series pdf(std::string_view) const = delete;
Series pdf(const std::string&) const = delete;
Series pdf(const char*) const = delete;

Series logpdf(std::string_view) const = delete;
Series logpdf(const std::string&) const = delete;
Series logpdf(const char*) const = delete;

Series logcdf(std::string_view) const = delete;
Series logcdf(const std::string&) const = delete;
Series logcdf(const char*) const = delete;

Series cdf(std::string_view) const = delete;
Series cdf(const std::string&) const = delete;
Series cdf(const char*) const = delete;

// ---- C++ convenience operators (ergonomic sugar) ----
// Thin wrappers around the generated method surface.
// Scalar rhs operators forward to the generated scalar-aware method surface.
// Matching lhs scalar overloads are provided as free functions in codem.h.

// Out-of-place arithmetic
Series operator-() const { return neg(); }
Series operator+(const Series& other) const { return add(other); }
Series operator-(const Series& other) const { return sub(other); }
Series operator*(const Series& other) const { return mul(other); }
Series operator/(const Series& other) const { return div(other); }
template <class T>
requires(std::is_arithmetic_v<std::remove_cv_t<T>> && !std::is_same_v<std::remove_cv_t<T>, bool>)
Series operator+(T scalar) const { return add(static_cast<double>(scalar)); }
template <class T>
requires(std::is_arithmetic_v<std::remove_cv_t<T>> && !std::is_same_v<std::remove_cv_t<T>, bool>)
Series operator-(T scalar) const { return sub(static_cast<double>(scalar)); }
template <class T>
requires(std::is_arithmetic_v<std::remove_cv_t<T>> && !std::is_same_v<std::remove_cv_t<T>, bool>)
Series operator*(T scalar) const { return mul(static_cast<double>(scalar)); }
template <class T>
requires(std::is_arithmetic_v<std::remove_cv_t<T>> && !std::is_same_v<std::remove_cv_t<T>, bool>)
Series operator/(T scalar) const { return div(static_cast<double>(scalar)); }

// In-place arithmetic
Series& operator+=(const Series& other) { add_(other); return *this; }
Series& operator-=(const Series& other) { sub_(other); return *this; }
Series& operator*=(const Series& other) { mul_(other); return *this; }
Series& operator/=(const Series& other) { div_(other); return *this; }
template <class T>
requires(std::is_arithmetic_v<std::remove_cv_t<T>> && !std::is_same_v<std::remove_cv_t<T>, bool>)
Series& operator+=(T scalar) { add_(static_cast<double>(scalar)); return *this; }
template <class T>
requires(std::is_arithmetic_v<std::remove_cv_t<T>> && !std::is_same_v<std::remove_cv_t<T>, bool>)
Series& operator-=(T scalar) { sub_(static_cast<double>(scalar)); return *this; }
template <class T>
requires(std::is_arithmetic_v<std::remove_cv_t<T>> && !std::is_same_v<std::remove_cv_t<T>, bool>)
Series& operator*=(T scalar) { mul_(static_cast<double>(scalar)); return *this; }
template <class T>
requires(std::is_arithmetic_v<std::remove_cv_t<T>> && !std::is_same_v<std::remove_cv_t<T>, bool>)
Series& operator/=(T scalar) { div_(static_cast<double>(scalar)); return *this; }

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
template <class T>
requires(std::is_arithmetic_v<std::remove_cv_t<T>> && !std::is_same_v<std::remove_cv_t<T>, bool>)
Series operator==(T scalar) const { return eq(static_cast<double>(scalar)); }
template <class T>
requires(std::is_arithmetic_v<std::remove_cv_t<T>> && !std::is_same_v<std::remove_cv_t<T>, bool>)
Series operator!=(T scalar) const { return ne(static_cast<double>(scalar)); }
template <class T>
requires(std::is_arithmetic_v<std::remove_cv_t<T>> && !std::is_same_v<std::remove_cv_t<T>, bool>)
Series operator<(T scalar) const { return lt(static_cast<double>(scalar)); }
template <class T>
requires(std::is_arithmetic_v<std::remove_cv_t<T>> && !std::is_same_v<std::remove_cv_t<T>, bool>)
Series operator<=(T scalar) const { return le(static_cast<double>(scalar)); }
template <class T>
requires(std::is_arithmetic_v<std::remove_cv_t<T>> && !std::is_same_v<std::remove_cv_t<T>, bool>)
Series operator>(T scalar) const { return gt(static_cast<double>(scalar)); }
template <class T>
requires(std::is_arithmetic_v<std::remove_cv_t<T>> && !std::is_same_v<std::remove_cv_t<T>, bool>)
Series operator>=(T scalar) const { return ge(static_cast<double>(scalar)); }
