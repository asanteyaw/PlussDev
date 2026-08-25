#include <pluss/lymph.h>
#include <pluss/mic.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame raw({
      {"income", Thread(pluss::make_series({1.0, 1.4, 1.8, 2.2, 3.8, 4.2, 4.6, 5.0}, {8}, f64))},
      {"balance", Thread(pluss::make_series({0.2, 0.4, 0.6, 0.9, 1.8, 2.1, 2.4, 2.7}, {8}, f64))},
      {"late", Thread(pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64))},
  });

  lymph::prep::StandardScaler scaler(lymph::prep::StandardScalerOptions{
      .with_centering = true,
      .with_scaling = true,
      .variance_floor = 1e-12,
  });

  const DataFrame scaled =
      scaler.learn_map(raw.features({"income", "balance", "late"}));
  const lymph::prep::StandardScalerSummary summary = scaler.summary();

  std::cout << "== standard_scaler ==\n";
  std::cout << "rows=" << scaled.ncases()
            << " features=" << scaled.nfeatures()
            << " summary_features=" << summary.feature_names.size() << "\n";
  std::cout << scaled.head(2) << "\n";

  return 0;
}
