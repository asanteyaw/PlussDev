#include <pluss/pluss.h>

#include <iomanip>
#include <iostream>
#include <limits>
#include <string_view>

namespace {

void heading(const char* name) {
  std::cout << "=== " << name << " ===\n";
}

template <class Range>
void print_range(const Range& values) {
  std::cout << '[';
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i) std::cout << ',';
    std::cout << values[i];
  }
  std::cout << "]\n";
}

void print_error(const std::exception& error) {
  const std::string_view message(error.what());
  const auto newline = message.find('\n');
  std::cout << message.substr(0, newline) << '\n';
}

} // namespace

int main() {
  std::cout << std::boolalpha;

  const auto scalar = pluss::make_series(7.0);
  const auto matrix = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});
  const auto transposed = matrix.transpose(0, 1);

  heading("rank_count_and_stride");
  std::cout << "scalar: dim=" << scalar.dim() << ", numel=" << scalar.numel() << '\n';
  std::cout << "matrix: dim=" << matrix.dim() << ", numel=" << matrix.numel()
            << ", stride(0)=" << matrix.stride(0)
            << ", stride(-1)=" << matrix.stride(-1) << '\n';
  std::cout << "matrix strides=";
  print_range(matrix.strides());
  std::cout << '\n';

  heading("logical_layout");
  std::cout << "matrix contiguous: " << matrix.is_contiguous() << '\n';
  std::cout << "transposed sizes=";
  print_range(transposed.sizes());
  std::cout << "transposed strides=";
  print_range(transposed.strides());
  std::cout << "transposed contiguous: " << transposed.is_contiguous() << "\n\n";

  heading("dtype_queries");
  const auto real = pluss::Series::zeros({2}, pluss::ScalarType::Float32);
  const auto integer = pluss::Series::zeros({2}, pluss::ScalarType::UInt32);
  const auto complex = pluss::Series::zeros({2}, pluss::ScalarType::Complex64);
  std::cout << "real dtype=" << real.dtype() << ", bytes=" << real.dtype().itemsize
            << ", floating=" << real.is_floating_point()
            << ", signed=" << real.is_signed() << '\n';
  std::cout << "integer dtype=" << integer.dtype()
            << ", signed=" << integer.is_signed() << '\n';
  std::cout << "complex dtype=" << complex.dtype()
            << ", complex=" << complex.is_complex()
            << ", signed=" << complex.is_signed() << "\n\n";

  heading("state_queries");
  auto leaf = pluss::make_series({1.0, 2.0});
  leaf.tracks_slope_(true);
  const auto result = leaf.square();
  std::cout << "cpu=" << leaf.is_cpu() << ", gpu=" << leaf.is_gpu()
            << ", cuda=" << leaf.is_cuda() << ", mps=" << leaf.is_mps()
            << ", leaf=" << leaf.is_leaf() << ", result leaf=" << result.is_leaf() << '\n';
  std::cout << "zero nonzero=" << pluss::make_series(0.0).is_nonzero()
            << ", three nonzero=" << pluss::make_series({3.0}).is_nonzero() << "\n\n";

  heading("size_storage_and_value_identity");
  const auto alias = matrix;
  const auto view = matrix.transpose(0, 1);
  const auto copy = matrix.clone();
  std::cout << "same size as transpose: " << matrix.is_same_size(view) << '\n';
  std::cout << "set to alias: " << matrix.is_set_to(alias) << '\n';
  std::cout << "set to transpose: " << matrix.is_set_to(view) << '\n';
  std::cout << "set to clone: " << matrix.is_set_to(copy) << '\n';
  std::cout << "equal to clone: " << matrix.equal(copy) << "\n\n";

  heading("exact_equality");
  std::cout << "same values, different dtype: "
            << pluss::make_series({1, 2, 3}).equal(pluss::make_series({1.0, 2.0, 3.0})) << '\n';
  std::cout << "broadcastable but different shape: "
            << pluss::make_series({1, 2, 3}).equal(pluss::make_series({{1, 2, 3}})) << '\n';
  std::cout << "two empty [0] Series: "
            << pluss::Series::empty({0}, pluss::ScalarType::Float64)
                   .equal(pluss::Series::empty({0}, pluss::ScalarType::Float64)) << "\n\n";

  heading("asymmetric_tolerance");
  std::cout << "9 close to 10: "
            << pluss::make_series({9.0}).isclose(pluss::make_series({10.0}), 0.1, 0.0).item<bool>()
            << '\n';
  std::cout << "10 close to 9: "
            << pluss::make_series({10.0}).isclose(pluss::make_series({9.0}), 0.1, 0.0).item<bool>()
            << "\n\n";

  heading("broadcast_isclose");
  const auto broadcast_close = pluss::make_series({9.0, 20.0})
                                   .reshape({2, 1})
                                   .isclose(
                                       pluss::make_series({{10.0, 20.0, 40.0}}),
                                       0.1,
                                       0.0);
  pluss::series::print(std::cout, broadcast_close);
  std::cout << '\n';

  heading("nan_and_infinity");
  const double inf = std::numeric_limits<double>::infinity();
  const double nan = std::numeric_limits<double>::quiet_NaN();
  const auto a = pluss::make_series({inf, -inf, nan, 1.0});
  const auto b = pluss::make_series({inf, inf, nan, 1.000001});
  pluss::series::print(std::cout, a.isclose(b));
  std::cout << '\n';
  pluss::series::print(std::cout, a.isclose(b, 1e-5, 1e-8, true));
  std::cout << '\n';
  std::cout << "all close (NaNs unequal): " << a.allclose(b) << '\n';
  std::cout << "all close (NaNs equal): " << a.allclose(b, 1e-5, 1e-8, true) << "\n\n";

  heading("contract_failures");
  try {
    matrix.stride(2);
  } catch (const std::exception& error) {
    print_error(error);
  }
  try {
    pluss::make_series({1.0, 2.0}).is_nonzero();
  } catch (const std::exception& error) {
    print_error(error);
  }
  try {
    pluss::make_series({1.0}).isclose(pluss::make_series({1}));
  } catch (const std::exception& error) {
    print_error(error);
  }
  try {
    pluss::make_series({1.0}).allclose(pluss::make_series({1.0}), -1.0, 0.0);
  } catch (const std::exception& error) {
    print_error(error);
  }
}
