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
  // city 1 appears 4/8, city 2 appears 3/8, city 3 appears 1/8.
  const DataFrame train({{"city", Slate(pluss::make_series({1., 1., 1., 1., 2., 2., 2., 3.}, {8}, f64))}});
  const DataFrame later({{"city", Slate(pluss::make_series({3., 2., 1.}, {3}, f64))}});

  // FrequencyEncoder replaces each category with its observed relative frequency
  // in [0, 1]. One numeric column in, one out, same name.
  lymph::prep::FrequencyEncoder encoder;
  std::cout << "learn_map(train):\n"
            << encoder.features({"city"}).learn_map(train) << "\n";
  std::cout << "map(later) -- fitted frequencies replayed:\n"
            << encoder.map(later) << "\n";

  return 0;
}
