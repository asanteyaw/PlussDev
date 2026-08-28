#include <pluss/pluss.h>

#include <iostream>
#include <limits>
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

} // namespace

int main() {
  const auto grid = pluss::make_series({
      {10.0, 20.0, 30.0},
      {40.0, 50.0, 60.0},
      {70.0, 80.0, 90.0},
  });

  heading("select_rows_dim0");
  pluss::series::print(std::cout, grid.select(0, 1)) << "\n\n";

  heading("select_cols_negative_dim");
  std::cout << grid.select(-1, 2) << "\n\n";

  heading("select_negative_index");
  std::cout << grid.select(1, -1) << "\n\n";

  heading("select_gradient");
  auto tracked = pluss::make_series({
      {1.0, 2.0, 3.0},
      {4.0, 5.0, 6.0},
      {7.0, 8.0, 9.0},
  });
  tracked.tracks_slope_(true);
  auto selected = tracked.select(0, 1);
  selected.sum().reverse();
  std::cout << "out=" << selected << "\n";
  std::cout << "dx=" << tracked.sensitivity() << "\n\n";

  heading("select_scalar_error");
  try {
    std::cout << pluss::make_series(42.0).select(0, 0) << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  heading("select_index_error");
  try {
    std::cout << grid.select(0, 5) << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  const auto mask2d = pluss::make_series({
      {0.0, 2.0, 0.0},
      {3.0, 0.0, 4.0},
  });

  heading("nonzero_matrix");
  pluss::series::print(std::cout, mask2d.nonzero()) << "\n\n";

  heading("nonzero_vector");
  std::cout << pluss::make_series({0.0, -2.0, 0.0, 5.0}).nonzero() << "\n\n";

  heading("nonzero_scalar_present");
  std::cout << pluss::make_series(5.0).nonzero() << "\n\n";

  heading("nonzero_scalar_absent");
  std::cout << pluss::make_series(0.0).nonzero() << "\n\n";

  heading("nonzero_empty");
  std::cout << pluss::series::zeros({0, 3}).nonzero() << "\n\n";

  const auto duplicates = pluss::make_series({4, 1, 4, 2, 1, 3}, pluss::ScalarType::Int64);

  heading("unique_sorted_default");
  pluss::series::print(std::cout, duplicates.unique()) << "\n\n";

  heading("unique_preserve_first_seen");
  std::cout << duplicates.unique(false) << "\n\n";

  heading("unique_flattens_higher_rank");
  std::cout << pluss::make_series({
      {3, 3, 2},
      {1, 2, 1},
  }, pluss::ScalarType::Int64).unique(false) << "\n\n";

  heading("unique_float_nan");
  std::cout << pluss::make_series({
      1.0,
      std::numeric_limits<double>::quiet_NaN(),
      std::numeric_limits<double>::quiet_NaN(),
      -0.0,
      0.0,
      1.0,
  }).unique(false) << "\n\n";

  const auto members = pluss::make_series({10, 30, 10, 50}, pluss::ScalarType::Int64);
  const auto universe = pluss::make_series({30, 40, 50}, pluss::ScalarType::Int64);

  heading("isin_basic");
  pluss::series::print(std::cout, members.isin(universe)) << "\n\n";

  heading("isin_invert");
  std::cout << members.isin(universe, false, true) << "\n\n";

  heading("isin_preserves_shape");
  auto matrix_members = pluss::make_series({
      {1, 2, 5},
      {7, 2, 9},
  }, pluss::ScalarType::Int64);
  std::cout << matrix_members.isin(
      pluss::make_series({2, 9}, pluss::ScalarType::Int64)) << "\n\n";

  heading("isin_dtype_error");
  try {
    auto ints = pluss::make_series({1, 2, 3}, pluss::ScalarType::Int64);
    auto floats = pluss::make_series({1.0, 3.0});
    std::cout << ints.isin(floats) << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  const auto counts_in = pluss::make_series({0, 2, 2, 4, 2, 1}, pluss::ScalarType::Int64);

  heading("bincount_basic");
  pluss::series::print(std::cout, counts_in.bincount()) << "\n\n";

  heading("bincount_minlength");
  std::cout << counts_in.bincount(7) << "\n\n";

  heading("bincount_empty_minlength");
  std::cout << pluss::series::zeros({0}, pluss::ScalarType::Int64).bincount(4) << "\n\n";

  heading("bincount_negative_error");
  try {
    auto bad = pluss::make_series({0, 1, -1}, pluss::ScalarType::Int64);
    std::cout << bad.bincount() << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  heading("bincount_dtype_error");
  try {
    std::cout << pluss::make_series({0.0, 1.0, 2.0}).bincount() << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  const auto hist_values = pluss::make_series({0.0, 0.5, 1.0, 1.5, 2.0, 3.5});

  heading("histc_inferred_range");
  pluss::series::print(std::cout, hist_values.histc(4, 0.0, 0.0)) << "\n\n";

  heading("histc_explicit_range");
  std::cout << hist_values.histc(4, 0.0, 2.0) << "\n\n";

  heading("histc_ignores_out_of_range_and_nonfinite");
  std::cout << pluss::make_series({
      -1.0,
      0.25,
      0.75,
      std::numeric_limits<double>::infinity(),
      1.25,
      2.25,
  }).histc(4, 0.0, 2.0) << "\n\n";

  heading("histc_single_bin");
  std::cout << hist_values.histc(1, 0.0, 2.0) << "\n\n";

  heading("histc_bad_bins");
  try {
    std::cout << hist_values.histc(0, 0.0, 2.0) << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  return 0;
}
