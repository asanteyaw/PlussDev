#include <array>
#include <cstdint>
#include <iostream>
#include <string_view>

#include <pluss/pluss.h>

namespace {

void show_duration(std::string_view label, const pluss::Duration& d) {
  std::cout << "\n== " << label << " ==\n";
  std::cout << "dim=" << d.dim() << " numel=" << d.numel() << "\n";
  std::cout << pluss::print_series(d.total().nanoseconds()) << "\n";
}

void show_series(std::string_view label, const pluss::Series& s) {
  std::cout << "\n== " << label << " ==\n";
  std::cout << pluss::print_series(s) << "\n";
}

void show_timeline(std::string_view label, const pluss::Timeline& t) {
  std::cout << "\n== " << label << " ==\n";
  std::cout << pluss::timeline::print(t) << "\n";
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
  show_series("floordiv_ranged_by_3_totaldays", ranged.floordiv(3).total().days());
  constexpr std::int64_t kThreeDaysNanos = 3LL * 86'400 * 1'000'000'000LL;
  show_series("mod_ranged_by_3days_totaldays", ranged.mod(kThreeDaysNanos).total().days());

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
  show_series("cmp_ne_1h_90m", one_hour.ne(ninety_min));
  show_series("cmp_lt_1h_90m", one_hour.lt(ninety_min));
  show_series("cmp_le_1h_90m", one_hour.le(ninety_min));
  show_series("cmp_gt_1d_1h", one_day.gt(one_hour));
  show_series("cmp_ge_1d_1h", one_day.ge(one_hour));

  // ---- Unary transforms ----
  show_duration("abs_neg15m", neg_fifteen_min.abs());
  show_duration("sign_neg15m", neg_fifteen_min.sign());
  show_duration("sign_90m", ninety_min.sign());
  show_duration("clip_ranged_2d_6d",
                ranged.clip(pluss::duration::days(2), pluss::duration::days(6)));

  // ---- Unit conversions: all via the total() accessor -- there are no flat
  // total_nanoseconds()/total_seconds()/etc. methods on Duration itself. ----
  show_series("total_nanoseconds_90m", ninety_min.total().nanoseconds());
  show_series("total_microseconds_90m", ninety_min.total().microseconds());
  show_series("total_milliseconds_90m", ninety_min.total().milliseconds());
  show_series("total_seconds_90m", ninety_min.total().seconds());
  show_series("total_minutes_90m", ninety_min.total().minutes());
  show_series("total_hours_90m", ninety_min.total().hours());
  show_series("total_days_ranged", ranged.total().days());
  show_series("total_weeks_21d", pluss::duration::days(21).total().weeks());

  // ---- Missing-value handling ----
  const auto with_nat = pluss::duration::nanos(pluss::Duration::kNaT);
  show_series("isnat_valid", one_hour.isnat());
  show_series("isnat_missing", with_nat.isnat());
  show_series("isvalid_valid", one_hour.isvalid());
  show_series("isvalid_missing", with_nat.isvalid());
  show_duration("fill_nat", with_nat.fill_nat(pluss::duration::hours(2)));

  // ---- where: mask=true keeps self, mask=false selects other ----
  const auto where_mask = ranged.gt(pluss::duration::days(4));
  show_series("where_mask_gt4days", where_mask);
  const auto where_replacement = pluss::duration::full({6}, pluss::duration::days(99));
  show_series("where_result_totaldays", ranged.where(where_mask, where_replacement).total().days());

  // ---- pluss::duration::print() on Duration-returning operation results --
  // the human-readable form, complementing the total_*()-based numeric
  // checks above (each of these is documented on its own method's page,
  // e.g. api/duration/add.md, using this exact verified output). ----
  std::cout << "\n== add_print ==\n" << pluss::duration::print(ninety_min + one_hour) << "\n";
  std::cout << "\n== sub_print ==\n" << pluss::duration::print(one_day - one_hour) << "\n";
  std::cout << "\n== neg_print ==\n" << pluss::duration::print(-one_hour) << "\n";
  std::cout << "\n== mul_print ==\n" << pluss::duration::print(one_hour * 3) << "\n";
  std::cout << "\n== div_print ==\n" << pluss::duration::print(one_day / 4) << "\n";
  std::cout << "\n== floordiv_print ==\n" << pluss::duration::print(ranged.floordiv(3)) << "\n";
  std::cout << "\n== mod_print ==\n" << pluss::duration::print(ranged.mod(kThreeDaysNanos)) << "\n";
  std::cout << "\n== abs_print ==\n" << pluss::duration::print(neg_fifteen_min.abs()) << "\n";
  std::cout << "\n== sign_print ==\n" << pluss::duration::print(neg_fifteen_min.sign()) << "\n";
  std::cout << "\n== clip_print ==\n"
            << pluss::duration::print(
                   ranged.clip(pluss::duration::days(2), pluss::duration::days(6)))
            << "\n";
  std::cout << "\n== fill_nat_print ==\n"
            << pluss::duration::print(with_nat.fill_nat(pluss::duration::hours(2))) << "\n";

  // ---- Printing: pluss::duration::print(d) and std::cout << d directly ----
  std::cout << "\n== print_scalar ==\n" << pluss::duration::print(ninety_min) << "\n";
  std::cout << "\n== print_1d ==\n" << pluss::duration::print(ranged) << "\n";
  std::cout << "\n== print_nat ==\n" << pluss::duration::print(with_nat) << "\n";
  std::cout << "\n== operator_lshift_scalar ==\n" << ninety_min << "\n";
  std::cout << "\n== operator_lshift_equals_print ==\n"
            << (pluss::duration::print(ninety_min) == [&] {
                 std::ostringstream oss; oss << ninety_min; return oss.str();
               }()) << "\n";

  // ---- Timeline integration ----
  const auto start = pluss::timeline::make("2024-11-01T09:00:00Z");
  const auto end = pluss::timeline::make("2024-11-01T10:30:00Z");
  show_timeline("timeline_plus_duration", start + ninety_min);
  show_timeline("timeline_minus_duration", start - ninety_min);
  show_duration("timeline_diff", end - start);

  return 0;
}
