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

  // Standalone view: region 0 appears 3/6, region 1 2/6, region 2 1/6.
  const pluss::Series region = pluss::make_series({0, 0, 0, 1, 1, 2}, {6}, f64);
  const DataFrame frame({{"region", Slate(region)}});

  lymph::prep::FrequencyEncoder enc{lymph::options::FrequencyEncoder{}};
  enc.features({"region"});
  std::cout << enc.learn_map(frame);

  return 0;
}
