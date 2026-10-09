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

  // region is an integer-coded category (0, 1, 2). The standalone encoder holds
  // its fitted category layout; pin the column with features(), fit with
  // learn() (which returns *this so it chains), then map() any later frame.
  const DataFrame train({{"region", Slate(pluss::make_series({0., 1., 2., 0., 1., 2.}, {6}, f64))}});
  const DataFrame later({{"region", Slate(pluss::make_series({2., 0., 1.}, {3}, f64))}});

  lymph::prep::OneHotEncoder encoder;
  std::cout << "learned before = " << std::boolalpha << encoder.learned() << "\n";

  encoder.features({"region"}).learn(train);
  std::cout << "learned after  = " << encoder.learned() << "\n\n";

  std::cout << "map(train) -- one indicator column per category, plus a reserved\n"
               "region_unknown column (include_unknown_bucket defaults to true):\n"
            << encoder.map(train) << "\n";
  std::cout << "map(later) -- same fitted layout replayed on new rows:\n"
            << encoder.map(later) << "\n";
  lymph::prep::OneHotEncoder fresh;
  std::cout << "learn_map(train) -- fit and apply in one call:\n"
            << fresh.features({"region"}).learn_map(train);

  return 0;
}
