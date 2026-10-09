#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series v = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0}, {10}, f64);
  const DataFrame frame({{"v", Slate(v)}});

  // with_centering / with_scaling switch the two steps independently, exactly
  // like standard_scaler. lower_quantile / upper_quantile widen or narrow the
  // spread window: the default 0.25/0.75 is the IQR; 0.10/0.90 uses a wider
  // band. Family-alias spelling for the option struct.
  lymph::prep::RobustScalerOptions centre_only{};
  centre_only.with_scaling = false;

  lymph::prep::RobustScalerOptions wide_band{};
  wide_band.lower_quantile = 0.10;
  wide_band.upper_quantile = 0.90;

  std::cout << "with_scaling = false (median removed, spread untouched):\n";
  std::cout << lymph::prep::RobustScaler{centre_only}.learn_map(frame) << "\n";
  std::cout << "quantiles 0.10 / 0.90 (wider band -> smaller scaled values):\n";
  std::cout << lymph::prep::RobustScaler{wide_band}.learn_map(frame);

  return 0;
}
