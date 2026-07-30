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

void print_sizes(std::span<const std::int64_t> sizes) {
  std::cout << "[";
  for (std::size_t i = 0; i < sizes.size(); ++i) {
    if (i != 0) {
      std::cout << ", ";
    }
    std::cout << sizes[i];
  }
  std::cout << "]";
}

void print_error(const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}

}  // namespace

int main() {
  auto shape_src = pluss::make_series(
      std::vector<double>{
          1, 2, 3, 4, 5, 6,
          7, 8, 9, 10, 11, 12,
          13, 14, 15, 16, 17, 18,
          19, 20, 21, 22, 23, 24},
      {2, 3, 4});

  heading("size_dim0");
  std::cout << shape_src.size(0) << "\n\n";

  heading("size_negative_last");
  std::cout << shape_src.size(-1) << "\n\n";

  heading("sizes_base");
  print_sizes(shape_src.sizes());
  std::cout << "\n\n";

  heading("sizes_transposed");
  print_sizes(shape_src.transpose(0, 1).sizes());
  std::cout << "\n\n";

  auto grid = pluss::make_series(
      std::vector<double>{10, 11, 12, 13, 20, 21, 22, 23, 30, 31, 32, 33},
      {3, 4});

  heading("extract_put_scalar_broadcast");
  const std::vector<ExtractSpec> fill_column{
      pluss::Slicing::SliceAll(),
      std::int64_t{1}};
  grid.extract_put_(specs_view(fill_column), pluss::codem::value(-1.0));
  std::cout << grid << "\n\n";

  heading("extract_put_reverse_slice");
  const std::vector<ExtractSpec> reverse_write{
      std::int64_t{1},
      pluss::Slicing::SliceRange(-1, pluss::Slicing::Slice::kUnbounded, -2)};
  auto reverse_values = pluss::make_series(std::vector<double>{90, 70}, {2});
  grid.extract_put_(specs_view(reverse_write), reverse_values);
  std::cout << grid << "\n\n";

  heading("extract_put_newaxis_ignored");
  const std::vector<ExtractSpec> with_newaxis{
      pluss::Slicing::NewAxis(),
      pluss::Slicing::SliceAll(),
      std::int64_t{0}};
  auto first_col = pluss::make_series(std::vector<double>{100, 200, 300}, {3});
  grid.extract_put_(specs_view(with_newaxis), first_col);
  std::cout << grid << "\n\n";

  heading("extract_put_mismatch_error");
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

  heading("extract_put_copy_result");
  const std::vector<ExtractSpec> right_two{
      pluss::Slicing::SliceAll(),
      pluss::Slicing::SliceRange(2, 4, 1)};
  auto patched = grid.extract_put(specs_view(right_two), pluss::codem::value(5.0));
  std::cout << patched << "\n\n";

  heading("extract_put_copy_mask_rows");
  auto row_mask = pluss::make_series(std::vector<bool>{true, false, true}, {3});
  const std::vector<ExtractSpec> mask_specs{
      row_mask,
      pluss::Slicing::SliceAll()};
  std::cout << grid.extract_put(specs_view(mask_specs), pluss::codem::value(0.0)) << "\n\n";

  auto vec = pluss::make_series(std::vector<double>{0, 1, 2, 3, 4}, {5});

  heading("index_put_series_list");
  auto idx = pluss::make_series(std::vector<std::int64_t>{1, 3}, {2});
  auto picked = pluss::make_series(std::vector<double>{90, 70}, {2});
  vec.index_put_({idx}, picked, false);
  std::cout << vec << "\n\n";

  heading("index_put_accumulate");
  auto dup_idx = pluss::make_series(std::vector<std::int64_t>{1, 1, 3}, {3});
  auto dup_values = pluss::make_series(std::vector<double>{10, 20, 30}, {3});
  vec.index_put_(index_view(std::vector<Series>{dup_idx}), dup_values, true);
  std::cout << vec << "\n\n";

  heading("index_put_scalar_convenience");
  vec.index_put_(0, pluss::codem::value(-5.0));
  std::cout << vec << "\n\n";

  heading("index_put_spec_1d");
  vec.index_put_({std::int64_t{2}}, pluss::codem::value(555.0));
  std::cout << vec << "\n\n";

  heading("index_put_spec_2d");
  auto mat = pluss::make_series(
      std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8, 9},
      {3, 3});
  auto col_values = pluss::make_series(std::vector<double>{30, 60, 90}, {3});
  mat.index_put_({pluss::Slicing::SliceAll(), std::int64_t{2}}, col_values);
  std::cout << mat << "\n\n";

  heading("index_put_spec_accumulate_error");
  try {
    mat.index_put_({pluss::Slicing::SliceAll(), std::int64_t{1}}, col_values, true);
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  return 0;
}
