#include <pluss/pluss.h>

#include <exception>
#include <iostream>
#include <limits>
#include <string>

namespace {

void heading(const char* label) { std::cout << "=== " << label << " ===\n"; }

void print_error(const std::exception& error) {
  const std::string message = error.what();
  std::cout << message.substr(0, message.find('\n')) << "\n";
}

} // namespace

int main() {
  heading("mean_accessor_and_proxy");
  auto table = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});
  std::cout << "global=" << table.stat.mean() << "\n";
  std::cout << "rows=" << table.stat.mean(1) << "\n";
  std::cout << "keepdim=" << table.stat().mean(-1, true) << "\n\n";

  heading("normal_distribution_defaults");
  auto points = pluss::make_series({-1.0, 0.0, 1.0});
  std::cout << "pdf=" << points.stat.pdf() << "\n";
  std::cout << "logpdf=" << points.stat.logpdf() << "\n";
  std::cout << "cdf=" << points.stat.cdf() << "\n";
  std::cout << "logcdf=" << points.stat.logcdf() << "\n\n";

  heading("normal_distribution_parameters_and_tail");
  auto shifted = pluss::make_series({-1.0, 1.0, 3.0});
  std::cout << "pdf_mu1_sigma2=" << shifted.stat.pdf(1.0, 2.0) << "\n";
  std::cout << "cdf_mu1_sigma2=" << shifted.stat.cdf(1.0, 2.0) << "\n";
  std::cout << "logcdf_tail=" << pluss::make_series({-20.0, -10.0, 10.0}).stat.logcdf() << "\n\n";

  heading("normal_distribution_gradients");
  auto pdf_x = pluss::make_series({-1.0, 0.0, 1.0});
  pdf_x.tracks_slope_(true);
  pdf_x.stat.pdf().sum().reverse();
  std::cout << "pdf_gradient=" << pdf_x.sensitivity() << "\n";
  auto cdf_x = pluss::make_series({-1.0, 0.0, 1.0});
  cdf_x.tracks_slope_(true);
  cdf_x.stat.cdf().sum().reverse();
  std::cout << "cdf_gradient=" << cdf_x.sensitivity() << "\n";
  auto logpdf_x = pluss::make_series({-1.0, 0.0, 1.0});
  logpdf_x.tracks_slope_(true);
  logpdf_x.stat.logpdf().sum().reverse();
  std::cout << "logpdf_gradient=" << logpdf_x.sensitivity() << "\n";
  auto logcdf_x = pluss::make_series({-20.0, -10.0, 0.0});
  logcdf_x.tracks_slope_(true);
  logcdf_x.stat.logcdf().sum().reverse();
  std::cout << "logcdf_gradient=" << logcdf_x.sensitivity() << "\n\n";

  heading("normal_distribution_error");
  try {
    std::cout << points.stat.pdf(0.0) << "\n";
  } catch (const std::exception& error) {
    print_error(error);
  }
  std::cout << "\n";

  auto sequence = pluss::make_series({1.0, 2.0, 3.0, 4.0});
  heading("moving_window_default_and_explicit");
  std::cout << "sum_default=" << sequence.stat.movsum() << "\n";
  std::cout << "sum_w2=" << sequence.stat.movsum(2) << "\n";
  std::cout << "mean_w3=" << sequence.stat.movmean(3) << "\n";
  std::cout << "max_w3=" << sequence.stat.movmax(3) << "\n";
  std::cout << "min_w3=" << sequence.stat.movmin(3) << "\n";
  std::cout << "var_w3=" << sequence.stat.movvar(3) << "\n";
  std::cout << "std_w3=" << sequence.stat.movstd(3) << "\n\n";

  heading("moving_window_dimensions");
  std::cout << "sum_dim0=" << table.stat.movsum(2, 0) << "\n";
  std::cout << "mean_dim1=" << table.stat.movmean(2, 1) << "\n\n";

  heading("moving_window_gradients_and_error");
  auto moving_x = pluss::make_series({1.0, 2.0, 3.0, 4.0});
  moving_x.tracks_slope_(true);
  moving_x.stat.movmean(3).sum().reverse();
  std::cout << "mean_gradient=" << moving_x.sensitivity() << "\n";

  auto moving_ties = pluss::make_series({2.0, 2.0, 1.0});
  moving_ties.tracks_slope_(true);
  moving_ties.stat.movmax(2).sum().reverse();
  std::cout << "max_tie_gradient=" << moving_ties.sensitivity() << "\n";

  auto moving_min_ties = pluss::make_series({1.0, 1.0, 2.0});
  moving_min_ties.tracks_slope_(true);
  moving_min_ties.stat.movmin(2).sum().reverse();
  std::cout << "min_tie_gradient=" << moving_min_ties.sensitivity() << "\n";

  auto moving_var = pluss::make_series({1.0, 2.0, 4.0});
  moving_var.tracks_slope_(true);
  moving_var.stat.movvar(2).sum().reverse();
  std::cout << "var_gradient=" << moving_var.sensitivity() << "\n";

  auto moving_std = pluss::make_series({1.0, 2.0, 4.0});
  moving_std.tracks_slope_(true);
  moving_std.stat.movstd(2).sum().reverse();
  std::cout << "std_gradient=" << moving_std.sensitivity() << "\n";
  try {
    std::cout << sequence.stat.movsum(0) << "\n";
  } catch (const std::exception& error) {
    print_error(error);
  }
  std::cout << "\n";

  heading("row_and_column_normalization");
  auto norms = pluss::make_series({{3.0, 4.0}, {0.0, 5.0}});
  std::cout << "normr=" << norms.stat.normr() << "\n";
  std::cout << "normc=" << norms.stat.normc() << "\n";
  std::cout << "zero_rows=" << pluss::make_series({{0.0, 0.0}, {3.0, 4.0}}).stat.normr() << "\n\n";

  heading("normalization_gradient");
  auto norm_x = pluss::make_series({{3.0, 4.0}, {5.0, 12.0}});
  norm_x.tracks_slope_(true);
  norm_x.stat.normr().sum().reverse();
  std::cout << norm_x.sensitivity() << "\n\n";

  heading("pairwise_distances_and_call_styles");
  auto left = pluss::make_series({{0.0, 0.0}, {1.0, 1.0}});
  auto right = pluss::make_series({{1.0, 0.0}, {3.0, 4.0}});
  std::cout << "cdist_direct=" << left.stat.cdist(right) << "\n";
  std::cout << "cdist_bound=" << left.stat(right).cdist() << "\n";
  std::cout << "pdist2=" << left.stat.pdist2(right) << "\n\n";

  heading("pairwise_distance_gradients");
  auto distance_left = pluss::make_series({{0.0, 0.0}, {1.0, 1.0}});
  auto distance_right = pluss::make_series({{1.0, 0.0}, {3.0, 4.0}});
  distance_left.tracks_slope_(true);
  distance_right.tracks_slope_(true);
  distance_left.stat.cdist(distance_right).sum().reverse();
  std::cout << "left_gradient=" << distance_left.sensitivity() << "\n";
  std::cout << "right_gradient=" << distance_right.sensitivity() << "\n\n";

  heading("coincident_pairwise_distance_gradient");
  auto coincident_left = pluss::make_series({{1.0, 2.0}});
  auto coincident_right = pluss::make_series({{1.0, 2.0}});
  coincident_left.tracks_slope_(true);
  coincident_right.tracks_slope_(true);
  coincident_left.stat.cdist(coincident_right).sum().reverse();
  std::cout << "left_gradient=" << coincident_left.sensitivity() << "\n";
  std::cout << "right_gradient=" << coincident_right.sensitivity() << "\n\n";

  heading("pairwise_distance_error");
  try {
    std::cout << left.stat.cdist(pluss::make_series({{1.0, 2.0, 3.0}})) << "\n";
  } catch (const std::exception& error) {
    print_error(error);
  }

  return 0;
}
