#include <pluss/pluss.h>

#include <cmath>
#include <iostream>
#include <string_view>

namespace {

void heading(const char* name) { std::cout << "=== " << name << " ===\n"; }

template <class Range>
void print_range(const Range& values) {
  std::cout << '[';
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i) std::cout << ',';
    std::cout << values[i];
  }
  std::cout << ']';
}

void print_error(const std::exception& error) {
  const std::string_view message(error.what());
  std::cout << message.substr(0, message.find('\n')) << '\n';
}

} // namespace

int main() {
  std::cout << std::boolalpha;

  heading("bernoulli_probability_receiver");
  const auto probabilities = pluss::make_series({0.0, 1.0, 0.0, 1.0});
  pluss::series::print(std::cout, probabilities.bernoulli());
  std::cout << "\n\n";

  heading("in_place_aliasing");
  auto destination = pluss::make_series({9.0, 9.0, 9.0});
  auto alias = destination;
  destination.bernoulli_(1.0);
  std::cout << "same storage: " << destination.is_set_to(alias) << '\n';
  pluss::series::print(std::cout, alias);
  std::cout << "\n\n";

  heading("batched_multinomial");
  const auto rows = pluss::make_series({{0.0, 1.0, 0.0}, {1.0, 0.0, 0.0}});
  pluss::series::print(std::cout, rows.multinomial(4, true));
  std::cout << "\n\n";

  heading("location_scale_degenerate_cases");
  auto normal = pluss::Series::empty({3}, pluss::ScalarType::Float64);
  normal.normal_(3.5, 0.0);
  pluss::series::print(std::cout, normal);
  std::cout << '\n';
  auto uniform = pluss::Series::empty({3}, pluss::ScalarType::Float64);
  uniform.uniform_(-2.0, -2.0);
  pluss::series::print(std::cout, uniform);
  std::cout << "\n\n";

  heading("discrete_random_on_float_storage");
  auto random = pluss::Series::empty({8}, pluss::ScalarType::Float64);
  random.random_(5, 6);
  pluss::series::print(std::cout, random);
  std::cout << "\n\n";

  heading("distribution_invariants");
  auto values = pluss::Series::empty({64}, pluss::ScalarType::Float64);
  values.exponential_(2.0);
  bool exponential_nonnegative = true;
  for (double value : values.host<double>()) exponential_nonnegative &= value >= 0.0;
  values.geometric_(0.25);
  bool geometric_positive_integer = true;
  for (double value : values.host<double>())
    geometric_positive_integer &= value >= 1.0 && value == std::floor(value);
  values.log_normal_(0.0, 0.5);
  bool log_normal_positive = true;
  for (double value : values.host<double>()) log_normal_positive &= value > 0.0;
  values.cauchy_(2.0, 0.5);
  std::cout << "exponential >= 0: " << exponential_nonnegative << '\n';
  std::cout << "geometric positive integers: " << geometric_positive_integer << '\n';
  std::cout << "log-normal > 0: " << log_normal_positive << '\n';
  std::cout << "cauchy shape=";
  print_range(values.sizes());
  std::cout << ", dtype=" << values.dtype() << "\n\n";

  heading("contract_failures");
  try { pluss::make_series({-0.1, 0.5}).bernoulli(); }
  catch (const std::exception& error) { print_error(error); }
  try { pluss::make_series({0.0, 0.0}).multinomial(1); }
  catch (const std::exception& error) { print_error(error); }
  try { values.geometric_(1.0); }
  catch (const std::exception& error) { print_error(error); }
  try { values.log_normal_(0.0, 0.0); }
  catch (const std::exception& error) { print_error(error); }
}
