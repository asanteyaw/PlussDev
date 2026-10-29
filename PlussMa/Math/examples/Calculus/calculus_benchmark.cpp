#include <pluss/ma.h>

#include <chrono>
#include <complex>
#include <iostream>
#include <string_view>

namespace {

using Clock = std::chrono::steady_clock;

double synchronize(const pluss::Series &value) {
  return value.abs()
      .sum()
      .to(pluss::pCPU, pluss::ScalarType::Float64)
      .item<double>();
}

void benchmark(std::string_view label, const pluss::math::Function &function,
               const pluss::Series &input, int repeats) {
  const auto cold_start = Clock::now();
  pluss::Series result = function({input});
  const double cold_checksum = synchronize(result);
  const auto cold_end = Clock::now();

  const auto warm_start = Clock::now();
  for (int repeat = 0; repeat < repeats; ++repeat) {
    result = function({input});
  }
  const double warm_checksum = synchronize(result);
  const auto warm_end = Clock::now();

  std::cout << label << "\n";
  std::cout << "  cold_us = "
            << std::chrono::duration_cast<std::chrono::microseconds>(
                   cold_end - cold_start)
                   .count()
            << "\n";
  std::cout << "  " << repeats << "_warm_ms = "
            << std::chrono::duration_cast<std::chrono::milliseconds>(
                   warm_end - warm_start)
                   .count()
            << "\n";
  std::cout << "  checksum = " << cold_checksum + warm_checksum << "\n";
}

} // namespace

int main() {
  namespace math = pluss::math;
  namespace operators = pluss::operators;

  const math::Function real = math::function({"x"}, [](math::Function x) {
    const math::Function shared = x.sin() * x.exp();
    return shared * shared + x.pow(4.0);
  });
  const auto transform_start = Clock::now();
  const math::Function fourth = operators::differential("x", 4)(real);
  const auto transform_end = Clock::now();

  std::cout << "analytic_transform_us = "
            << std::chrono::duration_cast<std::chrono::microseconds>(
                   transform_end - transform_start)
                   .count()
            << "\n";

  const pluss::Series cpu_real =
      pluss::Series::linspace(-2.0, 2.0, 100000,
                              pluss::ScalarType::Float64);
  benchmark("cpu_float64_d4", fourth, cpu_real, 100);

  const math::Function complex =
      math::function({"z"}, [](math::Function z) {
        const math::Function shared = z.sin() * z.exp();
        return shared * shared.conj() + z.pow(3.0);
      });
  const math::Function dz = operators::wirtinger("z")(complex);
  const pluss::Series cpu_complex = pluss::series::complex(
      pluss::Series::linspace(0.1, 1.0, 100000,
                              pluss::ScalarType::Float32),
      pluss::Series::linspace(-0.5, 0.5, 100000,
                              pluss::ScalarType::Float32));
  benchmark("cpu_complex64_dz", dz, cpu_complex, 100);

  if (pluss::platform_available(pluss::pMPS)) {
    benchmark("mps_float32_d4", fourth,
              cpu_real.to(pluss::ScalarType::Float32).to(pluss::pMPS), 100);
    benchmark("mps_complex64_dz", dz, cpu_complex.to(pluss::pMPS), 100);
  }
}
