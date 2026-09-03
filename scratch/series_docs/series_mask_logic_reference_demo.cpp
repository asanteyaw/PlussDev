#include <pluss/pluss.h>

#include <iostream>
#include <string>

namespace {

void heading(const char* label) {
  std::cout << "=== " << label << " ===\n";
}

void print_error(const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}

}  // namespace

int main() {
  auto x = pluss::make_series({
      {1, 2, 3},
      {4, 5, 6},
  }, pluss::ScalarType::Int32);
  auto mask = pluss::make_series({
      {true, false, true},
      {false, false, true},
  }, pluss::ScalarType::Bool);
  heading("masked_select_basic");
  std::cout << x.masked_select(mask) << "\n\n";

  auto broadcast_mask = pluss::make_series({true, false, true}, pluss::ScalarType::Bool);
  heading("masked_select_broadcast_mask");
  std::cout << x.masked_select(broadcast_mask) << "\n\n";

  auto empty_mask = pluss::make_series({
      {false, false, false},
      {false, false, false},
  }, pluss::ScalarType::Bool);
  heading("masked_select_empty");
  std::cout << x.masked_select(empty_mask) << "\n\n";

  heading("masked_select_bad_mask");
  try {
    std::cout << x.masked_select(pluss::make_series({1, 0, 1}, pluss::ScalarType::Int32)) << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  heading("masked_select_bad_shape");
  try {
    std::cout << x.masked_select(pluss::make_series({true, false}, pluss::ScalarType::Bool)) << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  auto where_series = pluss::make_series({10.0, 20.0, 30.0, 40.0});
  auto where_cond = pluss::make_series({false, true, false, true}, pluss::ScalarType::Bool);
  auto where_other = pluss::make_series({100.0, 200.0, 300.0, 400.0});
  heading("where_inplace_series");
  where_series.where_(where_cond, where_other);
  pluss::series::print(std::cout, where_series) << "\n\n";

  auto where_scalar = pluss::make_series({
      {1.0, 2.0, 3.0},
      {4.0, 5.0, 6.0},
  });
  auto where_col_mask = pluss::make_series({true, false, true}, pluss::ScalarType::Bool);
  heading("where_inplace_scalar");
  where_scalar.where_(where_col_mask, -1.0);
  std::cout << where_scalar << "\n\n";

  auto logical_a = pluss::make_series({
      {true, false},
      {true, false},
  }, pluss::ScalarType::Bool);
  auto logical_b = pluss::make_series({
      {true, true},
      {false, false},
  }, pluss::ScalarType::Bool);
  heading("logical_and_bool");
  std::cout << logical_a.logical_and(logical_b) << "\n\n";

  auto logical_cols = pluss::make_series({true, false}, pluss::ScalarType::Bool);
  heading("logical_or_broadcast");
  std::cout << logical_a.logical_or(logical_cols) << "\n\n";

  auto truthy_a = pluss::make_series({0, 2, 0, -5}, pluss::ScalarType::Int32);
  auto truthy_b = pluss::make_series({1, 0, 3, 0}, pluss::ScalarType::Int32);
  heading("logical_xor_numeric_truthiness");
  std::cout << truthy_a.logical_xor(truthy_b) << "\n\n";

  heading("logical_not_numeric");
  std::cout << truthy_a.logical_not() << "\n\n";

  auto logical_inplace_bad = pluss::make_series({0, 2, 0, -5}, pluss::ScalarType::Int32);
  heading("logical_not_inplace_numeric_error");
  try {
    logical_inplace_bad.logical_not_();
    std::cout << logical_inplace_bad << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  auto logical_inplace = pluss::make_series({true, false, true, false}, pluss::ScalarType::Bool);
  heading("logical_not_inplace_bool");
  logical_inplace.logical_not_();
  std::cout << logical_inplace << "\n\n";

  auto bits_a = pluss::make_series({1, 6, 12, 10}, pluss::ScalarType::Int32);
  auto bits_b = pluss::make_series({3, 10, 5, 12}, pluss::ScalarType::Int32);
  heading("bitwise_and_basic");
  std::cout << bits_a.bitwise_and(bits_b) << "\n\n";

  heading("bitwise_or_basic");
  std::cout << bits_a.bitwise_or(bits_b) << "\n\n";

  auto xor_left = pluss::make_series({1, 3, 7, 15}, pluss::ScalarType::UInt8);
  auto xor_right = pluss::make_series({1, 1, 2, 4}, pluss::ScalarType::UInt8);
  heading("bitwise_xor_basic");
  std::cout << xor_left.bitwise_xor(xor_right) << "\n\n";

  heading("bitwise_not_basic");
  std::cout << bits_a.bitwise_not() << "\n\n";

  heading("bitwise_float_error");
  try {
    std::cout << pluss::make_series({1.0, 2.0}).bitwise_and(pluss::make_series({1.0, 0.0})) << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  return 0;
}
