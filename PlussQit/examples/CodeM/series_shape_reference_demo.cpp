#include <pluss/pluss.h>

#include <array>
#include <cstdint>
#include <iostream>
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

void print_sizes(pluss::msharp::RangeView::Int shape) {
  std::cout << "[";
  for (std::size_t i = 0; i < shape.size(); ++i) {
    if (i != 0) std::cout << ", ";
    std::cout << shape[i];
  }
  std::cout << "]";
}

}  // namespace

int main() {
  auto mat = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6}, {2, 3});
  auto cube = pluss::make_series(
      std::vector<double>{
          1,  2,  3,  4,
          5,  6,  7,  8,
          9,  10, 11, 12,
          13, 14, 15, 16,
          17, 18, 19, 20,
          21, 22, 23, 24},
      {2, 3, 4});
  auto shape_template = pluss::make_series(std::vector<double>{10, 20, 30, 40, 50, 60}, {3, 2});
  const std::array<std::int64_t, 2> shape_3x2{{3, 2}};

  heading("size_dim0");
  std::cout << cube.size(0) << "\n\n";

  heading("size_negative_dim");
  std::cout << cube.size(-1) << "\n\n";

  heading("size_after_transpose");
  std::cout << cube.transpose(0, 2).size(1) << "\n\n";

  heading("sizes_base");
  print_sizes(cube.sizes());
  std::cout << "\n\n";

  heading("sizes_after_transpose");
  print_sizes(cube.transpose(0, 2).sizes());
  std::cout << "\n\n";

  heading("view_contiguous");
  std::cout << mat.view(
      std::span<const std::int64_t>(shape_3x2.data(), shape_3x2.size())) << "\n\n";

  heading("view_reuse_other_sizes");
  std::cout << mat.view(shape_template.sizes()) << "\n\n";

  heading("view_noncontiguous_error");
  try {
    auto transposed = mat.transpose(0, 1);
    std::cout << transposed.view(
        std::span<const std::int64_t>(shape_3x2.data(), shape_3x2.size())) << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  heading("reshape_contiguous");
  std::cout << mat.reshape(
      std::span<const std::int64_t>(shape_3x2.data(), shape_3x2.size())) << "\n\n";

  heading("reshape_noncontiguous");
  std::cout << mat.transpose(0, 1).reshape(
      std::span<const std::int64_t>(shape_3x2.data(), shape_3x2.size())) << "\n\n";

  auto tracked_mat = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0, 6.0}, {2, 3});
  tracked_mat.tracks_slope_(true);

  heading("reshape_gradient_fallback");
  tracked_mat.transpose(0, 1)
      .reshape(std::span<const std::int64_t>(shape_3x2.data(), shape_3x2.size()))
      .select(0, 1)
      .sum()
      .reverse();
  std::cout << tracked_mat.sensitivity() << "\n\n";

  heading("transpose_basic");
  std::cout << mat.transpose(0, 1) << "\n\n";

  heading("transpose_negative_dims");
  std::cout << cube.transpose(-1, -2) << "\n\n";

  tracked_mat = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0, 6.0}, {2, 3});
  tracked_mat.tracks_slope_(true);
  auto transposed = tracked_mat.transpose(0, 1);

  heading("transpose_gradient");
  transposed.select(0, 1).sum().reverse();
  std::cout << tracked_mat.sensitivity() << "\n\n";

  heading("permute_basic");
  std::cout << cube.permute({2, 0, 1}) << "\n\n";

  heading("permute_duplicate_error");
  try {
    std::cout << cube.permute({0, 0, 1}) << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  heading("unsqueeze_front");
  std::cout << mat.unsqueeze(0) << "\n\n";

  heading("unsqueeze_end_negative");
  std::cout << mat.unsqueeze(-1) << "\n\n";

  auto singleton =
      pluss::make_series(std::vector<double>{10, 20, 30, 40, 50, 60}, {1, 2, 1, 3, 1});

  heading("squeeze_all");
  std::cout << singleton.squeeze() << "\n\n";

  heading("squeeze_single_dim");
  std::cout << singleton.squeeze(2) << "\n\n";

  heading("squeeze_non_singleton_noop");
  std::cout << singleton.squeeze(1) << "\n\n";

  auto all_singletons = pluss::make_series(std::vector<double>{42.0}, {1, 1});

  heading("squeeze_to_scalar");
  std::cout << all_singletons.squeeze() << "\n\n";

  auto flat_cube = pluss::make_series(
      std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8},
      {2, 2, 2});

  heading("flatten_default");
  std::cout << flat_cube.flatten() << "\n\n";

  heading("flatten_partial");
  std::cout << flat_cube.flatten(1) << "\n\n";

  heading("flatten_noncontiguous");
  std::cout << flat_cube.transpose(0, 1).flatten(1, 2) << "\n\n";

  return 0;
}
