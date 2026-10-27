#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>
#include <vector>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series amount = pluss::make_series({10., 20., 30., 40., 50.}, {5}, f64);
  const pluss::Series ratio  = pluss::make_series({0.2, 0.4, 0.6, 0.8, 1.0}, {5}, f64);
  const DataFrame train({{"amount", Slate(amount)}, {"ratio", Slate(ratio)}});
  const DataFrame later({{"amount", Slate(pluss::make_series({15., 35., 60.}, {3}, f64))},
                        {"ratio",  Slate(pluss::make_series({0.3, 0.7, 1.2}, {3}, f64))}});

  // summary() exposes the fitted parameters. StandardScaler's summary is a
  // dedicated struct with mean and scale as one-per-column Series.
  lymph::prep::StandardScaler scaler;
  scaler.features({"amount", "ratio"}).learn(train);
  const lymph::prep::StandardScalerSummary s = scaler.summary();

  const auto mean  = s.mean.to(pluss::pCPU, ScalarType::Float64).reshape({s.mean.numel()}).host<double>();
  const auto scale = s.scale.to(pluss::pCPU, ScalarType::Float64).reshape({s.scale.numel()}).host<double>();
  for (std::size_t i = 0; i < s.feature_names.size(); ++i) {
    std::cout << s.feature_names[i] << ": mean=" << mean[i] << " scale=" << scale[i] << "\n";
  }

  return 0;
}
