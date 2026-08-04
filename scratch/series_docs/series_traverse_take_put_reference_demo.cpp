#include <pluss/pluss.h>

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace {

using pluss::Series;
using pluss::codem::ExtractSpec;
using ExtractSpecView = pluss::msharp::RangeView::Of<ExtractSpec>;
using SeriesView = pluss::msharp::RangeView::Of<Series>;

ExtractSpecView specs_view(const std::vector<ExtractSpec>& specs) {
  return ExtractSpecView(specs);
}

SeriesView index_view(const std::vector<Series>& indices) {
  return SeriesView(indices);
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

  heading("traverse_put_inplace_column");
  auto col_values = pluss::make_series(std::vector<double>{100, 200, 300}, {3});
  const std::vector<ExtractSpec> middle_column{
      pluss::Slicing::SliceAll(),
      std::int64_t{1}};
  grid.traverse_put_(specs_view(middle_column), col_values);
  std::cout << grid << "\n\n";

  heading("traverse_put_inplace_scalar_broadcast");
  const std::vector<ExtractSpec> row_tail{
      std::int64_t{2},
      pluss::Slicing::SliceRange(2, 4, 1)};
  grid.traverse_put_(specs_view(row_tail), pluss::codem::value(-5.0));
  std::cout << grid << "\n\n";

  heading("traverse_put_reverse_error");
  try {
    const std::vector<ExtractSpec> reverse_specs{
        pluss::Slicing::SliceAll(),
        pluss::Slicing::SliceRange(-1, pluss::Slicing::Slice::kUnbounded, -1)};
    grid.traverse_put_(specs_view(reverse_specs), pluss::codem::value(0.0));
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  heading("traverse_put_copy_original");
  std::cout << grid << "\n\n";

  heading("traverse_put_copy_result");
  const std::vector<ExtractSpec> first_two{
      std::int64_t{0},
      pluss::Slicing::SliceRange(0, 2, 1)};
  auto copied = grid.traverse_put(specs_view(first_two), pluss::codem::value(999.0));
  std::cout << copied << "\n\n";

  auto take_src = pluss::make_series(
      std::vector<double>{10, 11, 12, 20, 21, 22},
      {2, 3});
  auto take_idx = pluss::make_series(std::vector<std::int64_t>{5, 0, 3, 1}, {2, 2});

  heading("take_flattened_shape");
  std::cout << take_src.take(take_idx) << "\n\n";

  heading("take_repeated_gradient");
  auto gather_src = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0}, {4});
  gather_src.tracks_slope_(true);
  auto repeat_idx = pluss::make_series(std::vector<std::int64_t>{0, 0, 3}, {3});
  auto gathered = gather_src.take(repeat_idx);
  gathered.sum().reverse();
  std::cout << "out=" << gathered << "\n";
  std::cout << "dx=" << gather_src.sensitivity() << "\n\n";

  auto put_vec = pluss::make_series(std::vector<double>{0, 1, 2, 3, 4, 5}, {6});
  auto put_idx = pluss::make_series(std::vector<std::int64_t>{0, 2, 5}, {3});

  heading("put_scalar_broadcast");
  put_vec.put_(put_idx, pluss::codem::value(-1.0), false);
  std::cout << put_vec << "\n\n";

  heading("put_duplicate_replace");
  auto replace_idx = pluss::make_series(std::vector<std::int64_t>{1, 1, 4}, {3});
  auto replace_values = pluss::make_series(std::vector<double>{50, 60, 90}, {3});
  put_vec.put_(replace_idx, replace_values, false);
  std::cout << put_vec << "\n\n";

  heading("put_duplicate_accumulate");
  auto accum_base = pluss::make_series(std::vector<double>{0, 1, 2, 3, 4, 5}, {6});
  auto accum_values = pluss::make_series(std::vector<double>{10, 20, 30}, {3});
  accum_base.put_(replace_idx, accum_values, true);
  std::cout << accum_base << "\n\n";

  heading("put_mismatch_error");
  try {
    auto bad_values = pluss::make_series(std::vector<double>{1, 2}, {2});
    put_vec.put_(put_idx, bad_values, false);
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  auto base_vec = pluss::make_series(std::vector<double>{0, 1, 2, 3, 4}, {5});
  auto adv_idx = pluss::make_series(std::vector<std::int64_t>{1, 3}, {2});
  auto adv_values = pluss::make_series(std::vector<double>{90, 70}, {2});

  heading("index_put_copy_result");
  auto copy_result = base_vec.index_put({adv_idx}, adv_values, false);
  std::cout << copy_result << "\n\n";

  heading("index_put_copy_original");
  std::cout << base_vec << "\n\n";

  heading("index_put_copy_accumulate");
  auto dup_idx = pluss::make_series(std::vector<std::int64_t>{1, 1, 3}, {3});
  auto dup_values = pluss::make_series(std::vector<double>{10, 20, 30}, {3});
  std::vector<Series> dup_indices{dup_idx};
  std::cout << base_vec.index_put(index_view(dup_indices), dup_values, true) << "\n\n";

  heading("index_put_scalar_copy");
  std::cout << base_vec.index_put(2, pluss::codem::value(555.0)) << "\n\n";

  return 0;
}
