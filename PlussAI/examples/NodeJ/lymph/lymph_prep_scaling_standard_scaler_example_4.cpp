#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series amount = pluss::make_series({10., 20., 30., 40., 50.}, {5}, f64);
  const pluss::Series ratio  = pluss::make_series({0.2, 0.4, 0.6, 0.8, 1.0}, {5}, f64);
  const DataFrame train({{"amount", Thread(amount)}, {"ratio", Thread(ratio)}});
  const DataFrame later({{"amount", Thread(pluss::make_series({15., 35., 60.}, {3}, f64))},
                        {"ratio",  Thread(pluss::make_series({0.3, 0.7, 1.2}, {3}, f64))}});

  // One fitted scaler, reused. Fit once on train; every later frame is
  // transformed against the same statistics -- the point of the object form
  // over a one-shot stage.
  lymph::prep::StandardScaler scaler;
  scaler.features({"amount", "ratio"}).learn(train);

  std::cout << "map(train):\n" << scaler.map(train) << "\n";
  std::cout << "map(later) -- same mean/scale, so row 2 (amount 60) lands past +2:\n"
            << scaler.map(later) << "\n";
  std::cout << "map(train) again -- identical, the object is not consumed:\n"
            << scaler.map(train);

  return 0;
}
