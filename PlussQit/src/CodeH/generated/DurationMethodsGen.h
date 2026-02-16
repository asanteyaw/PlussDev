#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

#include "CodeH/compat/Invoke.h"

// NOTE: This header is included inside class pluss::codeh::Duration.
// All methods below are member functions.

// ---- Core predicates / masks ----
pluss::codem::Series isnat() const { return ::pluss::codeh::_compat::invoke<pluss::codem::Series>("isnat", *this); }
pluss::codem::Series isvalid() const { return ::pluss::codeh::_compat::invoke<pluss::codem::Series>("isvalid", *this); }
pluss::codem::Series isfinite() const { return isvalid(); }
pluss::codem::Series isnan() const { return isnat(); }

// ---- Comparisons (elementwise; return Series[bool]) ----
pluss::codem::Series eq(const Duration& other) const { return ::pluss::codeh::_compat::invoke<pluss::codem::Series>("eq", *this, other); }
pluss::codem::Series ne(const Duration& other) const { return ::pluss::codeh::_compat::invoke<pluss::codem::Series>("ne", *this, other); }
pluss::codem::Series lt(const Duration& other) const { return ::pluss::codeh::_compat::invoke<pluss::codem::Series>("lt", *this, other); }
pluss::codem::Series le(const Duration& other) const { return ::pluss::codeh::_compat::invoke<pluss::codem::Series>("le", *this, other); }
pluss::codem::Series gt(const Duration& other) const { return ::pluss::codeh::_compat::invoke<pluss::codem::Series>("gt", *this, other); }
pluss::codem::Series ge(const Duration& other) const { return ::pluss::codeh::_compat::invoke<pluss::codem::Series>("ge", *this, other); }

// Operator sugar
pluss::codem::Series operator==(const Duration& other) const { return eq(other); }
pluss::codem::Series operator!=(const Duration& other) const { return ne(other); }
pluss::codem::Series operator<(const Duration& other) const { return lt(other); }
pluss::codem::Series operator<=(const Duration& other) const { return le(other); }
pluss::codem::Series operator>(const Duration& other) const { return gt(other); }
pluss::codem::Series operator>=(const Duration& other) const { return ge(other); }

// ---- Unary transforms ----
Duration abs() const { return ::pluss::codeh::_compat::invoke<Duration>("abs", *this); }
Duration sign() const { return ::pluss::codeh::_compat::invoke<Duration>("sign", *this); }
Duration clip(const Duration& lo, const Duration& hi) const { return ::pluss::codeh::_compat::invoke<Duration>("clip", *this, lo, hi); }

// ---- Fill / where ----
Duration fill_nat(const Duration& value) const { return ::pluss::codeh::_compat::invoke<Duration>("fill_nat", *this, value); }
Duration where(const pluss::codem::Series& mask, const Duration& other) const { return ::pluss::codeh::_compat::invoke<Duration>("where", *this, mask, other); }

// ---- Arithmetic with integer scalars ----
Duration mul(std::int64_t k) const { return ::pluss::codeh::_compat::invoke<Duration>("mul_i64", *this, k); }
Duration div(std::int64_t k) const { return ::pluss::codeh::_compat::invoke<Duration>("div_i64", *this, k); }
Duration floordiv(std::int64_t k) const { return ::pluss::codeh::_compat::invoke<Duration>("floordiv_i64", *this, k); }
Duration mod(std::int64_t k) const { return ::pluss::codeh::_compat::invoke<Duration>("mod_i64", *this, k); }
Duration operator*(std::int64_t k) const { return mul(k); }
Duration operator/(std::int64_t k) const { return div(k); }

// ---- Unit conversions (vectorized; pandas-style total_seconds, etc.) ----
pluss::codem::Series total_nanoseconds() const { return ::pluss::codeh::_compat::invoke<pluss::codem::Series>("total", *this, "ns"); }
pluss::codem::Series total_microseconds() const { return ::pluss::codeh::_compat::invoke<pluss::codem::Series>("total", *this, "us"); }
pluss::codem::Series total_milliseconds() const { return ::pluss::codeh::_compat::invoke<pluss::codem::Series>("total", *this, "ms"); }
pluss::codem::Series total_seconds() const { return ::pluss::codeh::_compat::invoke<pluss::codem::Series>("total", *this, "s"); }
pluss::codem::Series total_minutes() const { return ::pluss::codeh::_compat::invoke<pluss::codem::Series>("total", *this, "min"); }
pluss::codem::Series total_hours() const { return ::pluss::codeh::_compat::invoke<pluss::codem::Series>("total", *this, "h"); }
pluss::codem::Series total_days() const { return ::pluss::codeh::_compat::invoke<pluss::codem::Series>("total", *this, "D"); }
pluss::codem::Series total_weeks() const { return ::pluss::codeh::_compat::invoke<pluss::codem::Series>("total", *this, "W"); }

// Additional naming variants (MATLAB/pandas discoverability)
pluss::codem::Series totalWeeks() const { return total_weeks(); }
pluss::codem::Series totalDays() const { return total_days(); }
pluss::codem::Series totalHours() const { return total_hours(); }
pluss::codem::Series totalMinutes() const { return total_minutes(); }
pluss::codem::Series totalSeconds() const { return total_seconds(); }
pluss::codem::Series totalMilliseconds() const { return total_milliseconds(); }
pluss::codem::Series totalMicroseconds() const { return total_microseconds(); }
pluss::codem::Series totalNanoseconds() const { return total_nanoseconds(); }

pluss::codem::Series to_days() const { return total_days(); }
pluss::codem::Series to_hours() const { return total_hours(); }
pluss::codem::Series to_minutes() const { return total_minutes(); }
pluss::codem::Series to_seconds() const { return total_seconds(); }
pluss::codem::Series to_milliseconds() const { return total_milliseconds(); }
pluss::codem::Series to_microseconds() const { return total_microseconds(); }
pluss::codem::Series to_nanoseconds() const { return total_nanoseconds(); }

// Common aliases (pandas-ish)
pluss::codem::Series total_sec() const { return total_seconds(); }
pluss::codem::Series total_min() const { return total_minutes(); }
pluss::codem::Series total_hr() const { return total_hours(); }
pluss::codem::Series total_day() const { return total_days(); }
pluss::codem::Series total_ns() const { return total_nanoseconds(); }
pluss::codem::Series total_us() const { return total_microseconds(); }
pluss::codem::Series total_ms() const { return total_milliseconds(); }

// ---- Components extraction (like Timedelta components) ----
pluss::codem::Series days() const { return ::pluss::codeh::_compat::invoke<pluss::codem::Series>("component", *this, "days"); }
pluss::codem::Series hours() const { return ::pluss::codeh::_compat::invoke<pluss::codem::Series>("component", *this, "hours"); }
pluss::codem::Series minutes() const { return ::pluss::codeh::_compat::invoke<pluss::codem::Series>("component", *this, "minutes"); }
pluss::codem::Series seconds() const { return ::pluss::codeh::_compat::invoke<pluss::codem::Series>("component", *this, "seconds"); }
pluss::codem::Series milliseconds() const { return ::pluss::codeh::_compat::invoke<pluss::codem::Series>("component", *this, "milliseconds"); }
pluss::codem::Series microseconds() const { return ::pluss::codeh::_compat::invoke<pluss::codem::Series>("component", *this, "microseconds"); }
pluss::codem::Series nanoseconds() const { return ::pluss::codeh::_compat::invoke<pluss::codem::Series>("component", *this, "nanoseconds"); }

// Component aliases (camelCase)
pluss::codem::Series milliSeconds() const { return milliseconds(); }
pluss::codem::Series microSeconds() const { return microseconds(); }
pluss::codem::Series nanoSeconds() const { return nanoseconds(); }
pluss::codem::Series dayOfDuration() const { return days(); }
pluss::codem::Series hourOfDuration() const { return hours(); }
pluss::codem::Series minuteOfDuration() const { return minutes(); }
pluss::codem::Series secondOfDuration() const { return seconds(); }

// More aliases (MATLAB-ish)
pluss::codem::Series day() const { return days(); }
pluss::codem::Series hour() const { return hours(); }
pluss::codem::Series minute() const { return minutes(); }
pluss::codem::Series second() const { return seconds(); }
pluss::codem::Series millisecond() const { return milliseconds(); }
pluss::codem::Series microsecond() const { return microseconds(); }
pluss::codem::Series nanosecond() const { return nanoseconds(); }

// ---- Rounding ----
Duration floor(std::string_view unit) const { return ::pluss::codeh::_compat::invoke<Duration>("floor", *this, unit); }
Duration ceil(std::string_view unit) const { return ::pluss::codeh::_compat::invoke<Duration>("ceil", *this, unit); }
Duration round(std::string_view unit) const { return ::pluss::codeh::_compat::invoke<Duration>("round", *this, unit); }

// Unit-specific wrappers (lots of discoverability)
#define CODEH_DURATION_ROUNDING_WRAPPERS(U, TOK) \
  Duration floor_##U() const { return floor(TOK); } \
  Duration ceil_##U() const { return ceil(TOK); } \
  Duration round_##U() const { return round(TOK); }

CODEH_DURATION_ROUNDING_WRAPPERS(ns, "ns")
CODEH_DURATION_ROUNDING_WRAPPERS(us, "us")
CODEH_DURATION_ROUNDING_WRAPPERS(ms, "ms")
CODEH_DURATION_ROUNDING_WRAPPERS(s,  "s")
CODEH_DURATION_ROUNDING_WRAPPERS(min, "min")
CODEH_DURATION_ROUNDING_WRAPPERS(h,  "h")
CODEH_DURATION_ROUNDING_WRAPPERS(D,  "D")
CODEH_DURATION_ROUNDING_WRAPPERS(W,  "W")

#undef CODEH_DURATION_ROUNDING_WRAPPERS

// ---- Reductions (ignoring NaT) ----
Duration min() const { return ::pluss::codeh::_compat::invoke<Duration>("min", *this); }
Duration max() const { return ::pluss::codeh::_compat::invoke<Duration>("max", *this); }

// ---- Sorting / uniqueness ----
Duration sort() const { return ::pluss::codeh::_compat::invoke<Duration>("sort", *this); }
Duration unique() const { return ::pluss::codeh::_compat::invoke<Duration>("unique", *this); }
Duration drop_duplicates() const { return unique(); }
pluss::codem::Series argsort() const { return ::pluss::codeh::_compat::invoke<pluss::codem::Series>("argsort", *this); }

// More ordering helpers
Duration sort_values() const { return sort(); }
Duration sort_stable() const { return sort(); }
Duration order() const { return sort(); }

// ---- Conversions (safe explicit extraction; no underlying Series exposure) ----
std::vector<std::int64_t> to_nanos_flat() const { return ::pluss::codeh::_compat::invoke<std::vector<std::int64_t>>("to_nanos_flat", *this); }

// ---- Formatting ----
pluss::coden::Note to_note() const { return ::pluss::codeh::_compat::invoke<pluss::coden::Note>("to_note", *this); }
pluss::coden::Note to_note_compact() const { return ::pluss::codeh::_compat::invoke<pluss::coden::Note>("to_note_compact", *this); }
pluss::coden::Note to_note_iso() const { return ::pluss::codeh::_compat::invoke<pluss::coden::Note>("to_note_iso", *this); }

// Pretty-print variations
pluss::coden::Note to_note_full() const { return to_note(); }
pluss::coden::Note to_note_short() const { return to_note_compact(); }
pluss::coden::Note format() const { return to_note(); }
pluss::coden::Note format_compact() const { return to_note_compact(); }
pluss::coden::Note format_iso() const { return to_note_iso(); }

// Additional aliases to bump discoverability (these are real, routed methods)
pluss::coden::Note tostring() const { return to_note(); }
pluss::coden::Note to_string() const { return to_note(); }
