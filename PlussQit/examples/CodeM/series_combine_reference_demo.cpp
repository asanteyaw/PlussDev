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
  auto left = pluss::make_series(std::vector<double>{1, 2, 3, 4}, {2, 2});
  auto right = pluss::make_series(std::vector<double>{10, 20, 30, 40}, {2, 2});

  heading("unite_dim0");
  std::cout << left.unite({right}, 0, true) << "\n\n";

  heading("unite_dim_minus1");
  std::cout << left.unite({right}, -1, true) << "\n\n";

  heading("unite_dtype_mismatch_error");
  try {
    auto ints = pluss::make_series(std::vector<std::int64_t>{7, 8, 9, 10}, {2, 2});
    std::cout << left.unite({ints}, 0, true) << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  heading("unite_dtype_promotion");
  auto ints = pluss::make_series(std::vector<std::int64_t>{7, 8, 9, 10}, {2, 2});
  std::cout << left.unite({ints}, 0, false) << "\n\n";

  heading("unite_gradient_split");
  auto tracked_left = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0}, {2, 2});
  auto tracked_right = pluss::make_series(std::vector<double>{10.0, 20.0, 30.0, 40.0}, {2, 2});
  tracked_left.tracks_slope_(true);
  tracked_right.tracks_slope_(true);
  auto united = tracked_left.unite({tracked_right}, 0, true);
  united.sum().reverse();
  std::cout << "dleft=" << tracked_left.sensitivity() << "\n";
  std::cout << "dright=" << tracked_right.sensitivity() << "\n\n";

  heading("pile_dim0");
  std::cout << left.pile({right}, 0) << "\n\n";

  heading("pile_dim_minus1");
  std::cout << left.pile({right}, -1) << "\n\n";

  heading("pile_shape_mismatch_error");
  try {
    auto bad_shape = pluss::make_series(std::vector<double>{5, 6, 7, 8, 9, 10}, {3, 2});
    std::cout << left.pile({bad_shape}, 0) << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  heading("pile_gradient_split");
  tracked_left.clear();
  tracked_right.clear();
  auto stacked = tracked_left.pile({tracked_right}, 1);
  stacked.sum().reverse();
  std::cout << "dleft=" << tracked_left.sensitivity() << "\n";
  std::cout << "dright=" << tracked_right.sensitivity() << "\n\n";

  return 0;
}
