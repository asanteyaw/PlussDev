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
  auto grid = pluss::make_series({
      {10.0, 11.0, 12.0, 13.0},
      {20.0, 21.0, 22.0, 23.0},
      {30.0, 31.0, 32.0, 33.0},
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

  heading("extract_one_spec");
  std::cout << grid.extract(pluss::Slicing::SliceRange(0, 2, 1)) << "\n\n";

  heading("extract_two_specs");
  std::cout << grid.extract(
                   pluss::Slicing::SliceAll(),
                   pluss::Slicing::SliceRange(2, 4, 1))
            << "\n\n";

  heading("extract_three_specs");
  std::cout << grid.extract(
                   pluss::Slicing::NewAxis(),
                   pluss::Slicing::SliceAll(),
                   std::int64_t{1})
            << "\n\n";

  heading("extract_four_specs");
  std::cout << cube.extract(
                   std::int64_t{1},
                   pluss::Slicing::SliceAll(),
                   pluss::Slicing::SliceRange(0, 4, 2),
                   pluss::Slicing::NewAxis())
            << "\n\n";

  heading("extract_ellipsis");
  std::cout << grid.extract(
                   pluss::Slicing::Ellipsis(),
                   std::int64_t{2})
            << "\n\n";

  heading("extract_reverse_cols");
  std::cout << grid.extract({
      pluss::Slicing::SliceAll(),
      pluss::Slicing::SliceRange(-1, pluss::Slicing::Slice::kUnbounded, -1)}) << "\n\n";

  heading("extract_bool_mask");
  auto row_mask = pluss::make_series({true, false, true});
  std::cout << grid.extract({
      row_mask,
      pluss::Slicing::SliceAll()}) << "\n\n";

  heading("extract_shaped_integer_index_flattens");
  auto row_pick_2d = pluss::make_series({
      {2LL, 0LL},
      {2LL, 1LL},
  });
  std::cout << grid.extract({
      row_pick_2d,
      pluss::Slicing::SliceAll()}) << "\n\n";

  heading("extract_zero_step_error");
  try {
    std::cout << grid.extract({
        pluss::Slicing::SliceAll(),
        pluss::Slicing::SliceRange(0, 4, 0)}) << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  heading("index_single_advanced_tensor");
  auto row_pick = pluss::make_series({2LL, 0LL, 2LL});
  std::cout << grid.index({row_pick}) << "\n\n";

  heading("index_advanced_pair_with_sliceall");
  auto row_idx = pluss::make_series({0LL, 1LL, 2LL, 1LL});
  auto col_idx = pluss::make_series({2LL, 3LL, 0LL, 2LL});
  auto adv_cube = pluss::make_series({
      {
          {0.0, 1.0},
          {2.0, 3.0},
          {4.0, 5.0},
          {6.0, 7.0},
      },
      {
          {8.0, 9.0},
          {10.0, 11.0},
          {12.0, 13.0},
          {14.0, 15.0},
      },
      {
          {16.0, 17.0},
          {18.0, 19.0},
          {20.0, 21.0},
          {22.0, 23.0},
      },
  });
  std::cout << adv_cube.index({
      row_idx,
      col_idx,
      pluss::Slicing::SliceAll()}) << "\n\n";

  heading("index_bool_mask_route");
  std::cout << grid.index({
      row_mask,
      pluss::Slicing::SliceAll()}) << "\n\n";

  heading("index_alias_to_extract");
  std::cout << grid.index({
      pluss::Slicing::SliceAll(),
      pluss::Slicing::SliceRange(1, 4, 2)}) << "\n\n";

  heading("index_mixed_nonidentity_falls_back");
  std::cout << adv_cube.index({
      row_idx,
      pluss::Slicing::SliceRange(1, 4, 2),
      pluss::Slicing::SliceAll()}) << "\n\n";

  heading("index_shaped_integer_index_preserves_shape");
  std::cout << grid.index({row_pick_2d}) << "\n\n";

  heading("index_negative_integer_indices");
  auto negative_row_pick = pluss::make_series({-1LL, 0LL});
  std::cout << grid.index({negative_row_pick}) << "\n\n";

  auto take_src = pluss::make_series({
      {10.0, 11.0, 12.0},
      {20.0, 21.0, 22.0},
  });
  auto take_idx = pluss::make_series({
      {5LL, 0LL},
      {3LL, 1LL},
  });

  heading("take_flattened_shape");
  std::cout << take_src.take(take_idx) << "\n\n";

  heading("take_negative_indices");
  auto negative_take_idx = pluss::make_series({-1LL, -3LL, 0LL});
  std::cout << take_src.take(negative_take_idx) << "\n\n";

  heading("take_transposed_logical_order");
  auto transposed = take_src.transpose(0, 1);
  auto transposed_idx = pluss::make_series({
      {0LL, 1LL, 2LL},
      {3LL, 4LL, 5LL},
  });
  std::cout << transposed.take(transposed_idx) << "\n\n";

  heading("take_scalar_shaped_index");
  auto one_idx = pluss::make_series({4LL});
  std::cout << take_src.take(one_idx) << "\n\n";

  heading("take_repeated_gradient");
  auto tracked = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4});
  tracked.tracks_slope_(true);
  auto repeat_idx = pluss::make_series({0LL, 0LL, 3LL});
  auto gathered = tracked.take(repeat_idx);
  gathered.sum().reverse();
  std::cout << "out=" << gathered << "\n";
  std::cout << "dx=" << tracked.sensitivity() << "\n\n";

  heading("take_out_of_range_error");
  try {
    auto bad_pos = pluss::make_series({6LL});
    std::cout << take_src.take(bad_pos) << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  heading("take_non_int64_error");
  try {
    auto bad_idx = pluss::make_series(std::vector<std::int32_t>{0, 1}, {2});
    std::cout << take_src.take(bad_idx) << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  return 0;
}
