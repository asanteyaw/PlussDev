#include <pluss/pluss.h>

#include <iostream>
#include <limits>

namespace {

void heading(const char* label) {
  std::cout << "=== " << label << " ===\n";
}

}  // namespace

int main() {
  using std::numeric_limits;

  heading("floor_basic");
  pluss::series::print(std::cout, pluss::make_series({
      {-1.2, -0.1, 0.0},
      {1.2, 2.9, 3.0},
  }).floor()) << "\n\n";

  heading("floor_gradient");
  auto floor_x = pluss::make_series({-1.2, 0.0, 3.8});
  floor_x.tracks_slope_(true);
  auto floor_y = floor_x.floor();
  floor_y.sum().reverse();
  std::cout << "out=" << floor_y << "\n";
  std::cout << "dL_dx=" << floor_x.sensitivity() << "\n\n";

  heading("ceil_basic");
  std::cout << pluss::make_series({-1.2, -0.1, 0.0, 1.2, 2.1}).ceil() << "\n\n";

  heading("ceil_gradient");
  auto ceil_x = pluss::make_series({-1.2, 0.0, 3.8});
  ceil_x.tracks_slope_(true);
  auto ceil_y = ceil_x.ceil();
  ceil_y.sum().reverse();
  std::cout << "out=" << ceil_y << "\n";
  std::cout << "dL_dx=" << ceil_x.sensitivity() << "\n\n";

  heading("round_basic");
  std::cout << pluss::make_series({-1.6, -1.5, -1.4, 1.4, 1.5, 1.6}).round() << "\n\n";

  heading("round_gradient");
  auto round_x = pluss::make_series({-1.2, 0.0, 3.8});
  round_x.tracks_slope_(true);
  auto round_y = round_x.round();
  round_y.sum().reverse();
  std::cout << "out=" << round_y << "\n";
  std::cout << "dL_dx=" << round_x.sensitivity() << "\n\n";

  heading("trunc_basic");
  pluss::series::print(std::cout, pluss::make_series({-1.9, -1.1, 0.0, 1.1, 1.9}).trunc()) << "\n\n";

  heading("trunc_gradient");
  auto trunc_x = pluss::make_series({-1.2, 0.0, 3.8});
  trunc_x.tracks_slope_(true);
  auto trunc_y = trunc_x.trunc();
  trunc_y.sum().reverse();
  std::cout << "out=" << trunc_y << "\n";
  std::cout << "dL_dx=" << trunc_x.sensitivity() << "\n\n";

  auto special = pluss::make_series({
      -numeric_limits<double>::infinity(),
      -1.0,
      numeric_limits<double>::quiet_NaN(),
      0.0,
      numeric_limits<double>::infinity(),
  });

  heading("isnan_basic");
  std::cout << special.isnan() << "\n\n";

  heading("isinf_basic");
  std::cout << special.isinf() << "\n\n";

  heading("isfinite_basic");
  pluss::series::print(std::cout, special.isfinite()) << "\n\n";

  heading("nan_to_num_defaultish");
  std::cout << special.nan_to_num(0.0, 99.0, -99.0) << "\n\n";

  heading("nan_to_num_matrix");
  auto messy = pluss::make_series({
      {numeric_limits<double>::quiet_NaN(), 1.5, numeric_limits<double>::infinity()},
      {-numeric_limits<double>::infinity(), -2.5, 0.0},
  });
  std::cout << messy.nan_to_num(-7.0, 500.0, -500.0) << "\n\n";

  heading("nan_to_num_gradient");
  auto clean_x = pluss::make_series({
      numeric_limits<double>::quiet_NaN(),
      2.0,
      numeric_limits<double>::infinity(),
      -3.0,
  });
  clean_x.tracks_slope_(true);
  auto clean_y = clean_x.nan_to_num(0.0, 10.0, -10.0);
  clean_y.sum().reverse();
  std::cout << "out=" << clean_y << "\n";
  std::cout << "dL_dx=" << clean_x.sensitivity() << "\n\n";

  heading("invalid_mask");
  auto invalid_values = pluss::make_series({
      {1.0, numeric_limits<double>::quiet_NaN(), 2.0},
      {numeric_limits<double>::infinity(), -numeric_limits<double>::infinity(), 5.0},
  });
  std::cout << pluss::series::print(invalid_values.invalid()) << "\n\n";

  heading("invalid_index");
  pluss::series::print(std::cout, invalid_values.invalid_index()) << "\n\n";

  heading("invalid_take");
  pluss::series::print(std::cout, invalid_values.take(invalid_values.invalid_index())) << "\n\n";

  heading("invalid_fill_constant");
  auto repaired_constant = invalid_values.clone();
  repaired_constant.where_(repaired_constant.invalid(), -99.0);
  pluss::series::print(std::cout, repaired_constant) << "\n\n";

  heading("invalid_fill_array");
  auto repaired_array = invalid_values.clone();
  repaired_array.put_(repaired_array.invalid_index(), pluss::make_series({70.0, 80.0, 90.0}));
  std::cout << pluss::series::print(repaired_array) << "\n\n";

  heading("invalid_mask_ints");
  std::cout << pluss::make_series({1, 2, 3}, pluss::ScalarType::Int64).invalid() << "\n\n";

  heading("invalid_index_valid");
  pluss::series::print(std::cout, pluss::make_series({1.5, 2.5, 3.5}).invalid_index()) << "\n\n";

  return 0;
}
