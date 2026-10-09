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

  // The standalone object holds its fitted state. Pin the columns with
  // features(), fit with learn() (which returns *this so it chains), then apply
  // map() to any later frame -- the learned mean and scale are replayed.
  lymph::prep::StandardScaler scaler;
  std::cout << "learned before = " << std::boolalpha << scaler.learned() << "\n";

  scaler.features({"amount", "ratio"}).learn(train);
  std::cout << "learned after  = " << scaler.learned() << "\n\n";

  std::cout << "map(train):\n" << scaler.map(train) << "\n";
  std::cout << "map(later) -- same fitted parameters, new rows:\n" << scaler.map(later);

  return 0;
}
