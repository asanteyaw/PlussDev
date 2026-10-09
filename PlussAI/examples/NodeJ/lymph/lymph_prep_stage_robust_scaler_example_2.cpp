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

  // Five clustered values and one extreme outlier (1000).
  const pluss::Series clean = pluss::make_series({10.0, 12.0, 14.0, 16.0, 18.0, 20.0}, {6}, f64);
  const pluss::Series spiked = pluss::make_series({10.0, 12.0, 14.0, 16.0, 18.0, 1000.0}, {6}, f64);
  const DataFrame clean_df({{"v", Slate(clean)}});
  const DataFrame spiked_df({{"v", Slate(spiked)}});

  // robust_scaler uses the median and IQR, so the quartile-based centre and
  // scale barely move when one value blows up. standard_scaler, driven by the
  // mean and standard deviation, is dragged toward the outlier.
  std::cout << "robust_scaler, no outlier:\n";
  std::cout << lymph::prep::RobustScaler{}.learn_map(clean_df) << "\n";
  std::cout << "robust_scaler, with outlier (first five rows still ~unchanged):\n";
  std::cout << lymph::prep::RobustScaler{}.learn_map(spiked_df) << "\n";
  std::cout << "standard_scaler, same outlier (first five rows squashed):\n";
  std::cout << lymph::prep::StandardScaler{}.learn_map(spiked_df);

  return 0;
}
