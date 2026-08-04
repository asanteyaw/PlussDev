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
  auto grid = pluss::make_series(
      std::vector<double>{10, 11, 12, 13, 20, 21, 22, 23, 30, 31, 32, 33},
      {3, 4});
  auto cube = pluss::make_series(
      std::vector<double>{
          1, 2, 3, 4, 5, 6,
          7, 8, 9, 10, 11, 12,
          13, 14, 15, 16, 17, 18,
          19, 20, 21, 22, 23, 24},
      {2, 3, 4});

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

  heading("extract_reverse_cols");
  const std::vector<ExtractSpec> reverse_cols{
      pluss::Slicing::SliceAll(),
      pluss::Slicing::SliceRange(-1, pluss::Slicing::Slice::kUnbounded, -1)};
  std::cout << grid.extract(specs_view(reverse_cols)) << "\n\n";

  heading("extract_bool_mask");
  auto row_mask = pluss::make_series(std::vector<bool>{true, false, true}, {3});
  const std::vector<ExtractSpec> bool_pick{
      row_mask,
      pluss::Slicing::SliceAll()};
  std::cout << grid.extract(specs_view(bool_pick)) << "\n\n";

  heading("extract_shaped_integer_index_flattens");
  auto row_pick_2d = pluss::make_series(std::vector<std::int64_t>{2, 0, 2, 1}, {2, 2});
  const std::vector<ExtractSpec> shaped_int_pick{
      row_pick_2d,
      pluss::Slicing::SliceAll()};
  std::cout << grid.extract(specs_view(shaped_int_pick)) << "\n\n";

  heading("index_single_advanced_tensor");
  auto row_pick = pluss::make_series(std::vector<std::int64_t>{2, 0, 2}, {3});
  const std::vector<ExtractSpec> single_adv{row_pick};
  std::cout << grid.index(specs_view(single_adv)) << "\n\n";

  heading("index_advanced_pair_with_sliceall");
  auto row_idx = pluss::make_series(std::vector<std::int64_t>{0, 1, 2, 1}, {4});
  auto col_idx = pluss::make_series(std::vector<std::int64_t>{2, 3, 0, 2}, {4});
  auto adv_cube = pluss::make_series(
      std::vector<double>{
          0, 1, 2, 3, 4, 5, 6, 7,
          8, 9, 10, 11, 12, 13, 14, 15,
          16, 17, 18, 19, 20, 21, 22, 23},
      {3, 4, 2});
  const std::vector<ExtractSpec> pair_specs{
      row_idx,
      col_idx,
      pluss::Slicing::SliceAll()};
  std::cout << adv_cube.index(specs_view(pair_specs)) << "\n\n";

  heading("index_bool_mask_route");
  const std::vector<ExtractSpec> bool_index_specs{
      row_mask,
      pluss::Slicing::SliceAll()};
  std::cout << grid.index(specs_view(bool_index_specs)) << "\n\n";

  heading("index_alias_to_extract");
  const std::vector<ExtractSpec> alias_specs{
      pluss::Slicing::SliceAll(),
      pluss::Slicing::SliceRange(1, 4, 2)};
  std::cout << grid.index(specs_view(alias_specs)) << "\n\n";

  heading("index_mixed_nonidentity_falls_back");
  const std::vector<ExtractSpec> mixed_specs{
      row_idx,
      pluss::Slicing::SliceRange(1, 4, 2),
      pluss::Slicing::SliceAll()};
  std::cout << adv_cube.index(specs_view(mixed_specs)) << "\n\n";

  heading("index_shaped_integer_index_preserves_shape");
  const std::vector<ExtractSpec> shaped_index_specs{row_pick_2d};
  std::cout << grid.index(specs_view(shaped_index_specs)) << "\n\n";

  auto take_src = pluss::make_series(
      std::vector<double>{10, 11, 12, 20, 21, 22},
      {2, 3});
  auto take_idx = pluss::make_series(std::vector<std::int64_t>{5, 0, 3, 1}, {2, 2});

  heading("take_flattened_shape");
  std::cout << take_src.take(take_idx) << "\n\n";

  heading("take_negative_indices");
  auto negative_take_idx = pluss::make_series(std::vector<std::int64_t>{-1, -3, 0}, {3});
  std::cout << take_src.take(negative_take_idx) << "\n\n";

  heading("take_transposed_logical_order");
  auto transposed = take_src.transpose(0, 1);
  auto transposed_idx = pluss::make_series(std::vector<std::int64_t>{0, 1, 2, 3, 4, 5}, {2, 3});
  std::cout << transposed.take(transposed_idx) << "\n\n";

  heading("take_repeated_gradient");
  auto tracked = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0}, {4});
  tracked.tracks_slope_(true);
  auto repeat_idx = pluss::make_series(std::vector<std::int64_t>{0, 0, 3}, {3});
  auto gathered = tracked.take(repeat_idx);
  gathered.sum().reverse();
  std::cout << "out=" << gathered << "\n";
  std::cout << "dx=" << tracked.sensitivity() << "\n\n";

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
