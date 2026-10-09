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
  // Five categories 10, 20, 30, 40, 50.
  const DataFrame train({{"code", Slate(pluss::make_series({10., 20., 30., 40., 50., 10., 30.}, {7}, f64))}});
  const DataFrame later({{"code", Slate(pluss::make_series({50., 20., 10.}, {3}, f64))}});

  // BaseNEncoder ordinal-codes each category (1..K, 0 reserved for unseen), then
  // writes that code in radix `base` across a fixed set of digit columns.
  // base = 2 is the binary encoder: 5 categories need ceil(log2(5 + 1)) = 3
  // columns instead of one-hot's 5.
  lymph::prep::BaseNEncoder encoder;  // base = 2
  std::cout << "learn_map(train) -- code_0..code_2 are the binary digits:\n"
            << encoder.features({"code"}).learn_map(train) << "\n";
  std::cout << "map(later):\n" << encoder.map(later) << "\n";

  return 0;
}
