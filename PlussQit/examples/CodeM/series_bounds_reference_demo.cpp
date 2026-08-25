#include <pluss/pluss.h>

#include <iostream>
#include <string>
#include <vector>

namespace {

void heading(const char* label) {
  std::cout << "=== " << label << " ===\n";
}

}  // namespace

int main() {
  auto grid = pluss::make_series(
      std::vector<double>{1, 5, 3, 4, 2, 8},
      {2, 3});
  auto peer = pluss::make_series(std::vector<double>{2, 6, 1}, {3});

  heading("maximum_series_broadcast");
  std::cout << grid.maximum(peer) << "\n\n";

  heading("maximum_scalar");
  std::cout << grid.maximum(4.5) << "\n\n";

  auto column_peer = pluss::make_series(std::vector<double>{2, 6}, {2, 1});
  heading("maximum_column_broadcast");
  std::cout << grid.maximum(column_peer) << "\n\n";

  auto int_grid = pluss::make_series(std::vector<int>{1, 5, 3, 4, 2, 8}, {2, 3});
  heading("maximum_scalar_int_input");
  std::cout << int_grid.maximum(4.5) << "\n\n";

  auto max_a = pluss::make_series(std::vector<double>{1, 5, 2}, {3});
  auto max_b = pluss::make_series(std::vector<double>{4, 2, 2}, {3});
  max_a.tracks_slope_(true);
  max_b.tracks_slope_(true);
  auto max_y = max_a.maximum(max_b);
  max_y.sum().reverse();
  heading("maximum_gradient_tie");
  std::cout << "y=" << max_y << "\n";
  std::cout << "da=" << max_a.sensitivity() << "\n";
  std::cout << "db=" << max_b.sensitivity() << "\n\n";

  heading("minimum_series_broadcast");
  std::cout << grid.minimum(peer) << "\n\n";

  heading("minimum_scalar");
  std::cout << grid.minimum(4.5) << "\n\n";

  heading("minimum_column_broadcast");
  std::cout << grid.minimum(column_peer) << "\n\n";

  heading("minimum_scalar_int_input");
  std::cout << int_grid.minimum(4.5) << "\n\n";

  auto min_a = pluss::make_series(std::vector<double>{1, 5, 2}, {3});
  auto min_b = pluss::make_series(std::vector<double>{4, 2, 2}, {3});
  min_a.tracks_slope_(true);
  min_b.tracks_slope_(true);
  auto min_y = min_a.minimum(min_b);
  min_y.sum().reverse();
  heading("minimum_gradient_tie");
  std::cout << "y=" << min_y << "\n";
  std::cout << "da=" << min_a.sensitivity() << "\n";
  std::cout << "db=" << min_b.sensitivity() << "\n\n";

  auto line = pluss::make_series(std::vector<double>{-2.0, -0.5, 0.0, 1.0, 3.0}, {5});
  heading("clamp_min_scalar");
  std::cout << line.clamp_min(0.0) << "\n\n";

  auto clamp_min_grad = line.clone();
  clamp_min_grad.tracks_slope_(true);
  auto clamp_min_y = clamp_min_grad.clamp_min(0.0);
  clamp_min_y.sum().reverse();
  heading("clamp_min_scalar_gradient");
  std::cout << "y=" << clamp_min_y << "\n";
  std::cout << "dx=" << clamp_min_grad.sensitivity() << "\n\n";

  auto lows = pluss::make_series(std::vector<double>{2, 4, 3}, {3});
  auto min_grid = pluss::make_series(
      std::vector<double>{1, 5, 3, 0, 7, 2},
      {2, 3});
  heading("clamp_min_series_broadcast");
  std::cout << min_grid.clamp_min(lows) << "\n\n";

  auto clamp_min_scalar_inplace = line.clone();
  heading("clamp_min_inplace_scalar");
  clamp_min_scalar_inplace.clamp_min_(1.5);
  std::cout << clamp_min_scalar_inplace << "\n\n";

  auto clamp_min_series_inplace = min_grid.clone();
  heading("clamp_min_inplace_series");
  clamp_min_series_inplace.clamp_min_(lows);
  std::cout << clamp_min_series_inplace << "\n\n";

  heading("clamp_max_scalar");
  std::cout << line.clamp_max(0.0) << "\n\n";

  auto clamp_max_grad = line.clone();
  clamp_max_grad.tracks_slope_(true);
  auto clamp_max_y = clamp_max_grad.clamp_max(0.0);
  clamp_max_y.sum().reverse();
  heading("clamp_max_scalar_gradient");
  std::cout << "y=" << clamp_max_y << "\n";
  std::cout << "dx=" << clamp_max_grad.sensitivity() << "\n\n";

  auto highs = pluss::make_series(std::vector<double>{2, 4, 3}, {3});
  auto max_grid = pluss::make_series(
      std::vector<double>{1, 5, 3, 0, 7, 2},
      {2, 3});
  heading("clamp_max_series_broadcast");
  std::cout << max_grid.clamp_max(highs) << "\n\n";

  auto clamp_max_scalar_inplace = line.clone();
  heading("clamp_max_inplace_scalar");
  clamp_max_scalar_inplace.clamp_max_(1.5);
  std::cout << clamp_max_scalar_inplace << "\n\n";

  auto clamp_max_series_inplace = max_grid.clone();
  heading("clamp_max_inplace_series");
  clamp_max_series_inplace.clamp_max_(highs);
  std::cout << clamp_max_series_inplace << "\n\n";

  return 0;
}
