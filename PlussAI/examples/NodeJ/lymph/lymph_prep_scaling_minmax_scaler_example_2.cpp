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

  // summary() on the macro-based scalers is a TransformerSummary: the pinned
  // feature_names plus a state vector of raw fitted tensors. For MinMaxScaler
  // state[1] is the learned minimum and state[2] the learned maximum
  // (state[0] is internal bookkeeping).
  lymph::prep::MinMaxScaler scaler;
  scaler.features({"amount"}).learn(train);
  const lymph::prep::TransformerSummary s = scaler.summary();
  std::cout << "feature_names: " << s.feature_names[0] << "\n";
  std::cout << "state tensors: " << s.state.size() << "\n";
  std::cout << "learned min = " << s.state[1].host<double>()[0] << "\n";
  std::cout << "learned max = " << s.state[2].host<double>()[0] << "\n";

  return 0;
}
