#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// The DataFrame path: pin the column with features(), fit with learn(), replay
// with map(). learned() guards it, summary() reports the learned category list.
int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame train({{"grade", Thread(pluss::make_series({10., 20., 30., 20., 10., 30.}, {6}, f64))}});
  const DataFrame later({{"grade", Thread(pluss::make_series({30., 30., 10.}, {3}, f64))}});

  lymph::prep::LabelEncoder encoder;
  std::cout << "learned before = " << std::boolalpha << encoder.learned() << "\n";

  encoder.features({"grade"}).learn(train);
  std::cout << "learned after  = " << encoder.learned() << "\n\n";

  std::cout << "map(train) -- dense 0..K-1 codes, column keeps its name:\n"
            << encoder.map(train) << "\n";
  std::cout << "map(later) -- same assignment replayed:\n" << encoder.map(later) << "\n";

  const lymph::prep::TransformerSummary s = encoder.summary();
  std::cout << "summary: " << s.feature_names[0] << ", "
            << s.state.back().numel() << " learned categories\n";

  return 0;
}
