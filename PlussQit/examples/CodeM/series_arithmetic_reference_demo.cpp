#include <pluss/pluss.h>

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

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
  auto base = pluss::make_series({
      {1.0, 2.0, 3.0},
      {4.0, 5.0, 6.0},
  });
  auto row = pluss::make_series({10.0, 20.0, 30.0});

  heading("add_scalar");
  pluss::series::print(std::cout, base.add(2.0)) << "\n\n";

  heading("add_series_scalar_alpha");
  std::cout << pluss::series::print(base.add(pluss::make_series(10.0), 0.5)) << "\n\n";

  heading("add_series_broadcast");
  std::cout << base.add(row) << "\n\n";

  heading("add_series_alpha");
  std::cout << base.add(row, 0.25) << "\n\n";

  heading("add_type_promotion");
  auto ints = pluss::series::arange(0, 4, 1, pluss::ScalarType::Int32);
  auto floats = pluss::series::full({4}, 1.5, pluss::ScalarType::Float32);
  std::cout << ints.add(floats) << "\n\n";

  heading("add_gradient");
  auto add_x = pluss::make_series({1.0, 2.0, 3.0});
  auto add_y = pluss::make_series({10.0, 20.0, 30.0});
  add_x.tracks_slope_(true);
  add_y.tracks_slope_(true);
  auto add_out = add_x.add(add_y, 0.5);
  add_out.sum().reverse();
  std::cout << "out=" << add_out << "\n";
  std::cout << "dx=" << add_x.sensitivity() << "\n";
  std::cout << "dy=" << add_y.sensitivity() << "\n\n";

  heading("add_broadcast_error");
  try {
    std::cout << base.add(pluss::make_series({{1.0, 2.0}, {3.0, 4.0}})) << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  heading("sub_scalar");
  pluss::series::print(std::cout, base.sub(2.0)) << "\n\n";

  heading("sub_series_scalar_alpha");
  std::cout << pluss::series::print(base.sub(pluss::make_series(10.0), 0.5)) << "\n\n";

  heading("sub_series_broadcast");
  std::cout << base.sub(row) << "\n\n";

  heading("sub_series_alpha");
  std::cout << base.sub(row, 0.25) << "\n\n";

  heading("sub_gradient");
  auto sub_x = pluss::make_series({1.0, 2.0, 3.0});
  auto sub_y = pluss::make_series({10.0, 20.0, 30.0});
  sub_x.tracks_slope_(true);
  sub_y.tracks_slope_(true);
  auto sub_out = sub_x.sub(sub_y, 0.5);
  sub_out.sum().reverse();
  std::cout << "out=" << sub_out << "\n";
  std::cout << "dx=" << sub_x.sensitivity() << "\n";
  std::cout << "dy=" << sub_y.sensitivity() << "\n\n";

  heading("sub_broadcast_error");
  try {
    std::cout << base.sub(pluss::make_series({{1.0, 2.0}, {3.0, 4.0}})) << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  heading("mul_scalar");
  std::cout << base.mul(2.0) << "\n\n";

  heading("mul_series_broadcast");
  std::cout << base.mul(row) << "\n\n";

  heading("mul_noncontiguous");
  auto transposed = base.transpose(0, 1);
  pluss::series::print(std::cout, transposed.mul(pluss::make_series({10.0, 100.0}))) << "\n\n";

  heading("mul_gradient");
  auto mul_x = pluss::make_series({1.0, 2.0, 3.0});
  auto mul_y = pluss::make_series({10.0, 20.0, 30.0});
  mul_x.tracks_slope_(true);
  mul_y.tracks_slope_(true);
  auto mul_out = mul_x.mul(mul_y);
  mul_out.sum().reverse();
  std::cout << "out=" << mul_out << "\n";
  std::cout << "dx=" << mul_x.sensitivity() << "\n";
  std::cout << "dy=" << mul_y.sensitivity() << "\n\n";

  heading("mul_type_promotion");
  auto mul_ints = pluss::series::arange(1, 5, 1, pluss::ScalarType::Int32);
  auto mul_floats = pluss::series::full({4}, 0.5, pluss::ScalarType::Float32);
  std::cout << mul_ints.mul(mul_floats) << "\n\n";

  heading("mul_broadcast_error");
  try {
    std::cout << base.mul(pluss::make_series({{1.0, 2.0}, {3.0, 4.0}})) << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  heading("div_scalar");
  std::cout << base.div(2.0) << "\n\n";

  heading("div_series_broadcast");
  std::cout << base.div(pluss::make_series({1.0, 2.0, 4.0})) << "\n\n";

  heading("div_integer_scalar_promotes");
  auto div_ints = pluss::series::arange(5, 8, 1, pluss::ScalarType::Int64);
  std::cout << div_ints.div(2) << "\n\n";

  heading("div_integer_series_promotes");
  std::cout << div_ints.div(pluss::series::full({3}, 2, pluss::ScalarType::Int64)) << "\n\n";

  heading("div_float_zero");
  std::cout << pluss::make_series({1.0, -1.0, 0.0}).div(0.0) << "\n\n";

  heading("div_gradient");
  auto div_x = pluss::make_series({2.0, 4.0, 8.0});
  auto div_y = pluss::make_series({1.0, 2.0, 4.0});
  div_x.tracks_slope_(true);
  div_y.tracks_slope_(true);
  auto div_out = div_x.div(div_y);
  div_out.sum().reverse();
  std::cout << "out=" << div_out << "\n";
  std::cout << "dx=" << div_x.sensitivity() << "\n";
  std::cout << "dy=" << div_y.sensitivity() << "\n\n";

  heading("div_broadcast_error");
  try {
    std::cout << base.div(pluss::make_series({{1.0, 2.0}, {3.0, 4.0}})) << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  heading("pow_scalar");
  pluss::series::print(std::cout, base.pow(2.0)) << "\n\n";

  heading("pow_series_broadcast");
  auto pow_base = pluss::make_series({
      {1.0, 4.0, 9.0},
      {16.0, 25.0, 36.0},
  });
  std::cout << pow_base.pow(pluss::make_series({0.5, 1.0, 2.0})) << "\n\n";

  heading("pow_fractional_scalar");
  std::cout << pluss::make_series({1.0, 4.0, 9.0}).pow(0.5) << "\n\n";

  heading("pow_negative_fractional");
  std::cout << pluss::make_series({-1.0, 4.0}).pow(0.5) << "\n\n";

  heading("pow_gradient");
  auto pow_x = pluss::make_series({1.0, 2.0, 3.0});
  pow_x.tracks_slope_(true);
  auto pow_out = pow_x.pow(2.0);
  pow_out.sum().reverse();
  std::cout << "out=" << pow_out << "\n";
  std::cout << "dx=" << pow_x.sensitivity() << "\n\n";

  heading("pow_broadcast_error");
  try {
    std::cout << base.pow(pluss::make_series({{1.0, 2.0}, {3.0, 4.0}})) << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  return 0;
}
