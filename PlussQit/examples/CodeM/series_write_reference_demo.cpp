#include <pluss/pluss.h>

#include <cstdint>
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
  auto put_idx = pluss::make_series(std::vector<std::int64_t>{0, 2, 5}, {3});
  auto replace_idx = pluss::make_series(std::vector<std::int64_t>{1, 1, 4}, {3});

  auto flat = pluss::make_series(std::vector<double>{0, 1, 2, 3, 4, 5}, {6});
  heading("put_scalar_broadcast");
  flat.put_(put_idx, pluss::codem::value(-1.0), false);
  std::cout << flat << "\n\n";

  auto negative_idx = pluss::make_series(std::vector<std::int64_t>{-1, -3}, {2});
  auto negative_values = pluss::make_series(std::vector<double>{99, 77}, {2});
  heading("put_negative_indices");
  flat.put_(negative_idx, negative_values, false);
  std::cout << flat << "\n\n";

  auto replace_base = pluss::make_series(std::vector<double>{0, 1, 2, 3, 4, 5}, {6});
  auto replace_values = pluss::make_series(std::vector<double>{50, 60, 90}, {3});
  heading("put_duplicate_replace");
  replace_base.put_(replace_idx, replace_values, false);
  std::cout << replace_base << "\n\n";

  auto accum_base = pluss::make_series(std::vector<double>{0, 1, 2, 3, 4, 5}, {6});
  auto accum_values = pluss::make_series(std::vector<double>{10, 20, 30}, {3});
  heading("put_duplicate_accumulate");
  accum_base.put_(replace_idx, accum_values, true);
  std::cout << accum_base << "\n\n";

  heading("put_mismatch_error");
  try {
    auto bad_values = pluss::make_series(std::vector<double>{1, 2}, {2});
    flat.put_(put_idx, bad_values, false);
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  auto vec = pluss::make_series(std::vector<double>{0, 1, 2, 3, 4}, {5});
  auto idx = pluss::make_series(std::vector<std::int64_t>{1, 3}, {2});
  auto picked = pluss::make_series(std::vector<double>{90, 70}, {2});
  heading("index_put_copy_basic");
  std::cout << vec.index_put({idx}, picked, false) << "\n\n";

  heading("index_put_copy_original_unchanged");
  std::cout << vec << "\n\n";

  auto overlap_idx = pluss::make_series(std::vector<std::int64_t>{1, 1, 3}, {3});
  auto overlap_values = pluss::make_series(std::vector<double>{10, 20, 30}, {3});
  std::vector<pluss::Series> overlap_indices{overlap_idx};
  heading("index_put_copy_accumulate_rangeview");
  std::cout << vec.index_put(
                   pluss::msharp::RangeView::Of<pluss::Series>(overlap_indices),
                   overlap_values,
                   true)
            << "\n\n";

  auto mask = pluss::make_series(std::vector<bool>{false, true, false, true, false}, {5});
  heading("index_put_copy_bool_mask_scalar");
  std::cout << vec.index_put({mask}, pluss::codem::value(-1.0), false) << "\n\n";

  auto mat = pluss::make_series(
      std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8, 9},
      {3, 3});
  auto row_idx = pluss::make_series(std::vector<std::int64_t>{0, 2}, {2});
  auto tail_pattern = pluss::make_series(std::vector<double>{100, 200, 300}, {3});
  heading("index_put_copy_tail_pattern");
  std::cout << mat.index_put({row_idx}, tail_pattern, false) << "\n\n";

  heading("index_put_copy_scalar_negative_index");
  std::cout << vec.index_put(-1, pluss::codem::value(555.0)) << "\n\n";

  heading("index_put_copy_overlap_error");
  try {
    std::cout << vec.index_put({overlap_idx}, overlap_values, false) << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  auto inplace = pluss::make_series(std::vector<double>{0, 1, 2, 3, 4}, {5});
  heading("index_put_inplace_basic");
  inplace.index_put_({idx}, picked, false);
  std::cout << inplace << "\n\n";

  auto mask_inplace = pluss::make_series(std::vector<double>{0, 1, 2, 3, 4}, {5});
  heading("index_put_inplace_bool_mask_scalar");
  mask_inplace.index_put_({mask}, pluss::codem::value(-1.0), false);
  std::cout << mask_inplace << "\n\n";

  auto accum_inplace = pluss::make_series(std::vector<double>{0, 1, 2, 3, 4}, {5});
  heading("index_put_inplace_accumulate_rangeview");
  accum_inplace.index_put_(
      pluss::msharp::RangeView::Of<pluss::Series>(overlap_indices),
      overlap_values,
      true);
  std::cout << accum_inplace << "\n\n";

  heading("index_put_inplace_scalar_negative_index");
  accum_inplace.index_put_(-1, pluss::codem::value(999.0));
  std::cout << accum_inplace << "\n\n";

  heading("index_put_inplace_extractspec_1d");
  accum_inplace.index_put_({std::int64_t{0}}, pluss::codem::value(123.0));
  std::cout << accum_inplace << "\n\n";

  auto column_base = pluss::make_series(
      std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8, 9},
      {3, 3});
  auto column_values = pluss::make_series(std::vector<double>{30, 60, 90}, {3});
  heading("index_put_inplace_extractspec_column");
  column_base.index_put_({pluss::Slicing::SliceAll(), std::int64_t{2}}, column_values);
  std::cout << column_base << "\n\n";

  heading("index_put_inplace_extractspec_accumulate_error");
  try {
    column_base.index_put_({pluss::Slicing::SliceAll(), std::int64_t{1}}, column_values, true);
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  heading("index_put_inplace_overlap_error");
  try {
    auto overlap_inplace = pluss::make_series(std::vector<double>{0, 1, 2, 3, 4}, {5});
    overlap_inplace.index_put_({overlap_idx}, overlap_values, false);
    std::cout << overlap_inplace << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  return 0;
}
