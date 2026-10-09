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

  // Standalone, showing the digit layout. Codes are 1..K (0 = unseen). base 2
  // gives a binary code; base 3 packs the same 5 categories into 2 columns.
  const pluss::Series c = pluss::make_series({0,1,2,3,4,0,1,2}, {8}, f64);
  const DataFrame frame({{"c", Slate(c)}});

  lymph::prep::BaseNEncoder binary{lymph::options::BaseNEncoder{}};   // base 2
  binary.features({"c"});
  std::cout << "base = 2:\n" << binary.learn_map(frame) << "\n";

  lymph::options::BaseNEncoder b3{};
  b3.base = 3;
  lymph::prep::BaseNEncoder ternary{b3};
  ternary.features({"c"});
  std::cout << "base = 3:\n" << ternary.learn_map(frame);

  return 0;
}
