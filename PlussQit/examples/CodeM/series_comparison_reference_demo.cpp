#include <pluss/pluss.h>

#include <iostream>
#include <limits>
#include <stdexcept>
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
  const auto base = pluss::make_series({
      {1.0, 2.0, 3.0},
      {4.0, 5.0, 6.0},
  });
  const auto eq_row = pluss::make_series({1.0, 2.0, 0.0});
  const auto lt_row = pluss::make_series({2.0, 2.0, 5.0});
  const auto gt_row = pluss::make_series({0.0, 2.0, 5.0});
  const auto ge_row = pluss::make_series({1.0, 2.0, 5.0});
  const auto nan_lhs = pluss::make_series({1.0, std::numeric_limits<double>::quiet_NaN(), -1.0});
  const auto nan_rhs = pluss::make_series({1.0, std::numeric_limits<double>::quiet_NaN(), 0.0});

  heading("eq_scalar");
  pluss::series::print(std::cout, base.eq(3.0)) << "\n\n";

  heading("eq_series_broadcast");
  std::cout << base.eq(eq_row) << "\n\n";

  heading("eq_operator");
  std::cout << (base == eq_row) << "\n\n";

  heading("eq_type_promotion");
  auto ints = pluss::series::arange(1, 5, 1, pluss::ScalarType::Int32);
  auto floats = pluss::make_series({1.0, 2.0, 3.0, 4.5}).to(pluss::ScalarType::Float32);
  std::cout << ints.eq(floats) << "\n\n";

  heading("eq_nan");
  std::cout << nan_lhs.eq(nan_rhs) << "\n\n";

  heading("eq_broadcast_error");
  try {
    std::cout << base.eq(pluss::make_series({{1.0, 2.0}, {3.0, 4.0}})) << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  heading("ne_scalar");
  pluss::series::print(std::cout, base.ne(3.0)) << "\n\n";

  heading("ne_series_broadcast");
  std::cout << base.ne(eq_row) << "\n\n";

  heading("ne_operator");
  std::cout << (base != eq_row) << "\n\n";

  heading("ne_nan");
  std::cout << nan_lhs.ne(nan_rhs) << "\n\n";

  heading("ne_broadcast_error");
  try {
    std::cout << base.ne(pluss::make_series({{1.0, 2.0}, {3.0, 4.0}})) << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  heading("lt_scalar");
  pluss::series::print(std::cout, base.lt(4.0)) << "\n\n";

  heading("lt_series_broadcast");
  std::cout << base.lt(lt_row) << "\n\n";

  heading("lt_operator");
  std::cout << (base < lt_row) << "\n\n";

  heading("lt_equal_boundary");
  std::cout << pluss::make_series({1.0, 2.0, 3.0}).lt(2.0) << "\n\n";

  heading("lt_nan");
  std::cout << nan_lhs.lt(nan_rhs) << "\n\n";

  heading("lt_broadcast_error");
  try {
    std::cout << base.lt(pluss::make_series({{1.0, 2.0}, {3.0, 4.0}})) << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  heading("le_scalar");
  pluss::series::print(std::cout, base.le(4.0)) << "\n\n";

  heading("le_series_broadcast");
  std::cout << base.le(lt_row) << "\n\n";

  heading("le_operator");
  std::cout << (base <= lt_row) << "\n\n";

  heading("le_equal_boundary");
  std::cout << pluss::make_series({1.0, 2.0, 3.0}).le(2.0) << "\n\n";

  heading("le_nan");
  std::cout << nan_lhs.le(nan_rhs) << "\n\n";

  heading("le_broadcast_error");
  try {
    std::cout << base.le(pluss::make_series({{1.0, 2.0}, {3.0, 4.0}})) << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  heading("gt_scalar");
  pluss::series::print(std::cout, base.gt(3.0)) << "\n\n";

  heading("gt_series_broadcast");
  std::cout << base.gt(gt_row) << "\n\n";

  heading("gt_operator");
  std::cout << (base > gt_row) << "\n\n";

  heading("gt_equal_boundary");
  std::cout << pluss::make_series({1.0, 2.0, 3.0}).gt(2.0) << "\n\n";

  heading("gt_nan");
  std::cout << nan_lhs.gt(nan_rhs) << "\n\n";

  heading("gt_broadcast_error");
  try {
    std::cout << base.gt(pluss::make_series({{1.0, 2.0}, {3.0, 4.0}})) << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  heading("ge_scalar");
  pluss::series::print(std::cout, base.ge(3.0)) << "\n\n";

  heading("ge_series_broadcast");
  std::cout << base.ge(ge_row) << "\n\n";

  heading("ge_operator");
  std::cout << (base >= ge_row) << "\n\n";

  heading("ge_equal_boundary");
  std::cout << pluss::make_series({1.0, 2.0, 3.0}).ge(2.0) << "\n\n";

  heading("ge_nan");
  std::cout << nan_lhs.ge(nan_rhs) << "\n\n";

  heading("ge_broadcast_error");
  try {
    std::cout << base.ge(pluss::make_series({{1.0, 2.0}, {3.0, 4.0}})) << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  return 0;
}
