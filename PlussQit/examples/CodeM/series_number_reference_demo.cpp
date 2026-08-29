#include <pluss/pluss.h>

#include <iostream>
#include <limits>
#include <string>

namespace {

void heading(const char* label) {
  std::cout << "=== " << label << " ===\n";
}

void print_error(const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}

} // namespace

int main() {
  const auto ints = pluss::make_series({-2, -1, 0, 1, 2, 3, 4, 17}, pluss::ScalarType::Int64);

  heading("is_prime_basic");
  pluss::series::print(std::cout, ints.is_prime()) << "\n\n";

  heading("is_prime_shape_preserved");
  std::cout << pluss::make_series({
      {2, 4, 5},
      {15, 17, 19},
  }, pluss::ScalarType::Int64).is_prime() << "\n\n";

  heading("is_prime_unsigned_input");
  std::cout << pluss::make_series({2, 9, 11, 12}, pluss::ScalarType::UInt32).is_prime() << "\n\n";

  heading("is_prime_dtype_error");
  try {
    std::cout << pluss::make_series({2.0, 3.0, 4.0}).is_prime() << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  const auto ordinals = pluss::make_series({1, 2, 3, 5}, pluss::ScalarType::Int64);

  heading("prime_basic");
  std::cout << ordinals.prime() << "\n\n";

  heading("prime_shape_preserved");
  pluss::series::print(std::cout, pluss::make_series({
      {1, 4},
      {6, 8},
  }, pluss::ScalarType::Int64).prime()) << "\n\n";

  heading("prime_bool_input");
  std::cout << pluss::make_series({true, true, true, true}, pluss::ScalarType::Bool).prime() << "\n\n";

  heading("prime_zero_error");
  try {
    std::cout << pluss::make_series({0}, pluss::ScalarType::Int64).prime() << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  heading("prime_large_n_error");
  try {
    std::cout << pluss::make_series({200001}, pluss::ScalarType::Int64).prime() << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  const auto values = pluss::make_series({0, 1, 2, 12, 49, -15}, pluss::ScalarType::Int64);

  heading("factor_basic");
  pluss::series::print(std::cout, values.factor()) << "\n\n";

  heading("factor_shape_preserved");
  std::cout << pluss::make_series({
      {-21, 25},
      {27, 29},
  }, pluss::ScalarType::Int64).factor() << "\n\n";

  heading("factor_unsigned_input");
  std::cout << pluss::make_series({0, 1, 14, 25}, pluss::ScalarType::UInt16).factor() << "\n\n";

  heading("factor_min_value_error");
  try {
    std::cout << pluss::make_series({std::numeric_limits<std::int64_t>::min()}, pluss::ScalarType::Int64).factor() << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  return 0;
}
