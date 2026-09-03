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
  const DataFrame train({{"region", Thread(pluss::make_series({10., 20., 30., 10., 20., 30.}, {6}, f64))}});
  const DataFrame later({{"region", Thread(pluss::make_series({30., 10., 20.}, {3}, f64))}});

  // OrdinalEncoder replaces each category with a small integer code. The column
  // count does not change -- one integer column in, one out, same name. Codes
  // are 1-based by default; 0 is reserved for a category unseen at fit time.
  lymph::prep::OrdinalEncoder encoder;
  const DataFrame coded = encoder.features({"region"}).learn_map(train);
  std::cout << "learn_map(train):\n" << coded << "\n";
  std::cout << "map(later) -- same code assignment replayed:\n" << encoder.map(later) << "\n";

  const lymph::prep::TransformerSummary s = encoder.summary();
  std::cout << "summary.state tensors = " << s.state.size()
            << ", learned categories = " << s.state.back().numel() << "\n";
  std::cout << "mapped_names_[0] = " << encoder.mapped_names_[0] << "\n";

  return 0;
}
