#include <pluss/pluss.h>

#include <cmath>
#include <iostream>
#include <string>

namespace {

void heading(const char* label) {
  std::cout << "=== " << label << " ===\n";
}

void print_error_prefix(const std::exception& e) {
  std::string msg = e.what();
  const auto nl = msg.find('\n');
  std::cout << msg.substr(0, nl);
}

} // namespace

int main() {
  auto values = pluss::make_series({
      {1.0, 7.0, 3.0},
      {4.0, 7.0, 6.0},
  });

  heading("amax_global");
  std::cout << values.amax() << "\n\n";

  heading("amax_dim0");
  pluss::series::print(std::cout, values.amax(0)) << "\n\n";

  heading("amax_dim1_keepdim");
  std::cout << values.amax(1, true) << "\n\n";

  heading("amax_negative_dim");
  std::cout << values.amax(-1) << "\n\n";

  heading("amax_tie_gradient");
  auto amax_grad = pluss::make_series({2.0, 5.0, 5.0});
  amax_grad.tracks_slope_(true);
  amax_grad.amax().reverse();
  pluss::series::print(std::cout, amax_grad.sensitivity()) << "\n\n";

  heading("amax_bad_dim_error");
  try {
    std::cout << values.amax(2) << "\n";
  } catch (const std::exception& e) {
    print_error_prefix(e);
  }
  std::cout << "\n\n";

  heading("amin_global");
  std::cout << values.amin() << "\n\n";

  heading("amin_dim0");
  pluss::series::print(std::cout, values.amin(0)) << "\n\n";

  heading("amin_dim1_keepdim");
  std::cout << values.amin(1, true) << "\n\n";

  heading("amin_negative_dim");
  std::cout << values.amin(-1) << "\n\n";

  heading("amin_tie_gradient");
  auto amin_grad = pluss::make_series({-2.0, -2.0, 3.0});
  amin_grad.tracks_slope_(true);
  amin_grad.amin().reverse();
  std::cout << amin_grad.sensitivity() << "\n\n";

  heading("amin_bad_dim_error");
  try {
    std::cout << values.amin(3) << "\n";
  } catch (const std::exception& e) {
    print_error_prefix(e);
  }
  std::cout << "\n\n";

  auto vec = pluss::make_series({3.0, 4.0});
  auto signed_vec = pluss::make_series({-1.0, 2.0, -3.0});
  auto general_vec = pluss::make_series({1.0, 2.0, 2.0});
  auto norm_mat = pluss::make_series({
      {3.0, 4.0},
      {5.0, 12.0},
  });

  heading("norm_l2_vector");
  std::cout << vec.norm(2.0) << "\n\n";

  heading("norm_l1_vector");
  pluss::series::print(std::cout, signed_vec.norm(1.0)) << "\n\n";

  heading("norm_linf_vector");
  std::cout << signed_vec.norm(std::numeric_limits<double>::infinity()) << "\n\n";

  heading("norm_general_p3");
  std::cout << general_vec.norm(3.0) << "\n\n";

  heading("norm_dim1");
  std::cout << norm_mat.norm(2.0, 1, false) << "\n\n";

  heading("norm_dim1_keepdim");
  pluss::series::print(std::cout, norm_mat.norm(2.0, 1, true)) << "\n\n";

  heading("norm_negative_dim");
  std::cout << norm_mat.norm(2.0, -1, false) << "\n\n";

  heading("norm_l2_gradient");
  auto norm_grad = pluss::make_series({3.0, 4.0});
  norm_grad.tracks_slope_(true);
  norm_grad.norm(2.0).reverse();
  std::cout << "y=" << norm_grad.norm(2.0) << "\n";
  std::cout << "dx=" << norm_grad.sensitivity() << "\n\n";

  heading("norm_bad_dim_error");
  try {
    std::cout << norm_mat.norm(2.0, 2, false) << "\n";
  } catch (const std::exception& e) {
    print_error_prefix(e);
  }
  std::cout << "\n";

  return 0;
}
