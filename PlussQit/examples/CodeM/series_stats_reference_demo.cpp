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
  std::cout << values.var(0, false, false) << "\n\n";

  heading("var_dim1_population");
  std::cout << values.var(1, false, false) << "\n\n";

  heading("var_sample_dim_keepdim");
  std::cout << values.var(1, true, true) << "\n\n";

  heading("var_negative_dim_population");
  std::cout << values.var(-1, false, false) << "\n\n";

  heading("var_stat_accessor");
  std::cout << values.stat.var(1, false) << "\n\n";

  heading("var_stat_proxy_explicit");
  std::cout << values.stat().var(1, false) << "\n\n";

  heading("var_population_singleton_lanes");
  auto singleton_lanes = pluss::make_series(std::vector<double>{1.0, 2.0}, {2, 1});
  std::cout << singleton_lanes.var(1, false, false) << "\n\n";

  heading("var_bad_dim_error");
  try {
    std::cout << values.var(2, false, false) << "\n";
  } catch (const std::exception& error) {
    print_error(error);
  }
  std::cout << "\n";

  heading("std_sample_global");
  std::cout << values.std() << "\n\n";

  heading("std_population_global");
  std::cout << values.std(false) << "\n\n";

  heading("std_dim0_population");
  std::cout << values.std(0, false, false) << "\n\n";

  heading("std_dim1_population");
  std::cout << values.std(1, false, false) << "\n\n";

  heading("std_stat_accessor");
  std::cout << values.stat.std(1, false) << "\n\n";

  heading("std_stat_proxy_explicit");
  std::cout << values.stat().std(1, false) << "\n\n";

  heading("std_keepdim_negative_dim");
  std::cout << values.std(-1, false, true) << "\n\n";

  heading("std_population_singleton_lanes");
  std::cout << singleton_lanes.std(1, false, false) << "\n\n";

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
  std::cout << missing.std(1, false, false) << "\n";
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
    std::cout << singleton_lanes.var(1, true, false) << "\n";
  } catch (const std::exception& error) {
    print_error(error);
  }
  std::cout << "\n";

  heading("std_unbiased_singleton_error");
  try {
    std::cout << singleton_lanes.std(1, true, false) << "\n";
  } catch (const std::exception& error) {
    print_error(error);
  }
  std::cout << "\n";

  heading("std_bad_dim_error");
  try {
    std::cout << values.std(2, false, false) << "\n";
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

  auto order_values = pluss::make_series({1.0, 2.0, 3.0, 4.0});
  heading("median_global_even");
  pluss::series::print(std::cout, order_values.median());
  std::cout << "\n\n";

  auto order_table = pluss::make_series({
      {1.0, 8.0, 3.0},
      {4.0, 2.0, 6.0},
      {7.0, 5.0, 9.0},
  });
  heading("median_dim0");
  std::cout << order_table.median(0) << "\n\n";

  heading("median_negative_dim");
  std::cout << order_table.median(-1) << "\n\n";

  heading("median_bad_dim_error");
  try {
    std::cout << order_table.median(2) << "\n";
  } catch (const std::exception& error) {
    print_error(error);
  }
  std::cout << "\n";

  auto missing_order = pluss::make_series({1.0, std::numeric_limits<double>::quiet_NaN(), 5.0, 9.0});
  heading("median_nan");
  std::cout << missing_order.median() << "\n\n";

  heading("nanmedian_ignore_nan");
  std::cout << missing_order.nanmedian() << "\n\n";

  heading("nanmedian_dim0");
  std::cout << pluss::make_series({
      {1.0, std::numeric_limits<double>::quiet_NaN(), 9.0},
      {3.0, 5.0, std::numeric_limits<double>::quiet_NaN()},
  }).nanmedian(0) << "\n\n";

  heading("nanmedian_all_nan");
  std::cout << pluss::make_series({
      std::numeric_limits<double>::quiet_NaN(),
      std::numeric_limits<double>::quiet_NaN(),
  }).nanmedian() << "\n\n";

  heading("nanmedian_bad_dim_error");
  try {
    std::cout << missing_order.nanmedian(1) << "\n";
  } catch (const std::exception& error) {
    print_error(error);
  }
  std::cout << "\n";

  heading("quantile_default");
  std::cout << order_values.quantile() << "\n\n";

  heading("quantile_quarter");
  std::cout << order_values.quantile(0.25) << "\n\n";

  heading("quantile_endpoints");
  std::cout << order_values.quantile(0.0) << "\n";
  std::cout << order_values.quantile(1.0) << "\n\n";

  heading("nanquantile_ignore_nan");
  std::cout << missing_order.nanquantile(0.5) << "\n\n";

  heading("nanquantile_default");
  std::cout << missing_order.nanquantile() << "\n\n";

  heading("quantile_vs_nanquantile");
  std::cout << missing_order.quantile(0.5) << "\n";
  std::cout << missing_order.nanquantile(0.5) << "\n\n";

  heading("quantile_bad_q_error");
  try {
    std::cout << order_values.quantile(1.1) << "\n";
  } catch (const std::exception& error) {
    print_error(error);
  }
  std::cout << "\n";

  heading("nanquantile_bad_q_error");
  try {
    std::cout << missing_order.nanquantile(-0.25) << "\n";
  } catch (const std::exception& error) {
    print_error(error);
  }
  std::cout << "\n";

  auto modes = pluss::make_series({4.0, 2.0, 4.0, 3.0, 2.0, 4.0});
  heading("mode_global");
  std::cout << modes.mode() << "\n\n";

  auto mode_table = pluss::make_series({
      {1.0, 2.0, 2.0, 3.0},
      {4.0, 4.0, 5.0, 5.0},
  });
  heading("mode_dim1_tie");
  std::cout << mode_table.mode(1) << "\n\n";

  heading("mode_dim0");
  std::cout << mode_table.mode(0) << "\n\n";

  heading("mode_empty_error");
  try {
    std::cout << pluss::series::zeros({0}).mode() << "\n";
  } catch (const std::exception& error) {
    print_error(error);
  }
  std::cout << "\n\n";

  heading("mode_empty_dim_error");
  try {
    std::cout << pluss::series::zeros({2, 0}).mode(1) << "\n";
  } catch (const std::exception& error) {
    print_error(error);
  }

  return 0;
}
