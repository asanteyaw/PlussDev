#include <pluss/pluss.h>

#include <iostream>
#include <string>

namespace {

void heading(const char* label) {
  std::cout << "=== " << label << " ===\n";
}

void print_error(const std::exception& error) {
  const std::string message = error.what();
  std::cout << message.substr(0, message.find('\n')) << "\n";
}

}  // namespace

int main() {
  auto table = pluss::make_series({
      {1.0, 2.0, 3.0},
      {4.0, 5.0, 6.0},
  });

  heading("flip_rows");
  pluss::series::print(std::cout, table.flip(0));
  std::cout << "\n\n";

  heading("flip_columns_negative_dim");
  std::cout << table.flip(-1) << "\n\n";

  heading("flip_both_dims");
  std::cout << table.flip({0, 1}) << "\n\n";

  heading("flip_duplicate_dims");
  std::cout << table.flip({0, 0}) << "\n\n";

  auto cube = pluss::make_series({
      {{1.0, 2.0}, {3.0, 4.0}},
      {{5.0, 6.0}, {7.0, 8.0}},
  });
  heading("flip_cube_outer_axis");
  std::cout << cube.flip(0) << "\n\n";

  heading("flip_bad_dim_error");
  try {
    std::cout << table.flip(2) << "\n";
  } catch (const std::exception& error) {
    print_error(error);
  }
  std::cout << "\n";

  heading("flipud");
  std::cout << table.flipud() << "\n\n";

  heading("flipud_cube");
  std::cout << cube.flipud() << "\n\n";

  heading("fliplr");
  std::cout << table.fliplr() << "\n\n";

  heading("fliplr_cube");
  std::cout << cube.fliplr() << "\n\n";

  heading("fliplr_rank_error");
  try {
    std::cout << pluss::make_series({1.0, 2.0, 3.0}).fliplr() << "\n";
  } catch (const std::exception& error) {
    print_error(error);
  }
  std::cout << "\n";

  heading("roll_flat_positive");
  std::cout << table.roll(2) << "\n\n";

  heading("roll_dim_negative_shift");
  std::cout << table.roll(-1, 1) << "\n\n";

  heading("roll_multi_axis");
  std::cout << table.roll({1, -1}, {0, 1}) << "\n\n";

  heading("roll_shift_wraparound");
  std::cout << table.roll(4, 1) << "\n\n";

  heading("roll_multi_shift_without_dims_error");
  try {
    std::cout << table.roll({1, 2}) << "\n";
  } catch (const std::exception& error) {
    print_error(error);
  }
  std::cout << "\n";

  heading("roll_bad_dim_error");
  try {
    std::cout << table.roll(1, 2) << "\n";
  } catch (const std::exception& error) {
    print_error(error);
  }
  std::cout << "\n";

  heading("rot90_default");
  std::cout << table.rot90() << "\n\n";

  heading("rot90_half_turn");
  std::cout << table.rot90(2) << "\n\n";

  heading("rot90_negative_turn");
  std::cout << table.rot90(-1) << "\n\n";

  heading("rot90_explicit_dims");
  std::cout << cube.rot90(1, 1, 2) << "\n\n";

  heading("rot90_same_dim_error");
  try {
    std::cout << table.rot90(1, 0, 0) << "\n";
  } catch (const std::exception& error) {
    print_error(error);
  }
}
