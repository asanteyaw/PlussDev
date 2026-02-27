#pragma once

#include "pluss/export.h"

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "CodeH/core/Duration.h"
#include "CodeH/core/TimelineBase.h"
#include "CodeH/compat/InvokeFwd.h"

#include "pluss/codem/codem.h"
#include "pluss/coden/coden.h" // for Note

namespace pluss::codeh {

namespace _compat { namespace detail { struct Access; } }

class Timeline;
class TimeIndex;
struct CalendarOffset;

namespace ops_internal {
  Timeline timeline_from_utf8(std::span<const std::string_view> utf8_flat,
                              std::span<const std::int64_t> sizes);
  Timeline timeline_from_epoch_nanos(std::span<const std::int64_t> ticks_flat,
                                     std::span<const std::int64_t> sizes);
} // namespace ops_internal

namespace detail {
// Core impls called by invoke dispatch (see CodeH/compat/Dispatch.cpp)
Timeline impl_add_duration(const Timeline& a, const Duration& d);
Timeline impl_sub_duration(const Timeline& a, const Duration& d);
Timeline impl_add_offset(const Timeline& a, const CalendarOffset& off);
Timeline impl_sub_offset(const Timeline& a, const CalendarOffset& off);
Duration impl_diff(const Timeline& a, const Timeline& b);
pluss::codem::Series impl_cmp(const Timeline& a, const Timeline& b, std::string_view op);
pluss::codem::Series impl_dt_field(const Timeline& a, std::string_view field);
pluss::codem::Series impl_dt_predicate(const Timeline& a, std::string_view pred_op);
Timeline impl_dt_round(const Timeline& a, std::string_view mode, std::string_view unit);
std::string impl_isoformat(const Timeline& a);
pluss::coden::Note impl_strftime(const Timeline& a, std::string_view fmt);
Timeline impl_date_range(const Timeline& start, const Timeline& end, std::string_view freq, std::int64_t step);
Timeline impl_tz_convert(const Timeline& a, std::string_view tz_name);
Timeline impl_tz_localize(const Timeline& a, std::string_view tz_name, int fold, std::string_view nonexistent);
Timeline impl_time_of_day(const Timeline& a);

pluss::codem::Series impl_isnat(const Timeline& a);
pluss::codem::Series impl_isvalid(const Timeline& a);
bool impl_isnat_scalar(const Timeline& a);
pluss::codem::Series impl_argsort(const Timeline& a);
Timeline impl_min(const Timeline& a);
Timeline impl_max(const Timeline& a);
Timeline impl_sort(const Timeline& a);
Timeline impl_unique(const Timeline& a);
Timeline impl_fill_nat(const Timeline& a, const Timeline& value);
Timeline impl_where(const Timeline& a, const pluss::codem::Series& mask, const Timeline& other);
pluss::codem::Series impl_searchsorted(const Timeline& a, const Timeline& values, bool left);
Timeline impl_add_nanos(const Timeline& a, std::int64_t nanos);
std::vector<std::int64_t> impl_to_epoch_nanos_flat(const Timeline& a);
pluss::coden::Note impl_weekday_name(const Timeline& a);
pluss::coden::Note impl_month_name(const Timeline& a);
} // namespace detail

// Timeline: N-D (0D..3D) array of instants.
// Canonical storage: epoch nanoseconds in UTC (int64).
class CODEH_API Timeline : public TimelineBase {
public:
  static constexpr std::int64_t kNaT = Duration::kNaT;

  Timeline() = delete; // construction via factories only

  using TimelineBase::weekday_name;
  using TimelineBase::month_name;

  static Timeline from_buffer_epoch_nanos(std::int64_t* data,
                                          std::span<const std::int64_t> sizes,
                                          std::span<const std::int64_t> strides,
                                          pluss::codem::BlobDeleter deleter = {},
                                          std::string_view tz_name = "UTC");

#if defined(CODEH_ENABLE_TIMELINE_SURFACE) && !defined(CODEH_NO_TIMELINE_SURFACE)
  #include "CodeH/core/TimelineSurface.h"
#endif

private:
  friend class TimeIndex;
  friend struct ::pluss::codeh::_compat::detail::Access; // internal bridge

  friend Timeline pluss::codeh::ops_internal::timeline_from_utf8(std::span<const std::string_view>,
                                                                 std::span<const std::int64_t>);
  friend Timeline pluss::codeh::ops_internal::timeline_from_epoch_nanos(std::span<const std::int64_t>,
                                                                        std::span<const std::int64_t>);

  friend Timeline detail::impl_add_duration(const Timeline&, const Duration&);
  friend Timeline detail::impl_sub_duration(const Timeline&, const Duration&);
  friend Timeline detail::impl_add_offset(const Timeline&, const CalendarOffset&);
  friend Timeline detail::impl_sub_offset(const Timeline&, const CalendarOffset&);
  friend Duration detail::impl_diff(const Timeline&, const Timeline&);
  friend pluss::codem::Series detail::impl_cmp(const Timeline&, const Timeline&, std::string_view);
  friend pluss::codem::Series detail::impl_dt_field(const Timeline&, std::string_view);
  friend pluss::codem::Series detail::impl_dt_predicate(const Timeline&, std::string_view);
  friend Timeline detail::impl_dt_round(const Timeline&, std::string_view, std::string_view);
  friend std::string detail::impl_isoformat(const Timeline&);
  friend pluss::coden::Note detail::impl_strftime(const Timeline&, std::string_view);
  friend Timeline detail::impl_date_range(const Timeline&, const Timeline&, std::string_view, std::int64_t);
  friend Timeline detail::impl_tz_convert(const Timeline&, std::string_view);
  friend Timeline detail::impl_tz_localize(const Timeline&, std::string_view, int, std::string_view);
  friend Timeline detail::impl_time_of_day(const Timeline&);
  friend pluss::codem::Series detail::impl_isnat(const Timeline&);
  friend pluss::codem::Series detail::impl_isvalid(const Timeline&);
  friend bool detail::impl_isnat_scalar(const Timeline&);
  friend pluss::codem::Series detail::impl_argsort(const Timeline&);
  friend Timeline detail::impl_min(const Timeline&);
  friend Timeline detail::impl_max(const Timeline&);
  friend Timeline detail::impl_sort(const Timeline&);
  friend Timeline detail::impl_unique(const Timeline&);
  friend Timeline detail::impl_fill_nat(const Timeline&, const Timeline&);
  friend Timeline detail::impl_where(const Timeline&, const pluss::codem::Series&, const Timeline&);
  friend pluss::codem::Series detail::impl_searchsorted(const Timeline&, const Timeline&, bool);
  friend Timeline detail::impl_add_nanos(const Timeline&, std::int64_t);
  friend std::vector<std::int64_t> detail::impl_to_epoch_nanos_flat(const Timeline&);
  friend pluss::coden::Note detail::impl_weekday_name(const Timeline&);
  friend pluss::coden::Note detail::impl_month_name(const Timeline&);

  explicit Timeline(pluss::codem::Series ticks, std::string tz_name = "UTC")
      : TimelineBase(std::move(ticks), std::move(tz_name)) {}
};

inline Timeline Timeline::from_buffer_epoch_nanos(std::int64_t* data,
                                                  std::span<const std::int64_t> sizes,
                                                  std::span<const std::int64_t> strides,
                                                  pluss::codem::BlobDeleter deleter,
                                                  std::string_view tz_name) {
  CODEM_CHECK(sizes.size() <= 3, "Timeline::from_buffer_epoch_nanos: only supports 0D..3D");
  auto ticks = pluss::codem::from_buffer(
      static_cast<void*>(data),
      sizes,
      strides,
      std::move(deleter),
      pluss::codem::ScalarType::Int64);
  return Timeline(std::move(ticks), std::string(tz_name));
}

} // namespace pluss::codeh
