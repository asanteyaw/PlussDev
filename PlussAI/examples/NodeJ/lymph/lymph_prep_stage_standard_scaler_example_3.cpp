#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series price = pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0}, {5}, f64);
  const DataFrame frame({{"price", Thread(price)}});

  // with_scaling toggles the divide-by-spread step. Turn it off to centre the
  // column on zero without changing its unit of measure. Family-alias spelling
  // for the option struct this time.
  lymph::prep::StandardScalerOptions centre_only{};
  centre_only.with_scaling = false;

  std::cout << "with_scaling = false (mean removed, spread unchanged):\n";
  std::cout << lymph::prep::StandardScaler{centre_only}.learn_map(frame) << "\n";

  // Both toggles off is an identity map -- the column passes through untouched.
  lymph::prep::StandardScalerOptions identity{};
  identity.with_centering = false;
  identity.with_scaling = false;
  std::cout << "both toggles off (identity):\n";
  std::cout << lymph::prep::StandardScaler{identity}.learn_map(frame);

  return 0;
}
