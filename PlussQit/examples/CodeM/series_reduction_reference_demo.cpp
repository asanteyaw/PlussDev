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
  auto mat = pluss::make_series({
      {1.0, 2.0, 3.0},
      {4.0, 5.0, 6.0},
  });

  heading("sum_all");
  std::cout << mat.sum() << "\n\n";

  heading("sum_dim1");
  std::cout << mat.sum(1) << "\n\n";

  heading("sum_dim1_keepdim");
  std::cout << mat.sum(1, true) << "\n\n";

  heading("sum_negative_dim");
  std::cout << mat.sum(-1) << "\n\n";

  auto flags = pluss::make_series({
      {true, false, true},
      {false, true, true},
  });
  heading("sum_bool_counts_true");
  std::cout << flags.sum() << "\n\n";

  auto tracked_sum = mat.clone();
  tracked_sum.tracks_slope_(true);
  auto summed = tracked_sum.sum(1);
  summed.sum().reverse();
  heading("sum_gradient");
  std::cout << "y=" << summed << "\n";
  std::cout << "dx=" << tracked_sum.sensitivity() << "\n\n";

  heading("mean_all");
  std::cout << mat.mean() << "\n\n";

  heading("mean_dim0");
  std::cout << mat.mean(0) << "\n\n";

  heading("mean_dim1_keepdim");
  std::cout << mat.mean(1, true) << "\n\n";

  auto int_mat = pluss::make_series({
      {1, 2, 3},
      {4, 5, 6},
  });
  heading("mean_integral_promotes");
  std::cout << int_mat.mean() << "\n\n";

  auto tracked_mean = mat.clone();
  tracked_mean.tracks_slope_(true);
  auto averaged = tracked_mean.mean(1);
  averaged.sum().reverse();
  heading("mean_gradient");
  std::cout << "y=" << averaged << "\n";
  std::cout << "dx=" << tracked_mean.sensitivity() << "\n\n";

  heading("prod_all");
  std::cout << mat.prod() << "\n\n";

  heading("prod_dim1");
  std::cout << mat.prod(1) << "\n\n";

  heading("prod_dim1_keepdim");
  std::cout << mat.prod(1, true) << "\n\n";

  auto prod_with_zero = pluss::make_series({
      {2.0, 0.0, 4.0},
      {3.0, 5.0, 1.0},
  });
  heading("prod_zero_lane");
  std::cout << prod_with_zero.prod(1) << "\n\n";

  auto tracked_prod = mat.clone();
  tracked_prod.tracks_slope_(true);
  auto produced = tracked_prod.prod(1);
  produced.sum().reverse();
  heading("prod_gradient_current");
  std::cout << "y=" << produced << "\n";
  std::cout << "has_dx=" << (tracked_prod.has_sensitivity() ? "true" : "false") << "\n";
  if (tracked_prod.has_sensitivity()) {
    std::cout << "dx=" << tracked_prod.sensitivity() << "\n";
  }
  std::cout << "\n";

  heading("cumsum_dim1");
  std::cout << mat.cumsum(1) << "\n\n";

  heading("cumsum_dim0");
  std::cout << mat.cumsum(0) << "\n\n";

  heading("cumsum_keepdim_ignored");
  std::cout << (mat.cumsum(1, false).sizes() == mat.cumsum(1, true).sizes() ? "true" : "false") << "\n\n";

  auto tracked_cumsum = mat.clone();
  tracked_cumsum.tracks_slope_(true);
  auto scanned_sum = tracked_cumsum.cumsum(1);
  scanned_sum.sum().reverse();
  heading("cumsum_gradient");
  std::cout << "y=" << scanned_sum << "\n";
  std::cout << "dx=" << tracked_cumsum.sensitivity() << "\n\n";

  heading("cumprod_dim1");
  std::cout << mat.cumprod(1) << "\n\n";

  heading("cumprod_dim0");
  std::cout << mat.cumprod(0) << "\n\n";

  heading("cumprod_keepdim_ignored");
  std::cout << (mat.cumprod(1, false).sizes() == mat.cumprod(1, true).sizes() ? "true" : "false") << "\n\n";

  auto tracked_cumprod = mat.clone();
  tracked_cumprod.tracks_slope_(true);
  auto scanned_prod = tracked_cumprod.cumprod(1);
  scanned_prod.sum().reverse();
  heading("cumprod_gradient_current");
  std::cout << "y=" << scanned_prod << "\n";
  std::cout << "has_dx=" << (tracked_cumprod.has_sensitivity() ? "true" : "false") << "\n";
  if (tracked_cumprod.has_sensitivity()) {
    std::cout << "dx=" << tracked_cumprod.sensitivity() << "\n";
  }
  std::cout << "\n";

  heading("sum_bad_dim_error");
  try {
    std::cout << mat.sum(2) << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  return 0;
}
