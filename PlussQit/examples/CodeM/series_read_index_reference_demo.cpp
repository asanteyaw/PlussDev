#include <pluss/pluss.h>

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

}  // namespace

int main() {
  auto vec = pluss::make_series({10.0, 20.0, 30.0, 40.0}, {4});
  auto mat = pluss::make_series({
      {1.0, 2.0, 3.0},
      {4.0, 5.0, 6.0},
  });
  auto cube = pluss::make_series({
      {
          {1.0, 2.0},
          {3.0, 4.0},
      },
      {
          {5.0, 6.0},
          {7.0, 8.0},
      },
  });
  auto grid = pluss::make_series({
      {10.0, 11.0, 12.0, 13.0},
      {20.0, 21.0, 22.0, 23.0},
      {30.0, 31.0, 32.0, 33.0},
  });

  heading("pos_1d_last");
  std::cout << vec.pos(-1) << "\n\n";

  heading("pos_2d_negative_col");
  std::cout << mat.pos(1, -1) << "\n\n";

  heading("pos_2d_negative_row");
  std::cout << mat.pos(-1, 0) << "\n\n";

  heading("pos_3d_negative_tail");
  std::cout << cube.pos(1, -1, -1) << "\n\n";

  heading("pos_rank_error");
  try {
    std::cout << vec.pos(0, 0) << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  auto tracked_vec = pluss::make_series({10.0, 20.0, 30.0, 40.0}, {4});
  tracked_vec.tracks_slope_(true);

  heading("pos_gradient");
  tracked_vec.pos(-1).reverse();
  std::cout << tracked_vec.sensitivity() << "\n\n";

  heading("slice_step");
  std::cout << grid.slice(1, 1, 4, 2) << "\n\n";

  heading("slice_dim0");
  std::cout << grid.slice(0, 1, 3) << "\n\n";

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

  heading("slice_zero_step_error");
  try {
    std::cout << grid.slice(1, 0, 4, 0) << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  auto tracked_grid = pluss::make_series({
      {10.0, 11.0, 12.0, 13.0},
      {20.0, 21.0, 22.0, 23.0},
      {30.0, 31.0, 32.0, 33.0},
  });
  tracked_grid.tracks_slope_(true);

  heading("slice_gradient");
  tracked_grid.slice(1, 1, 4, 2).sum().reverse();
  std::cout << tracked_grid.sensitivity() << "\n\n";

  heading("traverse_slice_row_stride");
  std::cout << grid.traverse_slice({
      std::int64_t{1},
      pluss::Slicing::SliceRange(1, 4, 2)}) << "\n\n";

  heading("traverse_slice_newaxis_column");
  std::cout << grid.traverse_slice({
      pluss::Slicing::NewAxis(),
      pluss::Slicing::SliceAll(),
      std::int64_t{2}}) << "\n\n";

  heading("traverse_slice_ellipsis");
  std::cout << grid.traverse_slice({
      pluss::Slicing::Ellipsis(),
      std::int64_t{2}}) << "\n\n";

  heading("traverse_slice_scalar_then_newaxis");
  std::cout << grid.traverse_slice({
      std::int64_t{2},
      pluss::Slicing::NewAxis(),
      pluss::Slicing::SliceRange(0, 3, 1)}) << "\n\n";

  heading("traverse_slice_advanced_error");
  try {
    auto row_mask = pluss::make_series({true, false, true}, {3});
    std::cout << grid.traverse_slice({
        row_mask,
        pluss::Slicing::SliceAll()}) << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  heading("traverse_slice_reverse_error");
  try {
    std::cout << grid.traverse_slice({
        pluss::Slicing::SliceAll(),
        pluss::Slicing::SliceRange(-1, pluss::Slicing::Slice::kUnbounded, -1)}) << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  tracked_grid = pluss::make_series({
      {10.0, 11.0, 12.0, 13.0},
      {20.0, 21.0, 22.0, 23.0},
      {30.0, 31.0, 32.0, 33.0},
  });
  tracked_grid.tracks_slope_(true);

  heading("traverse_slice_gradient");
  tracked_grid.traverse_slice({
      std::int64_t{1},
      pluss::Slicing::SliceRange(1, 4, 2)}).sum().reverse();
  std::cout << tracked_grid.sensitivity() << "\n\n";

  heading("block_center");
  std::cout << grid.block(1, 2, 1, 3) << "\n\n";

  heading("block_negative_start");
  std::cout << grid.block(-2, 2, -3, 2) << "\n\n";

  heading("block_zero_height");
  std::cout << grid.block(3, 0, 1, 2) << "\n\n";

  heading("block_zero_width");
  std::cout << grid.block(1, 2, 4, 0) << "\n\n";

  heading("block_oob_error");
  try {
    std::cout << grid.block(2, 2, 0, 2) << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  heading("block_rank_error");
  try {
    std::cout << vec.block(0, 1, 0, 1) << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  tracked_grid = pluss::make_series({
      {10.0, 11.0, 12.0, 13.0},
      {20.0, 21.0, 22.0, 23.0},
      {30.0, 31.0, 32.0, 33.0},
  });
  tracked_grid.tracks_slope_(true);

  heading("block_gradient");
  tracked_grid.block(1, 2, 1, 3).sum().reverse();
  std::cout << tracked_grid.sensitivity() << "\n\n";

  return 0;
}
