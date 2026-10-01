#include <array>
#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <pluss/pluss.h>

namespace {

void show_shape(const pluss::Duration& d) {
  std::cout << '[';
  for (std::size_t i = 0; i < d.sizes().size(); ++i) {
    if (i != 0) std::cout << ',';
    std::cout << d.sizes()[i];
  }
  std::cout << ']';
}

void show_duration(std::string_view label, const pluss::Duration& d) {
  std::cout << "\n== " << label << " ==\n";
  std::cout << "dim=" << d.dim() << " numel=" << d.numel() << "\n";
  std::cout << pluss::series::print(d.total().nanoseconds()) << "\n";
}

void show_series(std::string_view label, const pluss::Series& s) {
  std::cout << "\n== " << label << " ==\n";
  std::cout << pluss::series::print(s) << "\n";
}

void show_timeline(std::string_view label, const pluss::Timeline& t) {
  std::cout << "\n== " << label << " ==\n";
  std::cout << pluss::timeline::print(t) << "\n";
}

void show_note(std::string_view label, const pluss::Note& note) {
  std::cout << "\n== " << label << " ==\n";
  std::cout << note << "\n";
}

template <typename F>
void show_error(std::string_view label, F&& call) {
  std::cout << "\n== " << label << " ==\n";
  try {
    call();
    std::cout << "no error\n";
  } catch (const std::exception& error) {
    const std::string message(error.what());
    std::cout << message.substr(0, message.find('\n')) << "\n";
  }
}

} // namespace

int main() {
  // ---- Foundations and explicit host extraction ----
  const auto meta_scalar = pluss::duration::seconds(90);
  const auto meta_singleton = pluss::duration::seconds({90});
  const auto meta_vector = pluss::duration::minutes({15, 30, 45});
  const auto meta_matrix = pluss::duration::hours({1, 2, 3, 4}, {2, 2});
  const auto meta_cube = pluss::duration::nanos({1, 2, 3, 4}, {2, 1, 2});
  const auto meta_empty = pluss::duration::zeros({2, 0, 3});

  std::cout << "== metadata ==\n";
  for (const auto* value : {&meta_scalar, &meta_singleton, &meta_vector,
                            &meta_matrix, &meta_cube, &meta_empty}) {
    std::cout << "defined=" << std::boolalpha << value->defined()
              << " dim=" << value->dim()
              << " numel=" << value->numel()
              << " sizes=";
    show_shape(*value);
    std::cout << '\n';
  }

  const auto exact = pluss::duration::nanos(
      {9'007'199'254'740'993LL, -1LL, pluss::Duration::kNaT});
  const auto nat_scalar = pluss::duration::nanos(pluss::Duration::kNaT);
  std::cout << "defined=" << nat_scalar.defined()
            << " isnat=" << nat_scalar.isnat().item<bool>() << "\n";
  std::cout << "host=[";
  const auto exact_host = exact.host();
  for (std::size_t i = 0; i < exact_host.size(); ++i) {
    if (i != 0) std::cout << ',';
    std::cout << exact_host[i];
  }
  std::cout << "]\n";

  const auto flat_make = pluss::make_duration(
      {1, 2, 3, 4}, {2, 2}, pluss::duration::Unit::Seconds);
  std::cout << "make_duration dim=" << flat_make.dim()
            << " numel=" << flat_make.numel() << " sizes=";
  show_shape(flat_make);
  std::cout << "\n";
  const auto canonical_make = pluss::duration::make(
      {1, 2, 3, 4}, {2, 2}, pluss::duration::Unit::Seconds);
  std::cout << "same_shape=" << (canonical_make.sizes() == flat_make.sizes())
            << " same_values=" << canonical_make.eq(flat_make).all().item<bool>() << "\n";
  const auto flat_host = flat_make.host();
  std::cout << "matrix_host=[";
  for (std::size_t i = 0; i < flat_host.size(); ++i) {
    if (i != 0) std::cout << ',';
    std::cout << flat_host[i];
  }
  std::cout << "]\n";
  auto owned_host = meta_vector.host();
  owned_host[0] = 0;
  std::cout << "copy_first=" << owned_host[0]
            << " source_first=" << meta_vector.host()[0] << "\n";

  // ---- Normalized component extraction ----
  constexpr std::int64_t kDay = 86'400'000'000'000LL;
  constexpr std::int64_t kHour = 3'600'000'000'000LL;
  constexpr std::int64_t kMinute = 60'000'000'000LL;
  constexpr std::int64_t kSecond = 1'000'000'000LL;
  constexpr std::int64_t kMillisecond = 1'000'000LL;
  constexpr std::int64_t kMicrosecond = 1'000LL;
  constexpr std::int64_t composite =
      kDay + 2 * kHour + 3 * kMinute + 4 * kSecond +
      5 * kMillisecond + 6 * kMicrosecond + 7;

  const auto component_positive = pluss::duration::nanos(composite);
  const auto component_negative =
      pluss::duration::nanos({-1, -kHour, -kDay});
  const auto component_matrix = pluss::duration::nanos(
      {composite, pluss::Duration::kNaT, -1, 0}, {2, 2});
  const auto component_empty = pluss::duration::zeros({2, 0, 3});

#define SHOW_COMPONENT(name)                                                        \
  show_series("component_" #name "_positive", component_positive.name());         \
  show_series("component_" #name "_negative", component_negative.name());         \
  show_series("component_" #name "_matrix", component_matrix.name());             \
  show_series("component_" #name "_empty", component_empty.name())

  SHOW_COMPONENT(days);
  SHOW_COMPONENT(hours);
  SHOW_COMPONENT(minutes);
  SHOW_COMPONENT(seconds);
  SHOW_COMPONENT(milliseconds);
  SHOW_COMPONENT(microseconds);
  SHOW_COMPONENT(nanoseconds);

#undef SHOW_COMPONENT

  // ---- Rounding to fixed units ----
  const auto rounding = pluss::duration::nanos({
      -1'600'000'000LL, -1'500'000'000LL, -500'000'000LL,
       500'000'000LL,  1'500'000'000LL, 2'500'000'000LL,
       1'400'000'000LL, pluss::Duration::kNaT,
  });
  show_duration("floor_seconds", rounding.floor("s"));
  show_duration("ceil_seconds", rounding.ceil("s"));
  show_duration("round_seconds_half_even", rounding.round("s"));
  show_duration("floor_canonical_units",
                pluss::duration::nanos(93'784'005'006'007LL).floor("h"));
  show_duration("round_matrix_shape",
                pluss::duration::nanos({499'999'999, 500'000'000,
                                        1'499'999'999, 1'500'000'000},
                                       {2, 2}).round("s"));
  show_error("round_invalid_unit", [&] {
    (void)pluss::duration::seconds(1).round("second");
  });
  show_error("ceil_overflow", [&] {
    (void)pluss::duration::nanos(
        (std::numeric_limits<std::int64_t>::max)()).ceil("D");
  });

  // ---- Global extrema ----
  const auto extrema = pluss::duration::nanos(
      {5, pluss::Duration::kNaT, -2, 9}, {2, 2});
  show_duration("min_skip_nat", extrema.min());
  show_duration("max_skip_nat", extrema.max());
  show_duration("min_propagate_nat", extrema.min(false));
  show_duration("max_propagate_nat", extrema.max(false));
  show_duration("min_all_nat", pluss::duration::nanos(
      {pluss::Duration::kNaT, pluss::Duration::kNaT}).min());
  show_duration("max_empty", pluss::duration::nanos(
      std::vector<std::int64_t>{}).max());
  std::cout << "\n== min_skip_nat_print ==\n"
            << pluss::duration::print(extrema.min()) << "\n";
  std::cout << "\n== max_skip_nat_print ==\n"
            << pluss::duration::print(extrema.max()) << "\n";
  std::cout << "\n== min_propagate_nat_print ==\n"
            << pluss::duration::print(extrema.min(false)) << "\n";
  std::cout << "\n== max_empty_print ==\n"
            << pluss::duration::print(
                   pluss::duration::nanos(std::vector<std::int64_t>{}).max())
            << "\n";

  // ---- Stable one-dimensional ordering and first-occurrence uniqueness ----
  const auto ordering = pluss::duration::nanos(
      {3, pluss::Duration::kNaT, 1, 3, pluss::Duration::kNaT, 2});
  show_duration("sort_stable_nat_last", ordering.sort());
  const auto order = ordering.argsort();
  show_series("argsort_stable_nat_last", order);
  const auto source_nanos = ordering.host();
  std::vector<std::int64_t> gathered;
  gathered.reserve(source_nanos.size());
  for (const auto index : order.to_vec<std::int64_t>()) {
    gathered.push_back(source_nanos[static_cast<std::size_t>(index)]);
  }
  show_duration("argsort_applied", pluss::duration::nanos(gathered));
  show_duration("unique_first_occurrence", ordering.unique());
  show_duration("sort_empty",
                pluss::duration::nanos(std::vector<std::int64_t>{}).sort());
  std::cout << "\n== sort_stable_nat_last_print ==\n"
            << pluss::duration::print(ordering.sort()) << "\n";
  std::cout << "\n== argsort_applied_print ==\n"
            << pluss::duration::print(pluss::duration::nanos(gathered)) << "\n";
  std::cout << "\n== unique_first_occurrence_print ==\n"
            << pluss::duration::print(ordering.unique()) << "\n";
  std::cout << "\n== sort_empty_print ==\n"
            << pluss::duration::print(
                   pluss::duration::nanos(std::vector<std::int64_t>{}).sort())
            << "\n";
  show_error("sort_rank_error", [&] {
    (void)pluss::duration::nanos({1, 2, 3, 4}, {2, 2}).sort();
  });

  // ---- Text projections ----
  const auto formatted = pluss::duration::nanos(
      {93'784'005'006'007LL, -1, 0, pluss::Duration::kNaT});
  show_note("to_note_normalized", formatted.to_note());
  show_note("to_note_iso", formatted.to_note_iso());
  show_note("to_note_matrix",
            pluss::duration::nanos({0, -1, 1'000'000'000, 86'400'000'000'000LL},
                                   {2, 2}).to_note());
  show_note("to_note_iso_matrix",
            pluss::duration::nanos({0, -1, 1'000'000'000, 86'400'000'000'000LL},
                                   {2, 2}).to_note_iso());
  show_note("to_note_iso_fraction_trim",
            pluss::duration::nanos({1'500'000'000, 1'000'000'001})
                .to_note_iso());

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
  const auto operator_cmp_left = pluss::duration::minutes({30, 60, 90});
  show_series("operator_lt_vector_scalar", operator_cmp_left < one_hour);
  show_series("operator_ge_vector_scalar", operator_cmp_left >= one_hour);

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
  std::cout << "\n== operator_lshift_negative_tick ==\n"
            << pluss::duration::nanos(-1) << "\n";
  show_error("operator_add_overflow", [&] {
    (void)(pluss::duration::nanos(
               (std::numeric_limits<std::int64_t>::max)()) +
           pluss::duration::nanos(1));
  });

  // ---- Timeline integration ----
  const auto start = pluss::timeline::make("2024-11-01T09:00:00Z");
  const auto end = pluss::timeline::make("2024-11-01T10:30:00Z");
  show_timeline("timeline_plus_duration", start + ninety_min);
  show_timeline("timeline_minus_duration", start - ninety_min);
  show_duration("timeline_diff", end - start);

  return 0;
}
