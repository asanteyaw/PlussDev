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
      {10.0, 11.0, 12.0, 13.0, 20.0, 21.0, 22.0, 23.0, 30.0, 31.0, 32.0, 33.0},
      {3, 4});

  auto grid = base.clone();
  const std::vector<ExtractSpec> fill_column{
      pluss::Slicing::SliceAll(),
      std::int64_t{1}};
  heading("extract_put_inplace_setup");
  std::cout << grid << "\n\n";

  heading("extract_put_inplace_scalar_broadcast");
  grid.extract_put_(specs_view(fill_column), pluss::make_series(-1.0));
  std::cout << grid << "\n\n";

  const std::vector<ExtractSpec> reverse_write{
      std::int64_t{1},
      pluss::Slicing::SliceRange(-1, pluss::Slicing::Slice::kUnbounded, -2)};
  heading("extract_put_inplace_reverse_slice");
  grid.extract_put_(specs_view(reverse_write), pluss::make_series({90.0, 70.0}, {2}));
  std::cout << grid << "\n\n";

  const std::vector<ExtractSpec> with_newaxis{
      pluss::Slicing::NewAxis(),
      pluss::Slicing::SliceAll(),
      std::int64_t{0}};
  heading("extract_put_inplace_newaxis_ignored");
  grid.extract_put_(specs_view(with_newaxis), pluss::make_series({100.0, 200.0, 300.0}, {3}));
  std::cout << grid << "\n\n";

  auto row_mask = pluss::make_series({true, false, true}, {3});
  const std::vector<ExtractSpec> mask_specs{
      row_mask,
      pluss::Slicing::SliceAll()};
  heading("extract_put_inplace_mask_rows");
  grid.extract_put_(specs_view(mask_specs), pluss::make_series(0.0));
  std::cout << grid << "\n\n";

  heading("extract_put_inplace_mismatch_error");
  try {
    const std::vector<ExtractSpec> mismatch_specs{
        std::int64_t{0},
        pluss::Slicing::SliceRange(0, 4, 2)};
    grid.extract_put_(specs_view(mismatch_specs), pluss::make_series({1.0, 2.0, 3.0}, {3}));
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  auto copy_grid = pluss::make_series(
      {100.0, -1.0, 12.0, 13.0, 200.0, 70.0, 22.0, 90.0, 300.0, -1.0, 32.0, 33.0},
      {3, 4});
  heading("extract_put_copy_original");
  std::cout << copy_grid << "\n\n";

  const std::vector<ExtractSpec> right_two{
      pluss::Slicing::SliceAll(),
      pluss::Slicing::SliceRange(2, 4, 1)};
  heading("extract_put_copy_result");
  auto patched = copy_grid.extract_put(specs_view(right_two), pluss::make_series(5.0));
  std::cout << patched << "\n\n";

  heading("extract_put_copy_original_after_call");
  std::cout << copy_grid << "\n\n";

  heading("extract_put_copy_mask_rows");
  std::cout << copy_grid.extract_put(specs_view(mask_specs), pluss::make_series(0.0)) << "\n\n";

  heading("extract_put_copy_reverse_slice");
  std::cout << copy_grid.extract_put(
      specs_view(reverse_write),
      pluss::make_series({900.0, 700.0}, {2})) << "\n\n";

  auto traverse_grid = base.clone();
  heading("traverse_put_inplace_setup");
  std::cout << traverse_grid << "\n\n";

  const std::vector<ExtractSpec> middle_column{
      pluss::Slicing::SliceAll(),
      std::int64_t{1}};
  heading("traverse_put_inplace_column");
  traverse_grid.traverse_put_(specs_view(middle_column), pluss::make_series({100.0, 200.0, 300.0}, {3}));
  std::cout << traverse_grid << "\n\n";

  heading("traverse_put_inplace_newaxis_ignored");
  traverse_grid.traverse_put_(
      specs_view(with_newaxis),
      pluss::make_series({1.0, 2.0, 3.0}, {3}));
  std::cout << traverse_grid << "\n\n";

  const std::vector<ExtractSpec> row_tail{
      std::int64_t{2},
      pluss::Slicing::SliceRange(2, 4, 1)};
  heading("traverse_put_inplace_scalar_broadcast");
  traverse_grid.traverse_put_(specs_view(row_tail), pluss::make_series(-5.0));
  std::cout << traverse_grid << "\n\n";

  heading("traverse_put_inplace_reverse_error");
  try {
    const std::vector<ExtractSpec> reverse_specs{
        pluss::Slicing::SliceAll(),
        pluss::Slicing::SliceRange(-1, pluss::Slicing::Slice::kUnbounded, -1)};
    traverse_grid.traverse_put_(specs_view(reverse_specs), pluss::make_series(0.0));
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  heading("traverse_put_inplace_mask_error");
  try {
    traverse_grid.traverse_put_(specs_view(mask_specs), pluss::make_series(0.0));
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
  auto copied = traverse_grid.traverse_put(specs_view(first_two), pluss::make_series(999.0));
  std::cout << copied << "\n\n";

  heading("traverse_put_copy_newaxis");
  std::cout << traverse_grid.traverse_put(
      specs_view(with_newaxis),
      pluss::make_series({10.0, 20.0, 30.0}, {3})) << "\n\n";

  heading("traverse_put_copy_reverse_error");
  try {
    const std::vector<ExtractSpec> reverse_specs{
        pluss::Slicing::SliceAll(),
        pluss::Slicing::SliceRange(-1, pluss::Slicing::Slice::kUnbounded, -1)};
    std::cout << traverse_grid.traverse_put(specs_view(reverse_specs), pluss::make_series(0.0)) << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  heading("traverse_put_copy_mask_error");
  try {
    std::cout << traverse_grid.traverse_put(specs_view(mask_specs), pluss::make_series(0.0)) << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  heading("traverse_put_copy_original_after_call");
  std::cout << traverse_grid << "\n\n";

  return 0;
}
