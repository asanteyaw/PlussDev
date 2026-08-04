#include <pluss/pluss.h>

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace {

using pluss::codem::ExtractSpec;
using ExtractSpecView = pluss::msharp::RangeView::Of<ExtractSpec>;

ExtractSpecView specs_view(const std::vector<ExtractSpec>& specs) {
  return ExtractSpecView(specs);
}

void heading(const char* label) {
  std::cout << "=== " << label << " ===\n";
}

void print_error(const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}

}  // namespace

int main() {
  auto vec = pluss::make_series(std::vector<double>{10, 20, 30, 40}, {4});
  auto mat = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6}, {2, 3});
  auto cube = pluss::make_series(
      std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8},
      {2, 2, 2});
  auto grid = pluss::make_series(
      std::vector<double>{10, 11, 12, 13, 20, 21, 22, 23, 30, 31, 32, 33},
      {3, 4});

  heading("pos_1d_last");
  std::cout << vec.pos(-1) << "\n\n";

  heading("pos_2d_negative_col");
  std::cout << mat.pos(1, -1) << "\n\n";

  heading("pos_3d_negative_tail");
  std::cout << cube.pos(1, -1, -1) << "\n\n";

  heading("pos_rank_error");
  try {
    std::cout << vec.pos(0, 0) << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  auto tracked_vec = pluss::make_series(std::vector<double>{10.0, 20.0, 30.0, 40.0}, {4});
  tracked_vec.tracks_slope_(true);

  heading("pos_gradient");
  tracked_vec.pos(-1).reverse();
  std::cout << tracked_vec.sensitivity() << "\n\n";

  heading("slice_step");
  std::cout << grid.slice(1, 1, 4, 2) << "\n\n";

  heading("slice_default_step");
  std::cout << grid.slice(1, 2, 4) << "\n\n";

  heading("slice_from_start");
  std::cout << grid.slice(1, 1) << "\n\n";

  heading("slice_full_axis");
  std::cout << grid.slice(1) << "\n\n";

  heading("slice_negative_dim");
  std::cout << grid.slice(-1, 0, 4, 2) << "\n\n";

  heading("slice_negative_step_error");
  try {
    std::cout << grid.slice(1, -1, pluss::Slicing::Slice::kUnbounded, -1) << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  auto tracked_grid = pluss::make_series(
      std::vector<double>{10.0, 11.0, 12.0, 13.0, 20.0, 21.0, 22.0, 23.0, 30.0, 31.0, 32.0, 33.0},
      {3, 4});
  tracked_grid.tracks_slope_(true);

  heading("slice_gradient");
  tracked_grid.slice(1, 1, 4, 2).sum().reverse();
  std::cout << tracked_grid.sensitivity() << "\n\n";

  heading("traverse_slice_row_stride");
  const std::vector<ExtractSpec> row_stride{
      std::int64_t{1},
      pluss::Slicing::SliceRange(1, 4, 2)};
  std::cout << grid.traverse_slice(specs_view(row_stride)) << "\n\n";

  heading("traverse_slice_newaxis_column");
  const std::vector<ExtractSpec> newaxis_column{
      pluss::Slicing::NewAxis(),
      pluss::Slicing::SliceAll(),
      std::int64_t{2}};
  std::cout << grid.traverse_slice(specs_view(newaxis_column)) << "\n\n";

  heading("traverse_slice_ellipsis");
  const std::vector<ExtractSpec> ellipsis_column{
      pluss::Slicing::Ellipsis(),
      std::int64_t{2}};
  std::cout << grid.traverse_slice(specs_view(ellipsis_column)) << "\n\n";

  heading("traverse_slice_advanced_error");
  try {
    auto row_mask = pluss::make_series(std::vector<bool>{true, false, true}, {3});
    const std::vector<ExtractSpec> advanced_specs{
        row_mask,
        pluss::Slicing::SliceAll()};
    std::cout << grid.traverse_slice(specs_view(advanced_specs)) << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  heading("traverse_slice_reverse_error");
  try {
    const std::vector<ExtractSpec> reverse_specs{
        pluss::Slicing::SliceAll(),
        pluss::Slicing::SliceRange(-1, pluss::Slicing::Slice::kUnbounded, -1)};
    std::cout << grid.traverse_slice(specs_view(reverse_specs)) << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  tracked_grid = pluss::make_series(
      std::vector<double>{10.0, 11.0, 12.0, 13.0, 20.0, 21.0, 22.0, 23.0, 30.0, 31.0, 32.0, 33.0},
      {3, 4});
  tracked_grid.tracks_slope_(true);

  heading("traverse_slice_gradient");
  tracked_grid.traverse_slice(specs_view(row_stride)).sum().reverse();
  std::cout << tracked_grid.sensitivity() << "\n\n";

  heading("block_center");
  std::cout << grid.block(1, 2, 1, 3) << "\n\n";

  heading("block_negative_start");
  std::cout << grid.block(-2, 2, -3, 2) << "\n\n";

  heading("block_zero_height");
  std::cout << grid.block(3, 0, 1, 2) << "\n\n";

  heading("block_rank_error");
  try {
    std::cout << vec.block(0, 1, 0, 1) << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  tracked_grid = pluss::make_series(
      std::vector<double>{10.0, 11.0, 12.0, 13.0, 20.0, 21.0, 22.0, 23.0, 30.0, 31.0, 32.0, 33.0},
      {3, 4});
  tracked_grid.tracks_slope_(true);

  heading("block_gradient");
  tracked_grid.block(1, 2, 1, 3).sum().reverse();
  std::cout << tracked_grid.sensitivity() << "\n\n";

  return 0;
}
