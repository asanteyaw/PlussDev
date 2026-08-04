#include <array>
#include <cstdint>
#include <iostream>
#include <string_view>

#include <pluss/pluss.h>

namespace {

void show_duration(std::string_view label, const pluss::Duration& d) {
  std::cout << "\n== " << label << " ==\n";
  std::cout << "dim=" << d.dim() << " numel=" << d.numel() << "\n";
  std::cout << pluss::print_series(d.total_nanoseconds()) << "\n";
}

void show_series(std::string_view label, const pluss::Series& s) {
  std::cout << "\n== " << label << " ==\n";
  std::cout << pluss::print_series(s) << "\n";
}

void show_timeline(std::string_view label, const pluss::Timeline& t) {
  std::cout << "\n== " << label << " ==\n";
  std::cout << pluss::print_timeline(t) << "\n";
}

} // namespace

int main() {
  // ---- Construction ----
  const auto ninety_min = pluss::duration::minutes(90);
  const auto one_hour = pluss::duration::hours(1);
  const auto one_day = pluss::duration::days(1);
  const auto neg_fifteen_min = pluss::duration::minutes(-15);

  show_duration("ctor_duration_minutes_90", ninety_min);
  show_duration("ctor_duration_hours_1", one_hour);
  show_duration("ctor_duration_days_1", one_day);
  show_duration("ctor_duration_minutes_neg15", neg_fifteen_min);

  const auto ranged = pluss::duration::range(
      pluss::duration::days(0), pluss::duration::days(10), pluss::duration::days(2));
  show_duration("ctor_timedelta_range_0_10_step2", ranged);

  // ---- N-D construction: every unit factory has scalar/1D/nested-N-D/
  // flat+shape overloads, not just scalar. ----
  const auto minutes_1d = pluss::duration::minutes({15, 30, 45, 60});
  show_duration("ctor_minutes_1d_list", minutes_1d);

  const auto hours_2d = pluss::duration::hours({{1, 2}, {3, 4}, {5, 6}});
  show_duration("ctor_hours_nested_2d", hours_2d);

  // Plain braces convert implicitly to pluss::RangeView<int64_t> -- no
  // explicit wrapping needed even for the flat+shape overload.
  const auto days_flat_shape = pluss::duration::days({1, 2, 3, 4}, {2, 2});
  show_duration("ctor_days_flat_plus_shape_2x2", days_flat_shape);

  // pluss::duration::make(..., Unit) -- the same construction surface, with
  // the unit picked at runtime instead of by function name.
  const auto made_scalar = pluss::duration::make(3, pluss::duration::Unit::Days);
  const auto made_1d = pluss::duration::make({1, 2, 3}, pluss::duration::Unit::Hours);
  show_duration("ctor_make_scalar_3_days", made_scalar);
  show_duration("ctor_make_1d_hours", made_1d);

  // ---- Arithmetic ----
  show_duration("add_90m_plus_1h", ninety_min + one_hour);
  show_duration("sub_1d_minus_1h", one_day - one_hour);
  show_duration("neg_90m", -ninety_min);
  show_duration("mul_1h_times_3", one_hour * 3);
  show_duration("div_1d_by_4", one_day / 4);
  show_duration("floordiv_ranged_by_3", ranged.floordiv(3));
  constexpr std::int64_t kThreeDaysNanos = 3LL * 86'400 * 1'000'000'000LL;
  show_series("mod_ranged_by_3days_totaldays", ranged.mod(kThreeDaysNanos).total_days());

  // ---- Compound assignment: += and -= mutate self, matching operator+/- ----
  auto compound_add = one_hour;
  compound_add += ninety_min;
  show_duration("compound_add_1h_plus_equals_90m", compound_add);
  show_series("compound_add_equals_binary_plus", compound_add.eq(one_hour + ninety_min));

  auto compound_sub = one_day;
  compound_sub -= one_hour;
  show_duration("compound_sub_1d_minus_equals_1h", compound_sub);
  show_series("compound_sub_equals_binary_minus", compound_sub.eq(one_day - one_hour));

  // ---- Comparisons (elementwise, return Series[bool]) ----
  show_series("cmp_eq_1h_90m", one_hour.eq(ninety_min));
  show_series("cmp_lt_1h_90m", one_hour.lt(ninety_min));
  show_series("cmp_ge_1d_1h", one_day.ge(one_hour));

  // ---- Unary transforms ----
  show_duration("abs_neg15m", neg_fifteen_min.abs());
  show_duration("sign_neg15m", neg_fifteen_min.sign());
  show_duration("sign_90m", ninety_min.sign());
  show_duration("clip_ranged_2d_6d",
                ranged.clip(pluss::duration::days(2), pluss::duration::days(6)));

  // ---- Unit conversions ----
  show_series("total_seconds_90m", ninety_min.total_seconds());
  show_series("total_minutes_90m", ninety_min.total_minutes());
  show_series("total_hours_90m", ninety_min.total_hours());
  show_series("total_days_ranged", ranged.total_days());

  // ---- Missing-value handling ----
  const auto with_nat = pluss::duration::nanos(pluss::Duration::kNaT);
  show_series("isnat_valid", one_hour.isnat());
  show_series("isnat_missing", with_nat.isnat());
  show_duration("fill_nat", with_nat.fill_nat(pluss::duration::hours(2)));

  // ---- Timeline integration ----
  const auto start = pluss::make_timeline("2024-11-01T09:00:00Z");
  const auto end = pluss::make_timeline("2024-11-01T10:30:00Z");
  show_timeline("timeline_plus_duration", start + ninety_min);
  show_timeline("timeline_minus_duration", start - ninety_min);
  show_duration("timeline_diff", end - start);

  return 0;
}
