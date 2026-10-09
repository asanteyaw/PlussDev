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

  // Three distinct categories over six rows.
  const pluss::Series region = pluss::make_series({0, 1, 2, 0, 1, 2}, {6}, f64);
  const DataFrame frame({{"region", Slate(region)}});

  // The stage factory and the standalone prep::OneHotEncoder take the same
  // option struct. The standalone object shows the expansion directly.
  lymph::prep::OneHotEncoder encoder{lymph::options::OneHotEncoder{}};
  encoder.features({"region"});  // name the categorical column on a DataFrame
  const DataFrame expanded = encoder.learn_map(frame);

  std::cout << "one input column -> " << expanded.width() << " indicator columns\n\n";
  std::cout << expanded;

  return 0;
}
