#include <pluss/pluss.h>

#include <iostream>
#include <limits>
#include <string>

namespace {

void heading(const char* label) {
  std::cout << "=== " << label << " ===\n";
}

void show(const pluss::Series& value) {
  pluss::series::print(std::cout, value) << "\n\n";
}

} // namespace

int main() {
  const double nan = std::numeric_limits<double>::quiet_NaN();

  heading("nan_skipping_extrema");
  const auto left = pluss::make_series({1.0, nan, 4.0, nan});
  const auto right = pluss::make_series({2.0, 3.0, 4.0, nan});
  show(left.fmax(right));
  show(left.fmin(right));

  heading("extrema_reverse");
  auto a = pluss::make_series({1.0, 4.0, 3.0, nan});
  auto b = pluss::make_series({2.0, 4.0, nan, 5.0});
  a.tracks_slope_(true);
  b.tracks_slope_(true);
  a.fmax(b).sum().reverse();
  show(a.sensitivity());
  show(b.sensitivity());

  heading("heaviside_scalar_conventions");
  const auto step_input = pluss::make_series({-2.0, -0.0, 0.0, 3.0, nan});
  show(step_input.heaviside(0.0));
  show(step_input.heaviside(0.5));
  show(step_input.heaviside(1.0));

  heading("heaviside_broadcast");
  show(pluss::make_series({{-2.0, 0.0, 3.0}, {0.0, -1.0, 2.0}})
           .heaviside(pluss::make_series({{0.25, 0.5, 0.75}})));

  heading("gcd_lcm_broadcast");
  const auto integers = pluss::make_series({{-12, 18, 0}, {20, -27, 14}});
  const auto factors = pluss::make_series({{8, 6, 5}});
  show(integers.gcd(factors));
  show(integers.lcm(factors));

  heading("gcd_conventions");
  show(pluss::make_series({12, 18, 0, -15})
           .gcd(pluss::make_series({8, 24, 5, 9})));
  try {
    show(pluss::make_series({12.0}).gcd(pluss::make_series({8.0})));
  } catch (const std::exception& error) {
    const std::string message = error.what();
    std::cout << message.substr(0, message.find('\n')) << "\n\n";
  }

  heading("lcm_conventions");
  show(pluss::make_series({4, 6, 0}).lcm(pluss::make_series({6, 8, 5})));

  heading("gcd_lcm_complement");
  const auto periods = pluss::make_series({12, 18, 14});
  const auto references = pluss::make_series({8, 6, 5});
  show(periods.gcd(references));
  show(periods.lcm(references));

  heading("frexp_decomposition");
  auto [mantissa, exponent] = pluss::make_series({1.0, 6.0, 0.75, 0.0, -8.0}).frexp();
  show(mantissa);
  show(exponent);
  show(mantissa.ldexp(exponent));

  heading("frexp_round_trip");
  auto [round_trip_mantissa, round_trip_exponent] =
      pluss::make_series({3.0, 100.0, 0.1}).frexp();
  show(round_trip_mantissa.ldexp(round_trip_exponent));
  try {
    auto [invalid_mantissa, invalid_exponent] = pluss::make_series({1, 2, 3}).frexp();
    show(invalid_mantissa);
  } catch (const std::exception& error) {
    const std::string message = error.what();
    std::cout << message.substr(0, message.find('\n')) << "\n\n";
  }

  heading("frexp_reverse");
  auto decomposed = pluss::make_series({1.0, 6.0, 0.75, -8.0});
  decomposed.tracks_slope_(true);
  auto [fraction, ignored_exponent] = decomposed.frexp();
  fraction.sum().reverse();
  show(decomposed.sensitivity());

  heading("ldexp_continuous");
  show(pluss::make_series({2.0, 3.0}).ldexp(pluss::make_series({0.5, -1.0})));

  heading("ldexp_basic");
  show(pluss::make_series({1.0, 1.5, 3.0})
           .ldexp(pluss::make_series({3, 0, -1})));
  show(pluss::make_series({1.0, 2.0, 3.0}).ldexp(2.0));

  heading("ldexp_receiver_reverse");
  auto receiver_only = pluss::make_series({3.0, 5.0});
  receiver_only.tracks_slope_(true);
  receiver_only.ldexp(pluss::make_series({1, 2})).sum().reverse();
  show(receiver_only.sensitivity());

  heading("ldexp_reverse");
  auto scaled = pluss::make_series({2.0, 3.0});
  auto scale_exponent = pluss::make_series({0.5, -1.0});
  scaled.tracks_slope_(true);
  scale_exponent.tracks_slope_(true);
  scaled.ldexp(scale_exponent).sum().reverse();
  show(scaled.sensitivity());
  show(scale_exponent.sensitivity());

  heading("float_power_promotion");
  show(pluss::make_series({2, 4}).float_power(pluss::make_series({-1, 3})));
  show(pluss::make_series({4.0, 9.0, 2.0}).float_power(0.5));

  heading("float_power_documented_forms");
  show(pluss::make_series({2, 3, 4})
           .float_power(pluss::make_series({3, 2, 0})));
  show(pluss::make_series({4.0, 9.0, 2.0})
           .float_power(pluss::make_series({0.5, 0.5, 10.0})));
  show(pluss::make_series({2, 3}).float_power(2.0));

  heading("floor_divide_signs");
  show(pluss::make_series({7, -7, 8, -8})
           .floor_divide(pluss::make_series({2, 2, -3, -3})));
  show(pluss::make_series({7, 8}).floor_divide(2.5));
  show(pluss::make_series({7.5, -7.5})
           .floor_divide(pluss::make_series({2.0, 2.0})));

  heading("floor_divide_in_place");
  auto integer_quotients = pluss::make_series({7, -7});
  const void* quotient_storage = integer_quotients.raw_data();
  integer_quotients.floor_divide_(2.0);
  pluss::series::print(std::cout, integer_quotients) << '\n';
  std::cout << "same storage: " << std::boolalpha
            << (quotient_storage == integer_quotients.raw_data()) << "\n\n";

  heading("in_place_storage");
  auto mutable_values = pluss::make_series({1.0, 2.0, 3.0});
  const void* before = mutable_values.raw_data();
  mutable_values.ldexp_(pluss::make_series({1, 0, -1}));
  show(mutable_values);
  std::cout << "same storage: " << std::boolalpha
            << (before == mutable_values.raw_data()) << "\n";

  return 0;
}
