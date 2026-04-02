#pragma once

#include "pluss/export.h"

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <limits>
#include <vector>

#include "CodeH/core/Errors.h"
#include "CodeH/compat/InvokeFwd.h"
#include "CodeM/core/Series.h"
#include "CodeM/core/SeriesIterator.h"
#include "MSharp/RangeView.h"

#include "CodeN/core/Note.h"

namespace pluss::codeh {
class Duration;
class Timeline;
} // namespace pluss::codeh
namespace pluss::codeh::_compat::detail {
struct Access;
}

namespace pluss::codeh::detail {
Duration impl_add_duration(const Duration& a, const Duration& b);
Duration impl_sub_duration(const Duration& a, const Duration& b);
Duration impl_neg_duration(const Duration& a);
Duration impl_range(const Duration& start, const Duration& end, const Duration& step);
Duration impl_diff(const Timeline& a, const Timeline& b);
Timeline impl_add_duration(const Timeline& a, const Duration& b);
Timeline impl_sub_duration(const Timeline& a, const Duration& b);

// Extended Timedelta surface
pluss::codem::Series impl_cmp_duration(const Duration& a, const Duration& b, std::string_view op);
pluss::codem::Series impl_isnat_duration(const Duration& a);
pluss::codem::Series impl_isvalid_duration(const Duration& a);
Duration impl_abs_duration(const Duration& a);
Duration impl_sign_duration(const Duration& a);
Duration impl_clip_duration(const Duration& a, const Duration& lo, const Duration& hi);
Duration impl_fill_nat_duration(const Duration& a, const Duration& value);
Duration impl_where_duration(const Duration& a, const pluss::codem::Series& mask, const Duration& other);
Duration impl_mul_i64(const Duration& a, std::int64_t k);
Duration impl_div_i64(const Duration& a, std::int64_t k);
Duration impl_floordiv_i64(const Duration& a, std::int64_t k);
Duration impl_mod_i64(const Duration& a, std::int64_t k);
pluss::codem::Series impl_total(const Duration& a, std::string_view unit);
pluss::codem::Series impl_component(const Duration& a, std::string_view component);
Duration impl_round(const Duration& a, std::string_view mode, std::string_view unit);
Duration impl_min_duration(const Duration& a);
Duration impl_max_duration(const Duration& a);
Duration impl_sort_duration(const Duration& a);
Duration impl_unique_duration(const Duration& a);
pluss::codem::Series impl_argsort_duration(const Duration& a);
std::vector<std::int64_t> impl_to_nanos_flat(const Duration& a);
pluss::coden::Note impl_duration_to_note(const Duration& a, std::string_view style);
} // namespace pluss::codeh::detail

namespace pluss::codeh {

class TimeIndex;
class Timeline;
class Duration;

namespace ops_internal {
  Duration duration_from_nanos(std::span<const std::int64_t> nanos_flat,
                               std::span<const std::int64_t> sizes);
} // namespace ops_internal

// Duration: N-D (0D..3D) array of nanosecond spans.
// Storage: Series<int64> nanos_ (UTC-agnostic).
class CODEH_API Duration {
public:
  static constexpr std::int64_t kNaT = (std::numeric_limits<std::int64_t>::min)();

  Duration() = delete; // construction via factories only

// ---------------------------------------------------------------------------
// Full op surface (pandas/MATLAB-inspired)
// ---------------------------------------------------------------------------
#if defined(CODEH_ENABLE_DURATION_SURFACE) && !defined(CODEH_NO_DURATION_SURFACE)
  #include "CodeH/core/DurationSurface.h"
#endif

  bool defined() const noexcept { return nanos_.defined(); }
  pluss::msharp::RangeView::Int sizes() const { return nanos_.sizes(); }
  std::int64_t dim() const { return nanos_.dim(); }
  std::int64_t numel() const { return nanos_.numel(); }

  // Elementwise arithmetic (broadcasting via SeriesIterator).
  Duration operator+(const Duration& other) const;
  Duration operator-(const Duration& other) const;
  Duration operator-() const;

  // Scalar helpers
  static Duration from_scalar_nanos(std::int64_t nanos);

  // Range factory (like pandas timedelta_range)
  // Inclusive start; stops before exceeding end.
  static Duration range(const Duration& start, const Duration& end, const Duration& step);

private:

  friend class Timeline;
  friend class TimeIndex;
  friend struct ::pluss::codeh::_compat::detail::Access;
  friend Duration detail::impl_diff(const Timeline&, const Timeline&);
  friend Timeline detail::impl_add_duration(const Timeline&, const Duration&);
  friend Timeline detail::impl_sub_duration(const Timeline&, const Duration&);
  friend Duration pluss::codeh::ops_internal::duration_from_nanos(std::span<const std::int64_t>,
                                                                 std::span<const std::int64_t>);
  friend Duration detail::impl_add_duration(const Duration&, const Duration&);
  friend Duration detail::impl_sub_duration(const Duration&, const Duration&);
  friend Duration detail::impl_neg_duration(const Duration&);
  friend Duration detail::impl_range(const Duration&, const Duration&, const Duration&);
  friend pluss::codem::Series detail::impl_cmp_duration(const Duration&, const Duration&, std::string_view);
  friend pluss::codem::Series detail::impl_isnat_duration(const Duration&);
  friend pluss::codem::Series detail::impl_isvalid_duration(const Duration&);
  friend Duration detail::impl_abs_duration(const Duration&);
  friend Duration detail::impl_sign_duration(const Duration&);
  friend Duration detail::impl_clip_duration(const Duration&, const Duration&, const Duration&);
  friend Duration detail::impl_fill_nat_duration(const Duration&, const Duration&);
  friend Duration detail::impl_where_duration(const Duration&, const pluss::codem::Series&, const Duration&);
  friend Duration detail::impl_mul_i64(const Duration&, std::int64_t);
  friend Duration detail::impl_div_i64(const Duration&, std::int64_t);
  friend Duration detail::impl_floordiv_i64(const Duration&, std::int64_t);
  friend Duration detail::impl_mod_i64(const Duration&, std::int64_t);
  friend pluss::codem::Series detail::impl_total(const Duration&, std::string_view);
  friend pluss::codem::Series detail::impl_component(const Duration&, std::string_view);
  friend Duration detail::impl_round(const Duration&, std::string_view, std::string_view);
  friend Duration detail::impl_min_duration(const Duration&);
  friend Duration detail::impl_max_duration(const Duration&);
  friend Duration detail::impl_sort_duration(const Duration&);
  friend Duration detail::impl_unique_duration(const Duration&);
  friend pluss::codem::Series detail::impl_argsort_duration(const Duration&);
  friend std::vector<std::int64_t> detail::impl_to_nanos_flat(const Duration&);
  friend pluss::coden::Note detail::impl_duration_to_note(const Duration&, std::string_view);
  explicit Duration(pluss::codem::Series nanos) : nanos_(std::move(nanos)) {}

  pluss::codem::Series nanos_;
};

} // namespace pluss::codeh
