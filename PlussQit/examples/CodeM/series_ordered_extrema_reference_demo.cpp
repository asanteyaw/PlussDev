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
  auto lows = pluss::make_series({
      {4.0, 1.0, 3.0},
      {2.0, 1.0, 6.0},
  });

  heading("min_all");
  std::cout << lows.min() << "\n\n";

  heading("min_dim1");
  std::cout << lows.min(1) << "\n\n";

  heading("min_dim0");
  std::cout << lows.min(0) << "\n\n";

  heading("min_dim1_keepdim");
  std::cout << lows.min(1, true) << "\n\n";

  heading("min_negative_dim");
  std::cout << lows.min(-1) << "\n\n";

  auto ceilings = pluss::make_series({3.0, 2.0, 4.0});
  heading("min_other_series");
  std::cout << lows.min(ceilings) << "\n\n";

  heading("minimum_same_job");
  std::cout << lows.minimum(ceilings) << "\n\n";

  auto reduced_ties = pluss::make_series({
      {2.0, 1.0, 1.0},
      {3.0, -4.0, -4.0},
  });
  reduced_ties.tracks_slope_(true);
  auto reduced_winners = reduced_ties.min(1);
  reduced_winners.sum().reverse();
  heading("min_reduce_tie_gradient");
  std::cout << "y=" << reduced_winners << "\n";
  std::cout << "dx=" << reduced_ties.sensitivity() << "\n\n";

  auto global_ties = pluss::make_series({
      {-5.0, 0.0, 0.0},
      {-5.0, -5.0, 1.0},
  });
  global_ties.tracks_slope_(true);
  auto global_winner = global_ties.min();
  global_winner.reverse();
  heading("min_global_tie_gradient");
  std::cout << "y=" << global_winner << "\n";
  std::cout << "dx=" << global_ties.sensitivity() << "\n\n";

  auto lhs = pluss::make_series({1.0, 5.0, 2.0});
  auto rhs = pluss::make_series({1.0, 2.0, 2.0});
  lhs.tracks_slope_(true);
  rhs.tracks_slope_(true);
  auto binary = lhs.min(rhs);
  binary.sum().reverse();
  heading("min_binary_tie_gradient");
  std::cout << "y=" << binary << "\n";
  std::cout << "dlhs=" << lhs.sensitivity() << "\n";
  std::cout << "drhs=" << rhs.sensitivity() << "\n\n";

  heading("min_bad_dim_error");
  try {
    std::cout << lows.min(2) << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  heading("min_pair_rows");
  const auto [row_values, row_indices] = lows.min_pair(1);
  std::cout << "values=" << row_values << "\n";
  std::cout << "indices=" << row_indices << "\n\n";

  auto min_pair_ties = pluss::make_series({
      {1.0, 1.0, 5.0},
      {-4.0, -4.0, 2.0},
  });
  min_pair_ties.tracks_slope_(true);
  const auto tied_min_pair = min_pair_ties.min_pair(-1, true);
  tied_min_pair.first.sum().reverse();
  heading("min_pair_keepdim_first_tie_and_gradient");
  std::cout << "values=" << tied_min_pair.first << "\n";
  std::cout << "indices=" << tied_min_pair.second << "\n";
  std::cout << "dx=" << min_pair_ties.sensitivity() << "\n\n";

  heading("min_pair_scalar");
  const auto min_scalar_pair = pluss::make_series(42.0).min_pair(-1);
  std::cout << "value=" << min_scalar_pair.first << "\n";
  std::cout << "index=" << min_scalar_pair.second << "\n\n";

  heading("min_pair_empty_axis_error");
  try {
    std::cout << pluss::series::zeros({2, 0}).min_pair(1).first << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  auto highs = pluss::make_series({
      {1.0, 7.0, 3.0},
      {9.0, 7.0, 6.0},
  });

  heading("argmax_all");
  std::cout << highs.argmax() << "\n\n";

  heading("argmax_dim1");
  std::cout << highs.argmax(1) << "\n\n";

  heading("argmax_dim0");
  std::cout << highs.argmax(0) << "\n\n";

  heading("argmax_dim1_keepdim");
  std::cout << highs.argmax(1, true) << "\n\n";

  heading("argmax_negative_dim");
  std::cout << highs.argmax(-1) << "\n\n";

  auto argmax_ties = pluss::make_series({
      {4.0, 9.0, 9.0},
      {2.0, 2.0, 1.0},
  });
  heading("argmax_first_tie");
  std::cout << argmax_ties.argmax(1) << "\n\n";

  auto argmax_global_ties = pluss::make_series({
      {9.0, 9.0},
      {8.0, 9.0},
  });
  heading("argmax_global_first_tie");
  std::cout << argmax_global_ties.argmax() << "\n\n";

  auto scalar = pluss::make_series(42.0);
  heading("argmax_scalar");
  std::cout << scalar.argmax() << "\n\n";

  heading("argmax_bad_dim_error");
  try {
    std::cout << highs.argmax(2) << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  auto empty_rows = pluss::series::zeros({0, 3});
  heading("argmax_empty_error");
  try {
    std::cout << empty_rows.argmax(0) << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  auto cold = pluss::make_series({
      {1.0, -7.0, 3.0},
      {-9.0, -7.0, 6.0},
  });

  heading("argmin_all");
  std::cout << cold.argmin() << "\n\n";

  heading("argmin_dim1");
  std::cout << cold.argmin(1) << "\n\n";

  heading("argmin_dim0");
  std::cout << cold.argmin(0) << "\n\n";

  heading("argmin_dim1_keepdim");
  std::cout << cold.argmin(1, true) << "\n\n";

  heading("argmin_negative_dim");
  std::cout << cold.argmin(-1) << "\n\n";

  auto argmin_ties = pluss::make_series({
      {-4.0, -9.0, -9.0},
      {-2.0, -2.0, 1.0},
  });
  heading("argmin_first_tie");
  std::cout << argmin_ties.argmin(1) << "\n\n";

  auto argmin_global_ties = pluss::make_series({
      {-9.0, -9.0},
      {0.0, -9.0},
  });
  heading("argmin_global_first_tie");
  std::cout << argmin_global_ties.argmin() << "\n\n";

  heading("argmin_scalar");
  std::cout << scalar.argmin() << "\n\n";

  heading("argmin_bad_dim_error");
  try {
    std::cout << cold.argmin(2) << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  auto empty_cols = pluss::series::zeros({3, 0});
  heading("argmin_empty_error");
  try {
    std::cout << empty_cols.argmin(1) << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  return 0;
}
