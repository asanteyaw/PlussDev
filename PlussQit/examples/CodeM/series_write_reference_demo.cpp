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
  auto put_idx = pluss::make_series({0LL, 2LL, 5LL});
  auto replace_idx = pluss::make_series({1LL, 1LL, 4LL});

  auto flat = pluss::make_series({0.0, 1.0, 2.0, 3.0, 4.0, 5.0});
  heading("put_scalar_broadcast");
  flat.put_(put_idx, pluss::make_series(-1.0), false);
  std::cout << flat << "\n\n";

  auto negative_idx = pluss::make_series({-1LL, -3LL});
  auto negative_values = pluss::make_series({99.0, 77.0});
  heading("put_negative_indices");
  flat.put_(negative_idx, negative_values, false);
  std::cout << flat << "\n\n";

  auto replace_base = pluss::make_series({0.0, 1.0, 2.0, 3.0, 4.0, 5.0});
  auto replace_values = pluss::make_series({50.0, 60.0, 90.0});
  heading("put_duplicate_replace");
  replace_base.put_(replace_idx, replace_values, false);
  std::cout << replace_base << "\n\n";

  auto accum_base = pluss::make_series({0.0, 1.0, 2.0, 3.0, 4.0, 5.0});
  auto accum_values = pluss::make_series({10.0, 20.0, 30.0});
  heading("put_duplicate_accumulate");
  accum_base.put_(replace_idx, accum_values, true);
  std::cout << accum_base << "\n\n";

  auto transposed_base = pluss::make_series({
      {10.0, 11.0, 12.0},
      {20.0, 21.0, 22.0},
  }).transpose(0, 1);
  heading("put_transposed_logical_order");
  transposed_base.put_(pluss::make_series({0LL, 3LL, 5LL}), pluss::make_series({100.0, 300.0, 500.0}), false);
  std::cout << transposed_base << "\n\n";

  heading("put_out_of_range_error");
  try {
    auto bad_pos = pluss::make_series({6LL});
    flat.put_(bad_pos, pluss::make_series(0.0), false);
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  heading("put_non_int64_error");
  try {
    auto bad_idx = pluss::make_series(std::vector<std::int32_t>{0, 1}, {2});
    flat.put_(bad_idx, pluss::make_series(0.0), false);
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  heading("put_mismatch_error");
  try {
    auto bad_values = pluss::make_series({1.0, 2.0});
    flat.put_(put_idx, bad_values, false);
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  auto vec = pluss::make_series({0.0, 1.0, 2.0, 3.0, 4.0});
  auto idx = pluss::make_series({1LL, 3LL});
  auto picked = pluss::make_series({90.0, 70.0});
  heading("index_put_copy_basic");
  std::cout << vec.index_put({idx}, picked, false) << "\n\n";

  heading("index_put_copy_original_unchanged");
  std::cout << vec << "\n\n";

  auto overlap_idx = pluss::make_series({1LL, 1LL, 3LL});
  auto overlap_values = pluss::make_series({10.0, 20.0, 30.0});
  std::vector<pluss::Series> overlap_indices{overlap_idx};
  heading("index_put_copy_accumulate_rangeview");
  std::cout << vec.index_put(overlap_indices, overlap_values, true) << "\n\n";

  auto mask = pluss::make_series({false, true, false, true, false});
  heading("index_put_copy_bool_mask_scalar");
  std::cout << vec.index_put({mask}, pluss::make_series(-1.0), false) << "\n\n";

  auto mat = pluss::make_series(
      {{1.0, 2.0, 3.0},
       {4.0, 5.0, 6.0},
       {7.0, 8.0, 9.0}});
  auto row_idx = pluss::make_series({0LL, 2LL});
  auto tail_pattern = pluss::make_series({100.0, 200.0, 300.0});
  heading("index_put_copy_tail_pattern");
  std::cout << mat.index_put({row_idx}, tail_pattern, false) << "\n\n";

  auto plane = pluss::make_series({
      {10.0, 11.0, 12.0, 13.0},
      {20.0, 21.0, 22.0, 23.0},
      {30.0, 31.0, 32.0, 33.0},
  });
  auto adv_row_idx = pluss::make_series({0LL, 2LL});
  auto adv_col_idx = pluss::make_series({1LL, 3LL});
  auto adv_values = pluss::make_series({500.0, 700.0});
  heading("index_put_copy_advanced_pair");
  std::cout << plane.index_put({adv_row_idx, adv_col_idx}, adv_values, false) << "\n\n";

  heading("index_put_copy_scalar_negative_index");
  std::cout << vec.index_put(-1, pluss::make_series(555.0)) << "\n\n";

  heading("index_put_copy_overlap_last_write");
  std::cout << vec.index_put({overlap_idx}, overlap_values, false) << "\n\n";

  heading("index_put_copy_out_of_range_error");
  try {
    auto bad_idx = pluss::make_series({5LL});
    std::cout << vec.index_put({bad_idx}, pluss::make_series(10.0), false) << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  heading("index_put_copy_int32_indices");
  auto int32_idx_copy = pluss::make_series(std::vector<std::int32_t>{1, 3}, {2});
  std::cout << vec.index_put({int32_idx_copy}, picked, false) << "\n\n";

  auto inplace = pluss::make_series({0.0, 1.0, 2.0, 3.0, 4.0});
  heading("index_put_inplace_basic");
  inplace.index_put_({idx}, picked, false);
  std::cout << inplace << "\n\n";

  auto mask_inplace = pluss::make_series({0.0, 1.0, 2.0, 3.0, 4.0});
  heading("index_put_inplace_bool_mask_scalar");
  mask_inplace.index_put_({mask}, pluss::make_series(-1.0), false);
  std::cout << mask_inplace << "\n\n";

  auto accum_inplace = pluss::make_series({0.0, 1.0, 2.0, 3.0, 4.0});
  heading("index_put_inplace_accumulate_rangeview");
  accum_inplace.index_put_(overlap_indices, overlap_values, true);
  std::cout << accum_inplace << "\n\n";

  heading("index_put_inplace_scalar_negative_index");
  accum_inplace.index_put_(-1, pluss::make_series(999.0));
  std::cout << accum_inplace << "\n\n";

  heading("index_put_inplace_extractspec_1d");
  accum_inplace.index_put_({0LL}, pluss::make_series(123.0));
  std::cout << accum_inplace << "\n\n";

  auto column_base = pluss::make_series(
      {{1.0, 2.0, 3.0},
       {4.0, 5.0, 6.0},
       {7.0, 8.0, 9.0}});
  auto column_values = pluss::make_series({30.0, 60.0, 90.0});
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

  heading("index_put_inplace_overlap_last_write");
  auto overlap_inplace = pluss::make_series({0.0, 1.0, 2.0, 3.0, 4.0});
  overlap_inplace.index_put_({overlap_idx}, overlap_values, false);
  std::cout << overlap_inplace << "\n\n";

  heading("index_put_inplace_advanced_pair");
  auto inplace_plane = pluss::make_series({
      {10.0, 11.0, 12.0, 13.0},
      {20.0, 21.0, 22.0, 23.0},
      {30.0, 31.0, 32.0, 33.0},
  });
  inplace_plane.index_put_({adv_row_idx, adv_col_idx}, adv_values, false);
  std::cout << inplace_plane << "\n\n";

  heading("index_put_inplace_out_of_range_error");
  try {
    auto bad_idx = pluss::make_series({5LL});
    inplace.index_put_({bad_idx}, pluss::make_series(10.0), false);
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  heading("index_put_inplace_int32_indices");
  auto int32_idx_inplace = pluss::make_series(std::vector<std::int32_t>{1, 3}, {2});
  inplace.index_put_({int32_idx_inplace}, picked, false);
  std::cout << inplace << "\n\n";

  return 0;
}
