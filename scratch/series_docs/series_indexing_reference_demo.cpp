#include <pluss/pluss.h>

#include <array>
#include <cstdint>
#include <iostream>
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

}  // namespace

int main() {
  auto vec = pluss::make_series(std::vector<double>{10, 20, 30, 40}, {4});
  auto mat = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6}, {2, 3});
  auto cube = pluss::make_series(
      std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8},
      {2, 2, 2});

  heading("pos_1d_last");
  std::cout << vec.pos(-1) << "\n\n";

  heading("pos_2d_negative_col");
  std::cout << mat.pos(1, -1) << "\n\n";

  heading("pos_3d_negative_tail");
  std::cout << cube.pos(1, -1, -1) << "\n\n";

  heading("view_3x2");
  const std::array<std::int64_t, 2> shape_3x2{{3, 2}};
  std::cout << mat.view(std::span<const std::int64_t>(shape_3x2.data(), shape_3x2.size()))
            << "\n\n";

  heading("view_transposed_error");
  try {
    auto transposed = mat.transpose(0, 1);
    std::cout << transposed.view(std::span<const std::int64_t>(shape_3x2.data(), shape_3x2.size()))
              << "\n";
  } catch (const std::exception& e) {
    std::string msg = e.what();
    std::cout << msg.substr(0, msg.find('\n')) << "\n";
  }
  std::cout << "\n";

  heading("reshape_3x2");
  std::cout << mat.reshape(std::span<const std::int64_t>(shape_3x2.data(), shape_3x2.size()))
            << "\n\n";

  heading("reshape_transposed_copy");
  std::cout << mat.transpose(0, 1)
                   .reshape(std::span<const std::int64_t>(shape_3x2.data(), shape_3x2.size()))
            << "\n\n";

  heading("slice_every_other_col");
  auto slice_src = pluss::make_series(
      std::vector<double>{10, 11, 12, 13, 20, 21, 22, 23, 30, 31, 32, 33},
      {3, 4});
  std::cout << slice_src.slice(1, 1, 4, 2) << "\n\n";

  heading("block_center");
  auto block_src = pluss::make_series(
      std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12},
      {3, 4});
  std::cout << block_src.block(1, 2, 1, 3) << "\n\n";

  heading("extract_slice");
  const std::vector<ExtractSpec> right_two{
      pluss::Slicing::SliceAll(),
      pluss::Slicing::SliceRange(2, 4, 1)};
  std::cout << slice_src.extract(specs_view(right_two)) << "\n\n";

  heading("extract_negative_step");
  const std::vector<ExtractSpec> reverse_cols{
      pluss::Slicing::SliceAll(),
      pluss::Slicing::SliceRange(-1, pluss::Slicing::Slice::kUnbounded, -1)};
  std::cout << slice_src.extract(specs_view(reverse_cols)) << "\n\n";

  heading("extract_newaxis_and_scalar");
  const std::vector<ExtractSpec> with_newaxis{
      pluss::Slicing::NewAxis(),
      pluss::Slicing::SliceAll(),
      std::int64_t{1}};
  std::cout << slice_src.extract(specs_view(with_newaxis)) << "\n\n";

  heading("extract_bool_advanced");
  auto mask = pluss::make_series(std::vector<bool>{true, false, true}, {3});
  const std::vector<ExtractSpec> bool_pick{
      mask,
      pluss::Slicing::SliceAll()};
  std::cout << slice_src.extract(specs_view(bool_pick)) << "\n\n";

  heading("index_advanced_pair");
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

  heading("index_slice_alias");
  const std::vector<ExtractSpec> alias_specs{
      pluss::Slicing::SliceAll(),
      pluss::Slicing::SliceRange(1, 4, 2)};
  std::cout << slice_src.index(specs_view(alias_specs)) << "\n\n";

  heading("index_mixed_nonidentity_falls_back");
  const std::vector<ExtractSpec> mixed_specs{
      row_idx,
      pluss::Slicing::SliceRange(1, 4, 2),
      pluss::Slicing::SliceAll()};
  std::cout << adv_cube.index(specs_view(mixed_specs)) << "\n\n";

  return 0;
}
