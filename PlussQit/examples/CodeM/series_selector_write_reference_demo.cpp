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
  auto base = pluss::make_series({
      {10.0, 11.0, 12.0, 13.0},
      {20.0, 21.0, 22.0, 23.0},
      {30.0, 31.0, 32.0, 33.0},
  });

  auto grid = base.clone();
  heading("extract_put_inplace_setup");
  std::cout << grid << "\n\n";

  heading("extract_put_inplace_scalar_broadcast");
  grid.extract_put_({pluss::Slicing::SliceAll(), std::int64_t{1}}, pluss::make_series(-1.0));
  std::cout << grid << "\n\n";

  heading("extract_put_inplace_reverse_slice");
  grid.extract_put_(
      {std::int64_t{1}, pluss::Slicing::SliceRange(-1, pluss::Slicing::Slice::kUnbounded, -2)},
      pluss::make_series({90.0, 70.0}));
  std::cout << grid << "\n\n";

  heading("extract_put_inplace_newaxis_ignored");
  grid.extract_put_(
      {pluss::Slicing::NewAxis(), pluss::Slicing::SliceAll(), std::int64_t{0}},
      pluss::make_series({100.0, 200.0, 300.0}));
  std::cout << grid << "\n\n";

  auto row_mask = pluss::make_series({true, false, true});
  heading("extract_put_inplace_mask_rows");
  grid.extract_put_({row_mask, pluss::Slicing::SliceAll()}, pluss::make_series(0.0));
  std::cout << grid << "\n\n";

  heading("extract_put_inplace_integer_rows");
  auto picked_rows = pluss::make_series({2LL, 0LL});
  grid.extract_put_(
      {picked_rows, pluss::Slicing::SliceAll()},
      pluss::make_series({
          {3000.0, 3001.0, 3002.0, 3003.0},
          {1000.0, 1001.0, 1002.0, 1003.0},
      }));
  std::cout << grid << "\n\n";

  heading("extract_put_inplace_ellipsis");
  grid.extract_put_(
      {pluss::Slicing::Ellipsis(), std::int64_t{2}},
      pluss::make_series({12.5, 22.5, 32.5}));
  std::cout << grid << "\n\n";

  heading("extract_put_inplace_mismatch_error");
  try {
    grid.extract_put_(
        {std::int64_t{0}, pluss::Slicing::SliceRange(0, 4, 2)},
        pluss::make_series({1.0, 2.0, 3.0}));
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  heading("extract_put_inplace_zero_step_error");
  try {
    grid.extract_put_(
        {pluss::Slicing::SliceAll(), pluss::Slicing::SliceRange(0, 4, 0)},
        pluss::make_series(0.0));
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  auto copy_grid = pluss::make_series({
      {100.0, -1.0, 12.0, 13.0},
      {200.0, 70.0, 22.0, 90.0},
      {300.0, -1.0, 32.0, 33.0},
  });
  heading("extract_put_copy_original");
  std::cout << copy_grid << "\n\n";

  heading("extract_put_copy_result");
  auto patched = copy_grid.extract_put(
      {pluss::Slicing::SliceAll(), pluss::Slicing::SliceRange(2, 4, 1)},
      pluss::make_series(5.0));
  std::cout << patched << "\n\n";

  heading("extract_put_copy_original_after_call");
  std::cout << copy_grid << "\n\n";

  heading("extract_put_copy_mask_rows");
  std::cout << copy_grid.extract_put(
                   {row_mask, pluss::Slicing::SliceAll()},
                   pluss::make_series(0.0))
            << "\n\n";

  heading("extract_put_copy_reverse_slice");
  std::cout << copy_grid.extract_put(
      {std::int64_t{1}, pluss::Slicing::SliceRange(-1, pluss::Slicing::Slice::kUnbounded, -2)},
      pluss::make_series({900.0, 700.0})) << "\n\n";

  heading("extract_put_copy_integer_rows");
  std::cout << copy_grid.extract_put(
      {picked_rows, pluss::Slicing::SliceAll()},
      pluss::make_series({
          {9000.0, 9001.0, 9002.0, 9003.0},
          {7000.0, 7001.0, 7002.0, 7003.0},
      })) << "\n\n";

  heading("extract_put_copy_zero_step_error");
  try {
    std::cout << copy_grid.extract_put(
                     {pluss::Slicing::SliceAll(), pluss::Slicing::SliceRange(0, 4, 0)},
                     pluss::make_series(0.0))
              << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  auto traverse_grid = base.clone();
  heading("traverse_put_inplace_setup");
  std::cout << traverse_grid << "\n\n";

  heading("traverse_put_inplace_column");
  traverse_grid.traverse_put_(
      {pluss::Slicing::SliceAll(), std::int64_t{1}},
      pluss::make_series({100.0, 200.0, 300.0}));
  std::cout << traverse_grid << "\n\n";

  heading("traverse_put_inplace_newaxis_ignored");
  traverse_grid.traverse_put_(
      {pluss::Slicing::NewAxis(), pluss::Slicing::SliceAll(), std::int64_t{0}},
      pluss::make_series({1.0, 2.0, 3.0}));
  std::cout << traverse_grid << "\n\n";

  heading("traverse_put_inplace_scalar_broadcast");
  traverse_grid.traverse_put_(
      {std::int64_t{2}, pluss::Slicing::SliceRange(2, 4, 1)},
      pluss::make_series(-5.0));
  std::cout << traverse_grid << "\n\n";

  heading("traverse_put_inplace_ellipsis");
  traverse_grid.traverse_put_(
      {pluss::Slicing::Ellipsis(), std::int64_t{3}},
      pluss::make_series({13.5, 23.5, 33.5}));
  std::cout << traverse_grid << "\n\n";

  heading("traverse_put_inplace_reverse_error");
  try {
    traverse_grid.traverse_put_(
        {pluss::Slicing::SliceAll(), pluss::Slicing::SliceRange(-1, pluss::Slicing::Slice::kUnbounded, -1)},
        pluss::make_series(0.0));
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  heading("traverse_put_inplace_mask_error");
  try {
    traverse_grid.traverse_put_({row_mask, pluss::Slicing::SliceAll()}, pluss::make_series(0.0));
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  heading("traverse_put_inplace_mismatch_error");
  try {
    traverse_grid.traverse_put_(
        {std::int64_t{0}, pluss::Slicing::SliceRange(0, 2, 1)},
        pluss::make_series({1.0, 2.0, 3.0}));
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  heading("traverse_put_copy_original");
  std::cout << traverse_grid << "\n\n";

  heading("traverse_put_copy_result");
  auto copied = traverse_grid.traverse_put(
      {std::int64_t{0}, pluss::Slicing::SliceRange(0, 2, 1)},
      pluss::make_series(999.0));
  std::cout << copied << "\n\n";

  heading("traverse_put_copy_newaxis");
  std::cout << traverse_grid.traverse_put(
      {pluss::Slicing::NewAxis(), pluss::Slicing::SliceAll(), std::int64_t{0}},
      pluss::make_series({10.0, 20.0, 30.0})) << "\n\n";

  heading("traverse_put_copy_ellipsis");
  std::cout << traverse_grid.traverse_put(
      {pluss::Slicing::Ellipsis(), std::int64_t{2}},
      pluss::make_series({120.0, 220.0, 320.0})) << "\n\n";

  heading("traverse_put_copy_reverse_error");
  try {
    std::cout << traverse_grid.traverse_put(
                     {pluss::Slicing::SliceAll(), pluss::Slicing::SliceRange(-1, pluss::Slicing::Slice::kUnbounded, -1)},
                     pluss::make_series(0.0))
              << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  heading("traverse_put_copy_mask_error");
  try {
    std::cout << traverse_grid.traverse_put(
                     {row_mask, pluss::Slicing::SliceAll()},
                     pluss::make_series(0.0))
              << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  heading("traverse_put_copy_mismatch_error");
  try {
    std::cout << traverse_grid.traverse_put(
                     {std::int64_t{0}, pluss::Slicing::SliceRange(0, 2, 1)},
                     pluss::make_series({1.0, 2.0, 3.0}))
              << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  heading("traverse_put_copy_original_after_call");
  std::cout << traverse_grid << "\n\n";

  return 0;
}
