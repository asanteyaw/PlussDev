#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <iterator>
#include <ostream>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "pluss/codeh/codeh.h"
#include "pluss/codeh/internal/int64_flat_reader.h"
#include "pluss/codeh/internal/timeline_text_reader.h"
#include "pluss/codeh/timeline_factory.h"
#include "pluss/detail/nested_timeline_literal.h"

namespace pluss::timeindex {

// TimeIndex construction is fundamentally Timeline construction plus a
// wrap+sort-check -- every overload below except the Timeline one just
// builds a Timeline first, the same way pluss::timeline::make/from_epoch
// already do, then hands it to the "real" make(const Timeline&, bool).
// So TimeIndex doesn't need its own data-ingestion logic or its own reader
// type: it reuses Timeline's two (TimelineTextReader for string data,
// Int64FlatReader for raw epoch nanoseconds), collapsing what used to be
// 10 overloads (one per input shape x source type) down to 3.
//
// Users MUST NOT construct TimeIndex directly.
inline pluss::TimeIndex make(const pluss::Timeline& t, bool require_sorted = true) {
  return ::pluss::codeh::_compat::invoke<pluss::TimeIndex>("make_timeindex", t, {require_sorted});
}

// TimeIndex is always 1D, even for scalar input -- unlike Timeline itself,
// which is happy to be 0D. A scalar reader (empty shape) is promoted to a
// 1-element 1D shape before building the underlying Timeline, matching what
// the original scalar overloads did explicitly (`size{1}`, not `{}`).
// Skipping this reshape would build a 0D Timeline that TimeIndex's own
// construction path rejects outright ("TimeIndex requires a 1D Timeline").
inline pluss::TimeIndex make(pluss::codeh::detail::TimelineTextReader data,
                             bool require_sorted = true) {
  if (data.shape().empty()) {
    const std::array<std::int64_t, 1> one{1};
    return make(::pluss::codeh::ops_internal::timeline_from_utf8(data.flat().span(), one),
                require_sorted);
  }
  return make(pluss::timeline::make(std::move(data)), require_sorted);
}

inline pluss::TimeIndex make(pluss::codeh::detail::Int64FlatReader data,
                             bool require_sorted = true) {
  if (data.sizes().empty()) {
    const std::array<std::int64_t, 1> one{1};
    return make(::pluss::codeh::ops_internal::timeline_from_epoch_nanos(data.flat(), one),
                require_sorted);
  }
  return make(pluss::timeline::from_epoch(std::move(data)), require_sorted);
}

// pluss::timeindex::date_range(start, end, freq, step, require_sorted) ->
// TimeIndex built directly from pluss::timeline::date_range. A date_range
// result is inherently sorted (every step moves strictly forward in time),
// so this spares the caller the intermediate
// pluss::timeindex::make(pluss::timeline::date_range(...)) call -- the
// TimeIndex equivalent of pandas' pd.date_range(...) already returning a
// DatetimeIndex directly, rather than a plain array.
inline pluss::TimeIndex date_range(const pluss::Timeline& start,
                                   const pluss::Timeline& end,
                                   std::string_view freq,
                                   std::int64_t step = 1,
                                   bool require_sorted = true) {
  return make(pluss::timeline::date_range(start, end, freq, step), require_sorted);
}

// pluss::timeindex::print(idx) -- human-readable rendering, mirroring
// pluss::timeline::print/pluss::duration::print. `std::cout << idx` also
// works directly via pluss::codeh's TimeIndex operator<<.
inline std::string print(const pluss::TimeIndex& idx) {
  return pluss::codeh::print_timeindex(idx);
}
inline std::ostream& print(std::ostream& os, const pluss::TimeIndex& idx) {
  return pluss::codeh::print_timeindex(os, idx);
}

} // namespace pluss::timeindex

namespace pluss {

// Backward-compatible spelling -- forwards to pluss::timeindex::make(...).
// Each overload mirrors pluss::timeindex::make's parameter list exactly
// (not a generic forwarding template): a template can't deduce a type from
// a bare {...} braced-list argument, so e.g. pluss::make_timeindex({...})
// needs the parameter to already be the concrete std::initializer_list<T>
// type for that call to resolve at all.
inline pluss::TimeIndex make_timeindex(const pluss::Timeline& t, bool require_sorted = true) {
  return pluss::timeindex::make(t, require_sorted);
}
inline pluss::TimeIndex make_timeindex(std::string_view scalar, bool require_sorted = true) {
  return pluss::timeindex::make(scalar, require_sorted);
}
inline pluss::TimeIndex make_timeindex(const char* scalar, bool require_sorted = true) {
  return pluss::timeindex::make(scalar, require_sorted);
}
inline pluss::TimeIndex make_timeindex(std::span<const std::string_view> s1d, bool require_sorted = true) {
  return pluss::timeindex::make(s1d, require_sorted);
}
inline pluss::TimeIndex make_timeindex(const std::vector<std::string_view>& s1d, bool require_sorted = true) {
  return pluss::timeindex::make(s1d, require_sorted);
}
inline pluss::TimeIndex make_timeindex(const std::vector<std::string>& s1d, bool require_sorted = true) {
  return pluss::timeindex::make(s1d, require_sorted);
}
inline pluss::TimeIndex make_timeindex(std::initializer_list<const char*> s1d,
                                       bool require_sorted = true) {
  return pluss::timeindex::make(s1d, require_sorted);
}
inline pluss::TimeIndex make_timeindex(
    std::initializer_list<pluss::detail::NestedTimelineLiteral> nested,
    bool require_sorted = true) {
  return pluss::timeindex::make(nested, require_sorted);
}
inline pluss::TimeIndex make_timeindex(std::span<const std::int64_t> epoch_nanos,
                                       bool require_sorted = true) {
  return pluss::timeindex::make(epoch_nanos, require_sorted);
}
inline pluss::TimeIndex make_timeindex(const std::vector<std::int64_t>& epoch_nanos,
                                       bool require_sorted = true) {
  return pluss::timeindex::make(epoch_nanos, require_sorted);
}

} // namespace pluss
