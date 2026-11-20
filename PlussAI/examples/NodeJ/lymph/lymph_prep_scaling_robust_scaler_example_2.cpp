#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>
#include <vector>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Shelf;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series amount = pluss::make_series({10., 20., 30., 40., 50.}, {5}, f64);
  const pluss::Series ratio  = pluss::make_series({0.2, 0.4, 0.6, 0.8, 1.0}, {5}, f64);
  const DataFrame train({{"amount", Shelf(amount)}, {"ratio", Shelf(ratio)}});
  const DataFrame later({{"amount", Shelf(pluss::make_series({15., 35., 60.}, {3}, f64))},
                        {"ratio",  Shelf(pluss::make_series({0.3, 0.7, 1.2}, {3}, f64))}});

  // For RobustScaler, state[1] is the learned center (median) and state[2] the
  // learned scale (the quantile range).
  lymph::prep::RobustScaler scaler;
  scaler.features({"amount"}).learn(train);
  const auto s = scaler.summary();
  std::cout << "center (median) = " << s.state[1].host<double>()[0] << "\n";
  std::cout << "scale  (IQR)    = " << s.state[2].host<double>()[0] << "\n";

  return 0;
}
