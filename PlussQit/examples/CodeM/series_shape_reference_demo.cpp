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

template <class ShapeLike>
void print_shape(const ShapeLike& shape) {
  std::cout << "[";
  for (std::size_t i = 0; i < shape.size(); ++i) {
    if (i != 0) std::cout << ", ";
    std::cout << shape[i];
  }
  std::cout << "]";
}

}  // namespace

int main() {
  auto mat = pluss::make_series({
      {1.0, 2.0, 3.0},
      {4.0, 5.0, 6.0},
  });
  auto cube = pluss::make_series({
      {
          {1.0, 2.0, 3.0, 4.0},
          {5.0, 6.0, 7.0, 8.0},
          {9.0, 10.0, 11.0, 12.0},
      },
      {
          {13.0, 14.0, 15.0, 16.0},
          {17.0, 18.0, 19.0, 20.0},
          {21.0, 22.0, 23.0, 24.0},
      },
  });
  auto shape_template = pluss::make_series({
      {10.0, 20.0},
      {30.0, 40.0},
      {50.0, 60.0},
  });
  auto scalar = pluss::make_series(42.0);

  heading("size_dim0");
  std::cout << cube.size(0) << "\n\n";

  heading("size_negative_dim");
  std::cout << cube.size(-1) << "\n\n";

  heading("size_after_transpose");
  std::cout << cube.transpose(0, 2).size(1) << "\n\n";

  heading("size_scalar_error");
  try {
    std::cout << scalar.size(0) << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  heading("sizes_base");
  print_shape(cube.sizes());
  std::cout << "\n\n";

  heading("sizes_after_transpose");
  print_shape(cube.transpose(0, 2).sizes());
  std::cout << "\n\n";

  heading("sizes_scalar");
  print_shape(scalar.sizes());
  std::cout << "\n\n";

  heading("view_contiguous");
  std::cout << mat.view({3, 2}) << "\n\n";

  heading("view_reuse_other_sizes");
  std::cout << mat.view(shape_template.sizes()) << "\n\n";

  heading("view_noncontiguous_error");
  try {
    auto transposed = mat.transpose(0, 1);
    std::cout << transposed.view({3, 2}) << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  heading("view_numel_mismatch_error");
  try {
    std::cout << mat.view({4, 2}) << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  heading("reshape_contiguous");
  std::cout << mat.reshape({3, 2}) << "\n\n";

  heading("reshape_noncontiguous");
  std::cout << mat.transpose(0, 1).reshape({3, 2}) << "\n\n";

  auto tracked_mat = pluss::make_series({
      {1.0, 2.0, 3.0},
      {4.0, 5.0, 6.0},
  });
  tracked_mat.tracks_slope_(true);

  heading("reshape_gradient_fallback");
  tracked_mat.transpose(0, 1)
      .reshape({3, 2})
      .select(0, 1)
      .sum()
      .reverse();
  std::cout << tracked_mat.sensitivity() << "\n\n";

  heading("transpose_basic");
  std::cout << mat.transpose(0, 1) << "\n\n";

  heading("transpose_negative_dims");
  std::cout << cube.transpose(-1, -2) << "\n\n";

  tracked_mat = pluss::make_series({
      {1.0, 2.0, 3.0},
      {4.0, 5.0, 6.0},
  });
  tracked_mat.tracks_slope_(true);
  auto transposed = tracked_mat.transpose(0, 1);

  heading("transpose_gradient");
  transposed.select(0, 1).sum().reverse();
  std::cout << tracked_mat.sensitivity() << "\n\n";

  heading("permute_basic");
  std::cout << cube.permute({2, 0, 1}) << "\n\n";

  heading("permute_negative_dims");
  std::cout << cube.permute({-1, 0, 1}) << "\n\n";

  heading("permute_duplicate_error");
  try {
    std::cout << cube.permute({0, 0, 1}) << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  heading("unsqueeze_front");
  std::cout << mat.unsqueeze(0) << "\n\n";

  heading("unsqueeze_middle");
  std::cout << mat.unsqueeze(1) << "\n\n";

  heading("unsqueeze_end_negative");
  std::cout << mat.unsqueeze(-1) << "\n\n";

  auto singleton = pluss::make_series({
      {
          {
              {10.0, 20.0, 30.0},
          },
          {
              {40.0, 50.0, 60.0},
          },
      },
  }).unsqueeze(-1);

  heading("squeeze_all");
  std::cout << singleton.squeeze() << "\n\n";

  heading("squeeze_single_dim");
  std::cout << singleton.squeeze(2) << "\n\n";

  heading("squeeze_negative_dim");
  std::cout << singleton.squeeze(-1) << "\n\n";

  heading("squeeze_non_singleton_noop");
  std::cout << singleton.squeeze(1) << "\n\n";

  auto all_singletons = scalar.unsqueeze(0).unsqueeze(0);

  heading("squeeze_to_scalar");
  std::cout << all_singletons.squeeze() << "\n\n";

  auto flat_cube = pluss::make_series({
      {
          {1.0, 2.0},
          {3.0, 4.0},
      },
      {
          {5.0, 6.0},
          {7.0, 8.0},
      },
  });

  heading("flatten_default");
  std::cout << flat_cube.flatten() << "\n\n";

  heading("flatten_from_dim");
  std::cout << flat_cube.flatten(1) << "\n\n";

  heading("flatten_partial");
  std::cout << flat_cube.flatten(0, 1) << "\n\n";

  heading("flatten_noncontiguous");
  std::cout << flat_cube.transpose(0, 1).flatten(1, 2) << "\n\n";

  heading("flatten_scalar");
  std::cout << scalar.flatten() << "\n\n";

  heading("flatten_bad_range_error");
  try {
    std::cout << flat_cube.flatten(2, 1) << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  return 0;
}
