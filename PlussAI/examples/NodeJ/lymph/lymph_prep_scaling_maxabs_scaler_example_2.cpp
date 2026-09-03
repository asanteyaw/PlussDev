#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>
#include <vector>

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

  // MaxAbsScaler learns a single value per column: the largest magnitude,
  // state[1].
  lymph::prep::MaxAbsScaler scaler;
  scaler.features({"amount"}).learn(train);
  const auto s = scaler.summary();
  std::cout << "state tensors: " << s.state.size() << "\n";
  std::cout << "learned max magnitude = " << s.state[1].to_vec<double>()[0] << "\n";

  return 0;
}
