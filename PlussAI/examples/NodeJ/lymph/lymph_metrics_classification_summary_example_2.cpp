#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// ClassReportOptions.zero_division sets the value precision / recall take when
// their denominator is empty. Here class 1 is never predicted, so its precision
// denominator is 0.
int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series truth =
      pluss::make_series({0, 0, 0, 0, 0, 0, 1, 1, 1, 1}, {10}, f64);
  const pluss::Series predicted =
      pluss::make_series({0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {10}, f64);

  const lymph::ClassReport zero =
      lymph::metrics::classification::summary(truth, predicted,
                                    lymph::ClassReportOptions{.zero_division = 0.0});
  const lymph::ClassReport one =
      lymph::metrics::classification::summary(truth, predicted,
                                    lymph::ClassReportOptions{.zero_division = 1.0});

  std::cout << "zero_division=0.0  precision = " << zero.precision << "\n";
  std::cout << "zero_division=1.0  precision = " << one.precision << "\n";

  return 0;
}
