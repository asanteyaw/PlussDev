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

  // with_centering toggles whether the column mean is subtracted. The stage
  // factory and the standalone prep::StandardScaler take the identical option
  // struct; here the standalone object makes the numeric effect visible.
  lymph::options::StandardScaler with_center{};                 // default: true
  lymph::options::StandardScaler spread_only{.with_centering = false};

  // A stage built from the same options is a valid descriptor either way.
  const lymph::prep::Stage staged = lymph::prep::stage::standard_scaler(spread_only);
  std::cout << "staged kind is StandardScaler: " << std::boolalpha
            << (staged.kind == lymph::TransformKind::StandardScaler) << "\n\n";

  std::cout << "with_centering = true (mean removed, values straddle 0):\n";
  std::cout << lymph::prep::StandardScaler{with_center}.learn_map(frame) << "\n";

  std::cout << "with_centering = false (mean kept, values stay positive):\n";
  std::cout << lymph::prep::StandardScaler{spread_only}.learn_map(frame);

  return 0;
}
