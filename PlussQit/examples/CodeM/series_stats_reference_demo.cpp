#include <pluss/pluss.h>

#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace {

void heading(const char* label) {
  std::cout << "=== " << label << " ===\n";
}

void print_error(const std::exception& error) {
  const std::string message = error.what();
  std::cout << message.substr(0, message.find('\n')) << "\n";
}

} // namespace

int main() {
  auto values = pluss::make_series({
      {1.0, 2.0, 3.0},
      {4.0, 5.0, 6.0},
  });

  heading("var_sample_global");
  std::cout << values.var() << "\n\n";

  heading("var_population");
  std::cout << values.var(false) << "\n\n";

  heading("var_dim0_population");
  std::cout << values.var(0, false) << "\n\n";

  heading("var_dim1_population");
  std::cout << values.var(1, false) << "\n\n";

  heading("var_sample_dim_keepdim");
  std::cout << values.var(1, true, true) << "\n\n";

  heading("var_negative_dim_population");
  std::cout << values.var(-1, false) << "\n\n";

  heading("var_stat_accessor");
  std::cout << values.stat.var(1, false) << "\n\n";

  heading("var_stat_proxy_explicit");
  std::cout << values.stat().var(1, false) << "\n\n";

  heading("var_population_singleton_lanes");
  auto singleton_lanes = pluss::make_series(std::vector<double>{1.0, 2.0}, {2, 1});
  std::cout << singleton_lanes.var(1, false) << "\n\n";

  heading("var_bad_dim_error");
  try {
    std::cout << values.var(2, false) << "\n";
  } catch (const std::exception& error) {
    print_error(error);
  }
  std::cout << "\n";

  heading("std_sample_global");
  std::cout << values.std() << "\n\n";

  heading("std_population_global");
  std::cout << values.std(false) << "\n\n";

  heading("std_dim0_population");
  std::cout << values.std(0, false) << "\n\n";

  heading("std_dim1_population");
  std::cout << values.std(1, false) << "\n\n";

  heading("std_stat_accessor");
  std::cout << values.stat.std(1, false) << "\n\n";

  heading("std_stat_proxy_explicit");
  std::cout << values.stat().std(1, false) << "\n\n";

  heading("std_keepdim_negative_dim");
  std::cout << values.std(-1, false, true) << "\n\n";

  heading("std_population_singleton_lanes");
  std::cout << singleton_lanes.std(1, false) << "\n\n";

  auto missing = pluss::make_series({
      {1.0, std::numeric_limits<double>::quiet_NaN(), 3.0},
      {std::numeric_limits<double>::quiet_NaN(), 5.0, 7.0},
  });

  heading("nanvar_global");
  std::cout << missing.nanvar() << "\n\n";

  heading("nanvar_dim0");
  std::cout << missing.nanvar(0) << "\n\n";

  heading("nanvar_dim1");
  std::cout << missing.nanvar(1) << "\n\n";

  heading("nanvar_dim_keepdim");
  std::cout << missing.stat.nanvar(1, true) << "\n\n";

  heading("nanvar_negative_dim");
  std::cout << missing.nanvar(-1) << "\n\n";

  heading("nanvar_stat_proxy_explicit");
  std::cout << missing.stat().nanvar(1) << "\n\n";

  heading("nanvar_bad_dim_error");
  try {
    std::cout << missing.nanvar(2) << "\n";
  } catch (const std::exception& error) {
    print_error(error);
  }
  std::cout << "\n";

  heading("nanstd_global");
  std::cout << missing.nanstd() << "\n\n";

  heading("nanstd_dim0");
  std::cout << missing.nanstd(0) << "\n\n";

  heading("nanstd_dim");
  std::cout << missing.nanstd(1) << "\n\n";

  heading("nanstd_keepdim_negative_dim");
  std::cout << missing.stat.nanstd(-1, true) << "\n\n";

  heading("nanstd_stat_proxy_explicit");
  std::cout << missing.stat().nanstd(1) << "\n\n";

  heading("std_vs_nanstd");
  std::cout << missing.std(1, false) << "\n";
  std::cout << missing.nanstd(1) << "\n\n";

  heading("all_nan_variance");
  auto none = pluss::make_series({
      std::numeric_limits<double>::quiet_NaN(),
      std::numeric_limits<double>::quiet_NaN(),
  });
  std::cout << none.nanvar() << "\n\n";

  heading("all_nan_std");
  std::cout << none.nanstd() << "\n\n";

  heading("unbiased_singleton_error");
  try {
    std::cout << singleton_lanes.var(1, true) << "\n";
  } catch (const std::exception& error) {
    print_error(error);
  }
  std::cout << "\n";

  heading("std_unbiased_singleton_error");
  try {
    std::cout << singleton_lanes.std(1, true) << "\n";
  } catch (const std::exception& error) {
    print_error(error);
  }
  std::cout << "\n";

  heading("std_bad_dim_error");
  try {
    std::cout << values.std(2, false) << "\n";
  } catch (const std::exception& error) {
    print_error(error);
  }
  std::cout << "\n";

  heading("nanstd_bad_dim_error");
  try {
    std::cout << missing.nanstd(2) << "\n";
  } catch (const std::exception& error) {
    print_error(error);
  }
  std::cout << "\n";

  heading("center_global");
  std::cout << values.center() << "\n\n";

  heading("center_rows");
  std::cout << values.stat.center(1) << "\n\n";

  heading("center_stat_proxy_explicit");
  std::cout << values.stat().center(1) << "\n\n";

  heading("center_columns");
  std::cout << values.center(0) << "\n\n";

  heading("center_negative_dim");
  std::cout << values.center(-1) << "\n\n";

  heading("center_scalar");
  std::cout << pluss::make_series(42.0).center() << "\n\n";

  heading("standardize_global");
  std::cout << values.standardize() << "\n\n";

  heading("standardize_rows");
  std::cout << values.stat.standardize(1) << "\n\n";

  heading("standardize_stat_proxy_explicit");
  std::cout << values.stat().standardize(1) << "\n\n";

  heading("standardize_columns");
  std::cout << values.standardize(0) << "\n\n";

  heading("standardize_negative_dim");
  std::cout << values.standardize(-1) << "\n\n";

  auto constant_rows = pluss::make_series(std::vector<double>{2, 2, 2, 5, 5, 5}, {2, 3});
  heading("standardize_constant_rows");
  std::cout << constant_rows.standardize(1) << "\n\n";

  heading("skewness_rows");
  std::cout << values.stat.skewness(1) << "\n\n";

  heading("skewness_stat_proxy_rows");
  std::cout << values.stat().skewness(1) << "\n\n";

  heading("kurtosis_rows");
  std::cout << values.stat.kurtosis(1) << "\n\n";

  heading("kurtosis_stat_proxy_rows");
  std::cout << values.stat().kurtosis(1) << "\n\n";

  auto lopsided = pluss::make_series(std::vector<double>{1, 1, 4, 2, 2, 8}, {2, 3});
  heading("skewness_lopsided_rows");
  std::cout << lopsided.stat.skewness(1) << "\n\n";

  heading("kurtosis_lopsided_rows");
  std::cout << lopsided.stat.kurtosis(1) << "\n\n";

  heading("skewness_global");
  std::cout << lopsided.stat.skewness() << "\n\n";

  heading("skewness_negative_dim");
  std::cout << lopsided.stat.skewness(-1) << "\n\n";

  heading("kurtosis_global");
  std::cout << lopsided.stat.kurtosis() << "\n\n";

  heading("kurtosis_negative_dim");
  std::cout << lopsided.stat.kurtosis(-1) << "\n";

  return 0;
}
