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
  auto base = pluss::make_series(
      std::vector<double>{10, 11, 12, 13, 20, 21, 22, 23, 30, 31, 32, 33},
      {3, 4});

  auto grid = base.clone();
  const std::vector<ExtractSpec> fill_column{
      pluss::Slicing::SliceAll(),
      std::int64_t{1}};
  heading("extract_put_inplace_scalar_broadcast");
  grid.extract_put_(specs_view(fill_column), pluss::codem::value(-1.0));
  std::cout << grid << "\n\n";

  const std::vector<ExtractSpec> reverse_write{
      std::int64_t{1},
      pluss::Slicing::SliceRange(-1, pluss::Slicing::Slice::kUnbounded, -2)};
  auto reverse_values = pluss::make_series(std::vector<double>{90, 70}, {2});
  heading("extract_put_inplace_reverse_slice");
  grid.extract_put_(specs_view(reverse_write), reverse_values);
  std::cout << grid << "\n\n";

  const std::vector<ExtractSpec> with_newaxis{
      pluss::Slicing::NewAxis(),
      pluss::Slicing::SliceAll(),
      std::int64_t{0}};
  auto first_col = pluss::make_series(std::vector<double>{100, 200, 300}, {3});
  heading("extract_put_inplace_newaxis_ignored");
  grid.extract_put_(specs_view(with_newaxis), first_col);
  std::cout << grid << "\n\n";

  heading("extract_put_inplace_mismatch_error");
  try {
    const std::vector<ExtractSpec> mismatch_specs{
        std::int64_t{0},
        pluss::Slicing::SliceRange(0, 4, 2)};
    auto mismatch_values = pluss::make_series(std::vector<double>{1, 2, 3}, {3});
    grid.extract_put_(specs_view(mismatch_specs), mismatch_values);
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  heading("extract_put_copy_original");
  std::cout << grid << "\n\n";

  const std::vector<ExtractSpec> right_two{
      pluss::Slicing::SliceAll(),
      pluss::Slicing::SliceRange(2, 4, 1)};
  heading("extract_put_copy_result");
  auto patched = grid.extract_put(specs_view(right_two), pluss::codem::value(5.0));
  std::cout << patched << "\n\n";

  auto row_mask = pluss::make_series(std::vector<bool>{true, false, true}, {3});
  const std::vector<ExtractSpec> mask_specs{
      row_mask,
      pluss::Slicing::SliceAll()};
  heading("extract_put_copy_mask_rows");
  std::cout << grid.extract_put(specs_view(mask_specs), pluss::codem::value(0.0)) << "\n\n";

  auto traverse_grid = base.clone();
  auto col_values = pluss::make_series(std::vector<double>{100, 200, 300}, {3});
  const std::vector<ExtractSpec> middle_column{
      pluss::Slicing::SliceAll(),
      std::int64_t{1}};
  heading("traverse_put_inplace_column");
  traverse_grid.traverse_put_(specs_view(middle_column), col_values);
  std::cout << traverse_grid << "\n\n";

  const std::vector<ExtractSpec> row_tail{
      std::int64_t{2},
      pluss::Slicing::SliceRange(2, 4, 1)};
  heading("traverse_put_inplace_scalar_broadcast");
  traverse_grid.traverse_put_(specs_view(row_tail), pluss::codem::value(-5.0));
  std::cout << traverse_grid << "\n\n";

  heading("traverse_put_inplace_reverse_error");
  try {
    const std::vector<ExtractSpec> reverse_specs{
        pluss::Slicing::SliceAll(),
        pluss::Slicing::SliceRange(-1, pluss::Slicing::Slice::kUnbounded, -1)};
    traverse_grid.traverse_put_(specs_view(reverse_specs), pluss::codem::value(0.0));
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  heading("traverse_put_copy_original");
  std::cout << traverse_grid << "\n\n";

  const std::vector<ExtractSpec> first_two{
      std::int64_t{0},
      pluss::Slicing::SliceRange(0, 2, 1)};
  heading("traverse_put_copy_result");
  auto copied = traverse_grid.traverse_put(specs_view(first_two), pluss::codem::value(999.0));
  std::cout << copied << "\n\n";

  heading("traverse_put_copy_original_after_call");
  std::cout << traverse_grid << "\n\n";

  return 0;
}
