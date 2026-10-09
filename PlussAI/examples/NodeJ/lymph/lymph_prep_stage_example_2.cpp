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

  // A Stage carries its transform's options. The umbrella spelling
  // lymph::options::StandardScaler and the family spelling
  // lymph::prep::StandardScalerOptions name the same struct.
  lymph::options::StandardScaler centered{};
  lymph::options::StandardScaler spread_only{};
  spread_only.with_centering = false;  // divide by spread only; keep the mean

  // The identical struct feeds the attachable stage factory and the
  // standalone object -- same arithmetic, different return type and role.
  const lymph::prep::Stage staged = lymph::prep::stage::standard_scaler(spread_only);
  std::cout << "staged is a standard-scaler stage: " << std::boolalpha
            << (staged.kind == lymph::TransformKind::StandardScaler) << "\n\n";

  lymph::prep::StandardScaler a{centered};
  lymph::prep::StandardScaler b{spread_only};

  std::cout << "with_centering = true (default):\n";
  std::cout << a.learn_map(frame) << "\n";
  std::cout << "with_centering = false:\n";
  std::cout << b.learn_map(frame);

  return 0;
}
