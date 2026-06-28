#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <utility>

#include "pluss/export.h"

#include "CodeH/core/Timeline.h"
#include "CodeH/compat/InvokeFwd.h"

namespace pluss::codeh {

namespace _compat { namespace detail { struct Access; } }

// A 1D, (optionally) monotonic Timeline axis with index-oriented utilities.
// This is the key building block for pandas-like resampling/alignment.
// Construction is intentionally routed through pluss::make_timeindex(...)
// and Timeline::to_timeindex(...), not public constructors.
class CODEH_API TimeIndex {
public:
  TimeIndex() = delete;

  static TimeIndex from_buffer_epoch_nanos(std::int64_t* data,
                                           std::int64_t size,
                                           std::int64_t stride = 1,
                                           pluss::codem::BlobDeleter deleter = {},
                                           bool require_sorted = true);

// ---------------------------------------------------------------------------
// Full op surface (pandas/MATLAB-inspired)
// ---------------------------------------------------------------------------
#if defined(CODEH_ENABLE_TIMEINDEX_SURFACE) && !defined(CODEH_NO_TIMEINDEX_SURFACE)
  #include "CodeH/core/TimeIndexSurface.h"
#endif

  const Timeline& timeline() const noexcept { return t_; }
  std::int64_t size() const { return t_.numel(); }

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

  ResampleBins resample_bins(std::string_view freq, std::int64_t step = 1) const { return ::pluss::codeh::_compat::invoke<ResampleBins>("resample_bins", *this, {freq, step}); }

  // Count observations per bin (like resample().size()).
  pluss::codem::Series resample_count(std::string_view freq, std::int64_t step = 1) const { return ::pluss::codeh::_compat::invoke<pluss::codem::Series>("resample_count", *this, {freq, step}); }

  // Numeric aggregations. values must be 1D and same length as index.
  pluss::codem::Series resample_sum(const pluss::codem::Series& values, std::string_view freq, std::int64_t step = 1) const { return ::pluss::codeh::_compat::invoke<pluss::codem::Series>("resample_sum", *this, {values, freq, step}); }
  pluss::codem::Series resample_mean(const pluss::codem::Series& values, std::string_view freq, std::int64_t step = 1) const { return ::pluss::codeh::_compat::invoke<pluss::codem::Series>("resample_mean", *this, {values, freq, step}); }
  pluss::codem::Series resample_min(const pluss::codem::Series& values, std::string_view freq, std::int64_t step = 1) const { return ::pluss::codeh::_compat::invoke<pluss::codem::Series>("resample_min", *this, {values, freq, step}); }
  pluss::codem::Series resample_max(const pluss::codem::Series& values, std::string_view freq, std::int64_t step = 1) const { return ::pluss::codeh::_compat::invoke<pluss::codem::Series>("resample_max", *this, {values, freq, step}); }
  pluss::codem::Series resample_first(const pluss::codem::Series& values, std::string_view freq, std::int64_t step = 1) const { return ::pluss::codeh::_compat::invoke<pluss::codem::Series>("resample_first", *this, {values, freq, step}); }
  pluss::codem::Series resample_last(const pluss::codem::Series& values, std::string_view freq, std::int64_t step = 1) const { return ::pluss::codeh::_compat::invoke<pluss::codem::Series>("resample_last", *this, {values, freq, step}); }

  // OHLC: returns 2D [nbins, 4] in order [open, high, low, close].
  pluss::codem::Series resample_ohlc(const pluss::codem::Series& values, std::string_view freq, std::int64_t step = 1) const { return ::pluss::codeh::_compat::invoke<pluss::codem::Series>("resample_ohlc", *this, {values, freq, step}); }

  // Rolling count within a trailing window.
  pluss::codem::Series rolling_count(const Duration& window) const { return ::pluss::codeh::_compat::invoke<pluss::codem::Series>("rolling_count", *this, {window}); }

  // Rolling sum within trailing window.
  pluss::codem::Series rolling_sum(const pluss::codem::Series& values, const Duration& window) const { return ::pluss::codeh::_compat::invoke<pluss::codem::Series>("rolling_sum", *this, {values, window}); }

  // ---- Alignment/join skeletons ----
  enum class AsofDirection { Backward, Forward, Nearest };

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
  explicit TimeIndex(Timeline t) : t_(std::move(t)) {}
  Timeline t_;

  friend struct ::pluss::codeh::_compat::detail::Access;
};

inline TimeIndex TimeIndex::from_buffer_epoch_nanos(std::int64_t* data,
                                                    std::int64_t size,
                                                    std::int64_t stride,
                                                    pluss::codem::BlobDeleter deleter,
                                                    bool require_sorted) {
  CODEM_CHECK(size >= 0, "TimeIndex::from_buffer_epoch_nanos: size must be non-negative");
  CODEM_CHECK(stride > 0, "TimeIndex::from_buffer_epoch_nanos: stride must be positive");
  const std::array<std::int64_t, 1> sizes{size};
  const std::array<std::int64_t, 1> strides{stride};
  auto t = Timeline::from_buffer_epoch_nanos(
      data,
      std::span<const std::int64_t>(sizes.data(), sizes.size()),
      std::span<const std::int64_t>(strides.data(), strides.size()),
      std::move(deleter));
  return ::pluss::codeh::_compat::invoke<TimeIndex>("make_timeindex", t, {require_sorted});
}

} // namespace pluss::codeh

// TimeIndex inline methods route through _compat::invoke<...>.
#include "CodeH/compat/Invoke.h"
