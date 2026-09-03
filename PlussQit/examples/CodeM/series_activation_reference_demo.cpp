#include <pluss/pluss.h>

#include <iostream>

namespace {

void heading(const char* label) {
  std::cout << "=== " << label << " ===\n";
}

}  // namespace

int main() {
  heading("reciprocal_basic");
  pluss::series::print(std::cout, pluss::make_series({2.0, -4.0, 0.5}).reciprocal()) << "\n\n";

  heading("reciprocal_gradient");
  auto reciprocal_x = pluss::make_series({2.0, 4.0, -0.5});
  reciprocal_x.tracks_slope_(true);
  auto reciprocal_y = reciprocal_x.reciprocal();
  reciprocal_y.sum().reverse();
  std::cout << "out=" << reciprocal_y << "\n";
  std::cout << "dL_dx=" << reciprocal_x.sensitivity() << "\n\n";

  heading("log1p_basic");
  std::cout << pluss::make_series({-0.5, 0.0, 3.0}).log1p() << "\n\n";

  heading("log1p_gradient");
  auto log1p_x = pluss::make_series({0.0, 1.0, 3.0});
  log1p_x.tracks_slope_(true);
  auto log1p_y = log1p_x.log1p();
  log1p_y.sum().reverse();
  std::cout << "out=" << log1p_y << "\n";
  std::cout << "dL_dx=" << log1p_x.sensitivity() << "\n\n";

  heading("expm1_basic");
  std::cout << pluss::series::print(pluss::make_series({-1.0, 0.0, 1.0}).expm1()) << "\n\n";

  heading("expm1_gradient");
  auto expm1_x = pluss::make_series({-1.0, 0.0, 1.0});
  expm1_x.tracks_slope_(true);
  auto expm1_y = expm1_x.expm1();
  expm1_y.sum().reverse();
  std::cout << "out=" << expm1_y << "\n";
  std::cout << "dL_dx=" << expm1_x.sensitivity() << "\n\n";

  heading("sigmoid_basic");
  std::cout << pluss::series::print(pluss::make_series({-2.0, 0.0, 2.0}).sigmoid()) << "\n\n";

  heading("sigmoid_gradient");
  auto sigmoid_x = pluss::make_series({-2.0, 0.0, 2.0});
  sigmoid_x.tracks_slope_(true);
  auto sigmoid_y = sigmoid_x.sigmoid();
  sigmoid_y.sum().reverse();
  std::cout << "out=" << sigmoid_y << "\n";
  std::cout << "dL_dx=" << sigmoid_x.sensitivity() << "\n\n";

  heading("relu_basic");
  std::cout << pluss::make_series({-2.0, 0.0, 3.0}).relu() << "\n\n";

  heading("relu_gradient");
  auto relu_x = pluss::make_series({-2.0, 0.0, 3.0});
  relu_x.tracks_slope_(true);
  auto relu_y = relu_x.relu();
  relu_y.sum().reverse();
  std::cout << "out=" << relu_y << "\n";
  std::cout << "dL_dx=" << relu_x.sensitivity() << "\n\n";

  return 0;
}
