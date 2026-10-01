#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "pluss/export.h"

#include "CodeH/core/Timeline.h"
#include "CodeH/compat/InvokeFwd.h"

namespace pluss::codeh {

namespace _compat { namespace detail { struct Access; } }

// A 1D, (optionally) monotonic Timeline axis with index-oriented utilities.
// This is the key building block for pandas-like resampling/alignment.
// Construction is intentionally routed through Timeline::index(...), not
// public constructors -- this class is CodeH's private implementation
// detail; the public name for what index() returns is
// Timeline::TimelineIndexAccessor.
class CODEH_API TimeIndex {
public:
  TimeIndex() = delete;

// ---------------------------------------------------------------------------
// Full op surface (pandas/MATLAB-inspired)
// ---------------------------------------------------------------------------
#if defined(CODEH_ENABLE_TIMEINDEX_SURFACE) && !defined(CODEH_NO_TIMEINDEX_SURFACE)
  #include "CodeH/core/TimeIndexSurface.h"
#endif

  const Timeline& timeline() const noexcept { return t_; }
  std::int64_t size() const { return t_.numel(); }
  std::vector<std::int64_t> host() const { return t_.ticks(); }

  // searchsorted (like numpy/pandas). Returns insertion index in [0, size()].
  std::int64_t searchsorted(const Timeline& key, bool right = false) const { return ::pluss::codeh::_compat::invoke<std::int64_t>("searchsorted", *this, {key, right}); }

  // Slice by time bounds (inclusive). Returns indices [lo, hi) as a 1D Series<int64>.
  pluss::codem::Series slice(const Timeline& start, const Timeline& end) const { return ::pluss::codeh::_compat::invoke<pluss::codem::Series>("slice", *this, {start, end}); }

  // Slice by ISO-like strings (inclusive), using CodeH's parser.
  pluss::codem::Series slice_time(std::string_view start_iso, std::string_view end_iso) const { return ::pluss::codeh::_compat::invoke<pluss::codem::Series>("slice_time", *this, {start_iso, end_iso}); }

  // asof: last index <= key. Returns -1 if all values > key.
  std::int64_t asof(const Timeline& key) const { return ::pluss::codeh::_compat::invoke<std::int64_t>("asof", *this, {key}); }

  // ---- Resample / rolling primitives ----
  struct ResampleBins {
    Timeline labels;              // 1D bin labels (left edges in tz-aware local time)
    pluss::codem::Series bin_id;  // 1D int64 of length size(): bin index per observation
  };

  // Canonically defined on Timeline now (reachable publicly as
  // Timeline::ResampleOptions/ResampleClosed/ResampleLabel/ResampleOrigin);
  // aliased here for source compatibility while callers migrate off TimeIndex.
  using ResampleClosed = Timeline::ResampleClosed;
  using ResampleLabel = Timeline::ResampleLabel;
  using ResampleOrigin = Timeline::ResampleOrigin;
  using ResampleOptions = Timeline::ResampleOptions;

  ResampleBins resample_bins(const ResampleOptions& options) const {
    return ::pluss::codeh::_compat::invoke<ResampleBins>(
        "resample_bins_opts",
        *this,
        {options.freq, options.step, options.closed, options.label, options.origin, options.offset_nanos});
  }
  ResampleBins resample_bins(std::string_view freq, std::int64_t step = 1) const {
    return resample_bins(ResampleOptions{.freq = std::string(freq), .step = step});
  }

  // Count observations per bin (like resample().size()).
  pluss::codem::Series resample_count(const ResampleOptions& options) const {
    return ::pluss::codeh::_compat::invoke<pluss::codem::Series>(
        "resample_count_opts",
        *this,
        {options.freq, options.step, options.closed, options.label, options.origin, options.offset_nanos});
  }
  pluss::codem::Series resample_count(std::string_view freq, std::int64_t step = 1) const {
    return resample_count(ResampleOptions{.freq = std::string(freq), .step = step});
  }

  // Numeric aggregations. values must be 1D and same length as index.
  pluss::codem::Series resample_sum(const pluss::codem::Series& values, const ResampleOptions& options) const {
    return ::pluss::codeh::_compat::invoke<pluss::codem::Series>(
        "resample_sum_opts",
        *this,
        {values, options.freq, options.step, options.closed, options.label, options.origin, options.offset_nanos});
  }
  pluss::codem::Series resample_sum(const pluss::codem::Series& values, std::string_view freq, std::int64_t step = 1) const { return resample_sum(values, ResampleOptions{.freq = std::string(freq), .step = step}); }
  pluss::codem::Series resample_mean(const pluss::codem::Series& values, const ResampleOptions& options) const {
    return ::pluss::codeh::_compat::invoke<pluss::codem::Series>(
        "resample_mean_opts",
        *this,
        {values, options.freq, options.step, options.closed, options.label, options.origin, options.offset_nanos});
  }
  pluss::codem::Series resample_mean(const pluss::codem::Series& values, std::string_view freq, std::int64_t step = 1) const { return resample_mean(values, ResampleOptions{.freq = std::string(freq), .step = step}); }
  pluss::codem::Series resample_min(const pluss::codem::Series& values, const ResampleOptions& options) const {
    return ::pluss::codeh::_compat::invoke<pluss::codem::Series>(
        "resample_min_opts",
        *this,
        {values, options.freq, options.step, options.closed, options.label, options.origin, options.offset_nanos});
  }
  pluss::codem::Series resample_min(const pluss::codem::Series& values, std::string_view freq, std::int64_t step = 1) const { return resample_min(values, ResampleOptions{.freq = std::string(freq), .step = step}); }
  pluss::codem::Series resample_max(const pluss::codem::Series& values, const ResampleOptions& options) const {
    return ::pluss::codeh::_compat::invoke<pluss::codem::Series>(
        "resample_max_opts",
        *this,
        {values, options.freq, options.step, options.closed, options.label, options.origin, options.offset_nanos});
  }
  pluss::codem::Series resample_max(const pluss::codem::Series& values, std::string_view freq, std::int64_t step = 1) const { return resample_max(values, ResampleOptions{.freq = std::string(freq), .step = step}); }
  pluss::codem::Series resample_first(const pluss::codem::Series& values, const ResampleOptions& options) const {
    return ::pluss::codeh::_compat::invoke<pluss::codem::Series>(
        "resample_first_opts",
        *this,
        {values, options.freq, options.step, options.closed, options.label, options.origin, options.offset_nanos});
  }
  pluss::codem::Series resample_first(const pluss::codem::Series& values, std::string_view freq, std::int64_t step = 1) const { return resample_first(values, ResampleOptions{.freq = std::string(freq), .step = step}); }
  pluss::codem::Series resample_last(const pluss::codem::Series& values, const ResampleOptions& options) const {
    return ::pluss::codeh::_compat::invoke<pluss::codem::Series>(
        "resample_last_opts",
        *this,
        {values, options.freq, options.step, options.closed, options.label, options.origin, options.offset_nanos});
  }
  pluss::codem::Series resample_last(const pluss::codem::Series& values, std::string_view freq, std::int64_t step = 1) const { return resample_last(values, ResampleOptions{.freq = std::string(freq), .step = step}); }

  pluss::codem::Series resample_agg(const pluss::codem::Series& values, const ResampleOptions& options, std::string_view op) const {
    return ::pluss::codeh::_compat::invoke<pluss::codem::Series>(
        "resample_agg_opts",
        *this,
        {values, options.freq, options.step, options.closed, options.label, options.origin, options.offset_nanos, std::string(op)});
  }

  // OHLC: returns 2D [nbins, 4] in order [open, high, low, close].
  pluss::codem::Series resample_ohlc(const pluss::codem::Series& values, const ResampleOptions& options) const {
    return ::pluss::codeh::_compat::invoke<pluss::codem::Series>(
        "resample_ohlc_opts",
        *this,
        {values, options.freq, options.step, options.closed, options.label, options.origin, options.offset_nanos});
  }
  pluss::codem::Series resample_ohlc(const pluss::codem::Series& values, std::string_view freq, std::int64_t step = 1) const { return resample_ohlc(values, ResampleOptions{.freq = std::string(freq), .step = step}); }

  // Rolling count within a trailing window.
  pluss::codem::Series rolling_count(const Duration& window) const { return ::pluss::codeh::_compat::invoke<pluss::codem::Series>("rolling_count", *this, {window}); }

  // Rolling sum within trailing window.
  pluss::codem::Series rolling_sum(const pluss::codem::Series& values, const Duration& window) const { return ::pluss::codeh::_compat::invoke<pluss::codem::Series>("rolling_sum", *this, {values, window}); }

  // ---- Alignment/join skeletons ----
  using AsofDirection = Timeline::AsofDirection;

  // For each left timestamp, find matching right timestamp index (or -1).
  pluss::codem::Series join_asof(const TimeIndex& right,
                                AsofDirection direction = AsofDirection::Backward,
                                const Duration* tolerance = nullptr,
                                bool allow_exact_matches = true) const;

  struct AlignUnion {
    Timeline index;                 // union index timeline
    pluss::codem::Series left_indexer;  // map union -> left (or -1)
    pluss::codem::Series right_indexer; // map union -> right (or -1)
  };

  AlignUnion align_union(const TimeIndex& other) const { return ::pluss::codeh::_compat::invoke<AlignUnion>("align_union", *this, {other}); }

private:
  enum class OrderState : std::uint8_t {
    Unchecked = 0,
    Nondecreasing = 1,
  };

  explicit TimeIndex(Timeline t, OrderState order_state)
      : t_(std::move(t)), order_state_(order_state) {}
  Timeline t_;
  mutable OrderState order_state_{OrderState::Unchecked};

  friend struct ::pluss::codeh::_compat::detail::Access;
};

} // namespace pluss::codeh

// TimeIndex inline methods route through _compat::invoke<...>.
#include "CodeH/compat/Invoke.h"
