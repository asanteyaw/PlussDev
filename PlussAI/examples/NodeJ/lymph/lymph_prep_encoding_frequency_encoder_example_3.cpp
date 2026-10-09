#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// The caveat of frequency encoding: two distinct categories that occur equally
// often collapse to the same value. Here cities 2 and 3 each appear twice, so
// both map to 0.25 -- the model can no longer tell them apart.
int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  //   city 1 x4,  city 2 x2,  city 3 x2   (8 rows)
  const DataFrame train({{"city", Slate(pluss::make_series(
      {1., 1., 1., 1., 2., 2., 3., 3.}, {8}, f64))}});

  lymph::prep::FrequencyEncoder encoder;
  std::cout << "learn_map(train) -- cities 2 and 3 both -> 0.25:\n"
            << encoder.features({"city"}).learn_map(train);

  return 0;
}
