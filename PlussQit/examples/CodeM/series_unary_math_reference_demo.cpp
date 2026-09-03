#include <pluss/pluss.h>

#include <iostream>
#include <limits>
#include <string>

namespace {

void heading(const char* label) {
  std::cout << "=== " << label << " ===\n";
}

} // namespace

int main() {
  const auto base = pluss::make_series({
      {-3.0, -2.0, -1.0},
      {0.0, 1.0, 4.0},
  });

  heading("abs_basic");
  pluss::series::print(std::cout, base.abs()) << "\n\n";

  heading("abs_integer");
  std::cout << pluss::make_series({-3, 0, 7}, pluss::ScalarType::Int32).abs() << "\n\n";

  heading("abs_noncontiguous");
  std::cout << base.transpose(0, 1).abs() << "\n\n";

  heading("abs_gradient");
  auto abs_x = pluss::make_series({-3.0, -2.0, 4.0});
  abs_x.tracks_slope_(true);
  auto abs_out = abs_x.abs();
  abs_out.sum().reverse();
  std::cout << "out=" << abs_out << "\n";
  std::cout << "dx=" << abs_x.sensitivity() << "\n\n";

  heading("neg_basic");
  pluss::series::print(std::cout, base.neg()) << "\n\n";

  heading("neg_operator");
  std::cout << (-base) << "\n\n";

  heading("neg_gradient");
  auto neg_x = pluss::make_series({-3.0, 0.5, 4.0});
  neg_x.tracks_slope_(true);
  auto neg_out = neg_x.neg();
  neg_out.sum().reverse();
  std::cout << "out=" << neg_out << "\n";
  std::cout << "dx=" << neg_x.sensitivity() << "\n\n";

  heading("sign_basic");
  pluss::series::print(std::cout, base.sign()) << "\n\n";

  heading("sign_integer");
  std::cout << pluss::make_series({-9, 0, 11}, pluss::ScalarType::Int64).sign() << "\n\n";

  heading("sign_nan");
  std::cout << pluss::make_series({-1.0, 0.0, 1.0, std::numeric_limits<double>::quiet_NaN()}).sign() << "\n\n";

  heading("exp_basic");
  pluss::series::print(std::cout, pluss::make_series({-1.0, 0.0, 1.0}).exp()) << "\n\n";

  heading("exp_matrix");
  std::cout << pluss::make_series({
      {0.0, 1.0},
      {2.0, 3.0},
  }).exp() << "\n\n";

  heading("exp_gradient");
  auto exp_x = pluss::make_series({-1.0, 0.0, 1.0});
  exp_x.tracks_slope_(true);
  auto exp_out = exp_x.exp();
  exp_out.sum().reverse();
  std::cout << "out=" << exp_out << "\n";
  std::cout << "dx=" << exp_x.sensitivity() << "\n\n";

  heading("exp_overflow");
  std::cout << pluss::make_series({700.0, 710.0}).exp() << "\n\n";

  heading("log_basic");
  pluss::series::print(std::cout, pluss::make_series({1.0, 2.718281828459045, 7.38905609893065}).log())
      << "\n\n";

  heading("log_fraction");
  std::cout << pluss::make_series({0.25, 0.5, 1.0, 4.0}).log() << "\n\n";

  heading("log_gradient");
  auto log_x = pluss::make_series({1.0, 2.0, 4.0});
  log_x.tracks_slope_(true);
  auto log_out = log_x.log();
  log_out.sum().reverse();
  std::cout << "out=" << log_out << "\n";
  std::cout << "dx=" << log_x.sensitivity() << "\n\n";

  heading("log_domain_edge");
  std::cout << pluss::make_series({1.0, 0.0, -1.0}).log() << "\n\n";

  heading("sqrt_basic");
  pluss::series::print(std::cout, pluss::make_series({0.0, 1.0, 4.0, 9.0}).sqrt()) << "\n\n";

  heading("sqrt_matrix");
  std::cout << pluss::make_series({
      {1.0, 4.0},
      {9.0, 16.0},
  }).sqrt() << "\n\n";

  heading("sqrt_gradient");
  auto sqrt_x = pluss::make_series({1.0, 4.0, 9.0});
  sqrt_x.tracks_slope_(true);
  auto sqrt_out = sqrt_x.sqrt();
  sqrt_out.sum().reverse();
  std::cout << "out=" << sqrt_out << "\n";
  std::cout << "dx=" << sqrt_x.sensitivity() << "\n\n";

  heading("sqrt_domain_edge");
  std::cout << pluss::make_series({4.0, 0.0, -1.0}).sqrt() << "\n\n";

  heading("asinh_values_and_integer_promotion");
  std::cout << "anchors=" << pluss::make_series({-1.1752011936438014, 0.0, 1.1752011936438014}).asinh() << "\n";
  std::cout << "integer=" << pluss::make_series({-1, 0, 1}, pluss::ScalarType::Int64).asinh() << "\n\n";

  heading("asinh_gradient");
  auto asinh_x = pluss::make_series({0.0, 0.75});
  asinh_x.tracks_slope_(true);
  asinh_x.asinh().sum().reverse();
  std::cout << asinh_x.sensitivity() << "\n\n";

  heading("acosh_values_domain_and_gradient");
  std::cout << "anchors=" << pluss::make_series({1.0, 1.5430806348152437, 3.7621956910836314}).acosh() << "\n";
  std::cout << "domain=" << pluss::make_series({0.5, 1.0, 2.0}).acosh() << "\n";
  auto acosh_x = pluss::make_series({1.25, 2.0});
  acosh_x.tracks_slope_(true);
  acosh_x.acosh().sum().reverse();
  std::cout << "gradient=" << acosh_x.sensitivity() << "\n\n";

  heading("atanh_values_domain_and_gradient");
  std::cout << "anchors=" << pluss::make_series({-0.7615941559557649, 0.0, 0.7615941559557649}).atanh() << "\n";
  std::cout << "domain=" << pluss::make_series({-1.25, -1.0, 0.0, 1.0, 1.25}).atanh() << "\n";
  auto atanh_x = pluss::make_series({-0.5, 0.0, 0.5});
  atanh_x.tracks_slope_(true);
  atanh_x.atanh().sum().reverse();
  std::cout << "gradient=" << atanh_x.sensitivity() << "\n\n";

  return 0;
}
