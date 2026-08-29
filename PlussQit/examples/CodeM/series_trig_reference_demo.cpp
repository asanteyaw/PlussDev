#include <pluss/pluss.h>

#include <iostream>

namespace {

constexpr double kPi = 3.14159265358979323846;

void heading(const char* label) {
  std::cout << "=== " << label << " ===\n";
}

} // namespace

int main() {
  heading("sin_basic");
  pluss::series::print(
      std::cout,
      pluss::make_series({-kPi / 2.0, 0.0, kPi / 2.0, kPi}).sin())
      << "\n\n";

  heading("sin_matrix");
  std::cout << pluss::make_series({
      {0.0, kPi / 2.0},
      {kPi, 3.0 * kPi / 2.0},
  }).sin() << "\n\n";

  heading("sin_gradient");
  auto sin_x = pluss::make_series({0.0, kPi / 2.0, kPi});
  sin_x.tracks_slope_(true);
  auto sin_out = sin_x.sin();
  sin_out.sum().reverse();
  std::cout << "out=" << sin_out << "\n";
  std::cout << "dx=" << sin_x.sensitivity() << "\n\n";

  heading("sin_noncontiguous");
  auto sin_view = pluss::make_series({
      {0.0, kPi / 6.0, kPi / 3.0},
      {kPi / 2.0, 2.0 * kPi / 3.0, 5.0 * kPi / 6.0},
  }).transpose(0, 1);
  std::cout << sin_view.sin() << "\n\n";

  heading("cos_basic");
  std::cout << pluss::series::print(
      pluss::make_series({0.0, kPi / 2.0, kPi, 2.0 * kPi}).cos()) << "\n\n";

  heading("cos_matrix");
  pluss::series::print(
      std::cout,
      pluss::make_series({
          {0.0, kPi / 2.0},
          {kPi, 2.0 * kPi},
      }).cos())
      << "\n\n";

  heading("cos_gradient");
  auto cos_x = pluss::make_series({0.0, kPi / 6.0, kPi / 3.0});
  cos_x.tracks_slope_(true);
  auto cos_out = cos_x.cos();
  cos_out.sum().reverse();
  std::cout << "out=" << cos_out << "\n";
  std::cout << "dx=" << cos_x.sensitivity() << "\n\n";

  heading("cos_evenness");
  std::cout << pluss::make_series({-kPi / 3.0, -kPi / 6.0, 0.0, kPi / 6.0, kPi / 3.0}).cos()
            << "\n\n";

  heading("tan_basic");
  pluss::series::print(
      std::cout,
      pluss::make_series({-kPi / 4.0, 0.0, kPi / 4.0}).tan())
      << "\n\n";

  heading("tan_matrix");
  std::cout << pluss::make_series({
      {-0.5, 0.0},
      {0.5, 1.0},
  }).tan() << "\n\n";

  heading("tan_gradient");
  auto tan_x = pluss::make_series({-0.5, 0.0, 0.5});
  tan_x.tracks_slope_(true);
  auto tan_out = tan_x.tan();
  tan_out.sum().reverse();
  std::cout << "out=" << tan_out << "\n";
  std::cout << "dx=" << tan_x.sensitivity() << "\n\n";

  heading("tan_steep");
  std::cout << pluss::make_series({1.2, 1.4, 1.5}).tan() << "\n\n";

  heading("asin_basic");
  std::cout << pluss::series::print(
      pluss::make_series({-1.0, -0.5, 0.0, 0.5, 1.0}).asin()) << "\n\n";

  heading("asin_matrix");
  pluss::series::print(
      std::cout,
      pluss::make_series({
          {-1.0, 0.0},
          {0.5, 1.0},
      }).asin())
      << "\n\n";

  heading("asin_gradient");
  auto asin_x = pluss::make_series({-0.5, 0.0, 0.5});
  asin_x.tracks_slope_(true);
  auto asin_out = asin_x.asin();
  asin_out.sum().reverse();
  std::cout << "out=" << asin_out << "\n";
  std::cout << "dx=" << asin_x.sensitivity() << "\n\n";

  heading("asin_domain");
  std::cout << pluss::make_series({-1.25, -1.0, 0.0, 1.0, 1.25}).asin() << "\n\n";

  heading("acos_basic");
  pluss::series::print(
      std::cout,
      pluss::make_series({-1.0, 0.0, 0.5, 1.0}).acos())
      << "\n\n";

  heading("acos_matrix");
  std::cout << pluss::make_series({
      {-1.0, -0.5},
      {0.5, 1.0},
  }).acos() << "\n\n";

  heading("acos_gradient");
  auto acos_x = pluss::make_series({-0.5, 0.0, 0.5});
  acos_x.tracks_slope_(true);
  auto acos_out = acos_x.acos();
  acos_out.sum().reverse();
  std::cout << "out=" << acos_out << "\n";
  std::cout << "dx=" << acos_x.sensitivity() << "\n\n";

  heading("acos_domain");
  std::cout << pluss::make_series({-1.25, -1.0, 0.0, 1.0, 1.25}).acos() << "\n\n";

  heading("atan_basic");
  pluss::series::print(
      std::cout,
      pluss::make_series({-1.0, 0.0, 1.0}).atan())
      << "\n\n";

  heading("atan_matrix");
  std::cout << pluss::make_series({
      {-2.0, -0.5},
      {0.5, 2.0},
  }).atan() << "\n\n";

  heading("atan_gradient");
  auto atan_x = pluss::make_series({-1.0, 0.0, 1.0});
  atan_x.tracks_slope_(true);
  auto atan_out = atan_x.atan();
  atan_out.sum().reverse();
  std::cout << "out=" << atan_out << "\n";
  std::cout << "dx=" << atan_x.sensitivity() << "\n\n";

  heading("atan_saturation");
  std::cout << pluss::make_series({-100.0, -10.0, 10.0, 100.0}).atan() << "\n\n";

  heading("sinh_basic");
  std::cout << pluss::series::print(
      pluss::make_series({-1.0, 0.0, 1.0}).sinh()) << "\n\n";

  heading("sinh_matrix");
  pluss::series::print(
      std::cout,
      pluss::make_series({
          {-1.0, -0.5},
          {0.5, 1.0},
      }).sinh())
      << "\n\n";

  heading("sinh_gradient");
  auto sinh_x = pluss::make_series({-1.0, 0.0, 1.0});
  sinh_x.tracks_slope_(true);
  auto sinh_out = sinh_x.sinh();
  sinh_out.sum().reverse();
  std::cout << "out=" << sinh_out << "\n";
  std::cout << "dx=" << sinh_x.sensitivity() << "\n\n";

  heading("sinh_oddness");
  std::cout << pluss::make_series({-2.0, -1.0, 0.0, 1.0, 2.0}).sinh() << "\n\n";

  heading("cosh_basic");
  pluss::series::print(
      std::cout,
      pluss::make_series({-1.0, 0.0, 1.0}).cosh())
      << "\n\n";

  heading("cosh_matrix");
  std::cout << pluss::make_series({
      {-1.0, -0.5},
      {0.5, 1.0},
  }).cosh() << "\n\n";

  heading("cosh_gradient");
  auto cosh_x = pluss::make_series({-1.0, 0.0, 1.0});
  cosh_x.tracks_slope_(true);
  auto cosh_out = cosh_x.cosh();
  cosh_out.sum().reverse();
  std::cout << "out=" << cosh_out << "\n";
  std::cout << "dx=" << cosh_x.sensitivity() << "\n\n";

  heading("cosh_nonnegative");
  std::cout << pluss::make_series({-3.0, -1.0, 0.0, 1.0, 3.0}).cosh() << "\n\n";

  heading("tanh_basic");
  std::cout << pluss::series::print(
      pluss::make_series({-2.0, 0.0, 2.0}).tanh()) << "\n\n";

  heading("tanh_matrix");
  pluss::series::print(
      std::cout,
      pluss::make_series({
          {-2.0, -0.5},
          {0.5, 2.0},
      }).tanh())
      << "\n\n";

  heading("tanh_gradient");
  auto tanh_x = pluss::make_series({-2.0, 0.0, 2.0});
  tanh_x.tracks_slope_(true);
  auto tanh_out = tanh_x.tanh();
  tanh_out.sum().reverse();
  std::cout << "out=" << tanh_out << "\n";
  std::cout << "dx=" << tanh_x.sensitivity() << "\n\n";

  heading("tanh_saturation");
  std::cout << pluss::make_series({-8.0, -3.0, 3.0, 8.0}).tanh() << "\n";

  return 0;
}
