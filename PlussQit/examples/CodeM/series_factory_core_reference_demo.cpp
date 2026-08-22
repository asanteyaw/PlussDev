#include <pluss/pluss.h>

#include <iostream>
#include <string>
#include <vector>

namespace {

void heading(const char* label) {
  std::cout << "=== " << label << " ===\n";
}

}  // namespace

int main() {
  heading("make_scalar");
  std::cout << pluss::series::make(42.5) << "\n\n";

  heading("make_vector");
  std::cout << pluss::series::make({10.0, 12.5, 9.0}) << "\n\n";

  heading("make_matrix");
  std::cout << pluss::series::make({
      {1.0, 2.0},
      {3.0, 4.0},
  }) << "\n\n";

  heading("make_flat_with_shape");
  std::cout << pluss::series::make(std::vector<double>{1.0, 2.0, 3.0, 4.0}, {2, 2}) << "\n\n";

  heading("make_dtype_override");
  std::cout << pluss::series::make(std::vector<int>{1, 2, 3, 4}, {2, 2}, pluss::ScalarType::Float64) << "\n\n";

  heading("arange_end");
  std::cout << pluss::series::arange(5) << "\n\n";

  heading("arange_start_end");
  std::cout << pluss::series::arange(2, 7) << "\n\n";

  heading("arange_step");
  std::cout << pluss::series::arange(10, 0, -3) << "\n\n";

  heading("arange_config_float32");
  std::cout << pluss::series::arange(
      2,
      7,
      pluss::Config{}.with_dtype(pluss::ScalarType::Float32)) << "\n\n";

  heading("empty_safe_init");
  auto scratch = pluss::series::empty({2, 2}, pluss::ScalarType::Float64);
  scratch.copy_(pluss::series::make({
      {5.0, 6.0},
      {7.0, 8.0},
  }));
  std::cout << scratch << "\n\n";

  heading("empty_int32_safe_init");
  auto ids = pluss::series::empty({3}, pluss::ScalarType::Int32);
  ids.copy_(pluss::series::make({10, 20, 30}, pluss::ScalarType::Int32));
  std::cout << ids << "\n\n";

  heading("full_matrix");
  std::cout << pluss::series::full({2, 3}, 1.5) << "\n\n";

  heading("full_scalar");
  std::cout << pluss::series::full({}, -2.0) << "\n\n";

  heading("full_int32");
  std::cout << pluss::series::full({2, 2}, 7.0, pluss::ScalarType::Int32) << "\n\n";

  heading("ones_matrix");
  std::cout << pluss::series::ones({2, 3}) << "\n\n";

  heading("ones_int16");
  std::cout << pluss::series::ones({4}, pluss::ScalarType::Int16) << "\n\n";

  heading("zeros_matrix");
  std::cout << pluss::series::zeros({2, 3}) << "\n\n";

  heading("zeros_bool");
  std::cout << pluss::series::zeros({4}, pluss::ScalarType::Bool) << "\n\n";

  return 0;
}
