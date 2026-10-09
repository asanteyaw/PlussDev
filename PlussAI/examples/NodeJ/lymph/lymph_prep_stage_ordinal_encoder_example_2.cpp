#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {

  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  // The expansion made visible through the standalone prep::OrdinalEncoder.
  // one_based = true (default): codes start at 1, 0 marks unseen. Set
  // one_based = false to start at 0.
  const pluss::Series tier = pluss::make_series({10, 20, 30, 10, 20, 30}, {6}, f64);
  const DataFrame frame({{"tier", Slate(tier)}});

  lymph::prep::OrdinalEncoder one_based{lymph::options::OrdinalEncoder{}};
  one_based.features({"tier"});
  std::cout << "one_based = true:\n" << one_based.learn_map(frame) << "\n";

  lymph::options::OrdinalEncoder zero{};
  zero.one_based = false;
  lymph::prep::OrdinalEncoder zero_based{zero};
  zero_based.features({"tier"});
  std::cout << "one_based = false:\n" << zero_based.learn_map(frame);

  return 0;
}
