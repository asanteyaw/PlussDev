#include <cstdint>
#include <iostream>
#include <string_view>

#include <pluss/pluss.h>

namespace {

void show_timeline(std::string_view label, const pluss::Timeline& t) {
  std::cout << "\n== " << label << " ==\n";
  std::cout << pluss::print_timeline(t) << "\n";
}

void show_duration(std::string_view label, const pluss::Duration& d) {
  std::cout << "\n== " << label << " ==\n";
  std::cout << "dim=" << d.dim() << " numel=" << d.numel() << "\n";
  std::cout << pluss::print_series(d.total_seconds()) << "\n";
}

void show_series(std::string_view label, const pluss::Series& s) {
  std::cout << "\n== " << label << " ==\n";
  std::cout << pluss::print_series(s) << "\n";
}

} // namespace

int main() {
  // ---- Base instant ----
  const auto t0 = pluss::make_timeline("1999-04-17T21:15:23Z");
  show_timeline("base_scalar", t0);

  // ---- Full scope: every Duration unit factory shifting the same instant ----
  show_timeline("plus_1ns", t0 + pluss::duration::nanos(1));
  show_timeline("plus_1us", t0 + pluss::duration::microseconds(1));
  show_timeline("plus_1ms", t0 + pluss::duration::milliseconds(1));
  show_timeline("plus_1s", t0 + pluss::duration::seconds(1));
  show_timeline("plus_1min", t0 + pluss::duration::minutes(1));
  show_timeline("plus_1h", t0 + pluss::duration::hours(1));
  show_timeline("plus_1d", t0 + pluss::duration::days(1));
  show_timeline("plus_1w", t0 + pluss::duration::weeks(1));

  // The original motivating case: adding 3 hours rolls the instant into the
  // next calendar day (21:15:23 + 3h = 00:15:23 the next day).
  show_timeline("plus_3h_crosses_day_boundary", t0 + pluss::duration::hours(3));

  // ---- Operator sugar is exactly the named method, not a separate path ----
  const auto via_operator_add = t0 + pluss::duration::hours(3);
  const auto via_method_add = t0.add_duration(pluss::duration::hours(3));
  show_series("operator_plus_equals_add_duration", via_operator_add.eq(via_method_add));

  const auto via_operator_sub = t0 - pluss::duration::hours(3);
  const auto via_method_sub = t0.sub_duration(pluss::duration::hours(3));
  show_series("operator_minus_equals_sub_duration", via_operator_sub.eq(via_method_sub));

  // ---- Compound assignment: t += d / t -= d mutate self, matching t + d / t - d ----
  auto compound_add_t = t0;
  compound_add_t += pluss::duration::hours(3);
  show_timeline("compound_add_t0_plus_equals_3h", compound_add_t);
  show_series("compound_add_equals_binary_plus", compound_add_t.eq(via_operator_add));

  auto compound_sub_t = t0;
  compound_sub_t -= pluss::duration::hours(3);
  show_timeline("compound_sub_t0_minus_equals_3h", compound_sub_t);
  show_series("compound_sub_equals_binary_minus", compound_sub_t.eq(via_operator_sub));

  // ---- Timeline - Timeline -> Duration, operator vs method ----
  const auto t1 = pluss::make_timeline("1999-04-18T00:15:23Z"); // t0 + 3h
  const auto diff_operator = t1 - t0;
  const auto diff_method = t1.diff(t0);
  show_duration("diff_via_operator_t1_minus_t0", diff_operator);
  show_series("diff_operator_equals_diff_method", diff_operator.eq(diff_method));

  // ---- Broadcasting: scalar Timeline + 1D Duration -> 1D Timeline ----
  const auto steps = pluss::duration::hours({1, 2, 3, 4});
  show_timeline("broadcast_scalar_timeline_plus_1d_duration", t0 + steps);

  // ---- Broadcasting: 1D Timeline + scalar Duration -> 1D Timeline ----
  const auto t_vec = pluss::make_timeline({
      "2024-11-01T09:00:00Z",
      "2024-11-01T10:00:00Z",
      "2024-11-01T11:00:00Z",
      "2024-11-01T12:00:00Z",
  });
  show_timeline("broadcast_1d_timeline_plus_scalar_duration", t_vec + pluss::duration::minutes(30));

  // ---- Elementwise: matching-shape 1D Timeline + 1D Duration ----
  show_timeline("elementwise_1d_timeline_plus_1d_duration", t_vec + steps);

  // ---- NaT propagates from either side ----
  const auto t_missing = pluss::make_timeline(""); // documented missing-timestamp text
  show_timeline("nat_timeline_plus_valid_duration", t_missing + pluss::duration::hours(1));

  const auto d_missing = pluss::duration::nanos(pluss::Duration::kNaT);
  show_timeline("valid_timeline_plus_nat_duration", t0 + d_missing);

  // ---- Timezone label is preserved across add/sub (Duration itself has no tz) ----
  const auto t0_ny = t0.tz_convert("America/New_York");
  show_timeline("tz_before_shift_ny", t0_ny);
  show_timeline("tz_after_add_duration_ny", t0_ny + pluss::duration::hours(3));

  return 0;
}
