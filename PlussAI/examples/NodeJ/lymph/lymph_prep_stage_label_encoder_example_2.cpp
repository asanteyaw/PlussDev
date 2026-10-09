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

  // Standalone, showing the 0-based codes. label_encoder and ordinal_encoder
  // share their option struct; the difference is the one_based default.
  const pluss::Series band = pluss::make_series({5, 9, 5, 1, 9, 1}, {6}, f64);
  const DataFrame frame({{"band", Slate(band)}});

  lymph::prep::LabelEncoder enc{lymph::options::LabelEncoder{}};
  enc.features({"band"});
  std::cout << enc.learn_map(frame);

  return 0;
}
