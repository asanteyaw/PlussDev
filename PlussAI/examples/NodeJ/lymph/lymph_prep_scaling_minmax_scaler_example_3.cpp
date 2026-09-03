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

  // learn_map does learn() then map() and hands back the transformed frame.
  // With no features() call it takes every numeric column.
  lymph::prep::MinMaxScaler scaler;
  const DataFrame out = scaler.learn_map(train);
  std::cout << out << "\n";
  std::cout << "learned = " << std::boolalpha << scaler.learned() << "\n";

  return 0;
}
