#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// compare_series(a, b) returns a SeriesCompareReport (reports::SeriesCompare):
// the per-element abs_error Series, plus max_abs_diff and mae as 1-element
// Series. Used to check how far two vectors are apart.
int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series expected =
      pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0}, {5}, f64);
  const pluss::Series actual =
      pluss::make_series({1.0, 2.1, 2.7, 4.0, 5.5}, {5}, f64);

  const lymph::metrics::SeriesCompareReport diff =
      lymph::metrics::compare_series(expected, actual);

  std::cout << "abs_error   =";
  for (const double v : diff.abs_error.to_vec<double>()) std::cout << " " << v;
  std::cout << "\nmax_abs_diff = " << diff.max_abs_diff.item<double>() << "\n";
  std::cout << "mae         = " << diff.mae.item<double>() << "\n";

  return 0;
}
