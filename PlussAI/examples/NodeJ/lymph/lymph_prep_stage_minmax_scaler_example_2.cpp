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

  const pluss::Series price = pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0}, {5}, f64);
  const DataFrame frame({{"price", Slate(price)}});

  // lower / upper set the target range. The default is [0, 1]; here a second
  // scaler targets [-1, 1]. The stage factory and the standalone object take
  // the identical option struct.
  lymph::options::MinMaxScaler unit{};                       // [0, 1]
  lymph::options::MinMaxScaler signed_range{.lower = -1.0, .upper = 1.0};

  const lymph::prep::Stage staged = lymph::prep::stage::minmax_scaler(signed_range);
  std::cout << "staged is a MinMaxScaler stage: " << std::boolalpha
            << (staged.kind == lymph::TransformKind::MinMaxScaler) << "\n\n";

  std::cout << "default range [0, 1]:\n";
  std::cout << lymph::prep::MinMaxScaler{unit}.learn_map(frame) << "\n";
  std::cout << "range [-1, 1]:\n";
  std::cout << lymph::prep::MinMaxScaler{signed_range}.learn_map(frame);

  return 0;
}
