#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// max_abs_diff(a, b) is the one-liner: it builds the SeriesCompareReport and
// hands back just the largest absolute element-wise gap, as a double.
int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series a =
      pluss::make_series({0.10, 0.20, 0.30, 0.40, 0.50}, {5}, f64);
  const pluss::Series b =
      pluss::make_series({0.101, 0.198, 0.307, 0.400, 0.494}, {5}, f64);

  const double gap = lymph::metrics::max_abs_diff(a, b);
  std::cout << "max_abs_diff = " << gap << "\n";
  std::cout << "within 1e-2 ? " << std::boolalpha << (gap < 1e-2) << "\n";

  return 0;
}
