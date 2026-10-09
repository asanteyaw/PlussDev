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

  // degree sets the highest power; interactions_only drops pure powers and keeps
  // only cross terms; include_bias adds a constant column. Standalone view.
  const pluss::Series a = pluss::make_series({1., 2., 3.}, {3}, f64);
  const pluss::Series b = pluss::make_series({4., 5., 6.}, {3}, f64);
  const DataFrame frame({{"a", Slate(a)}, {"b", Slate(b)}});

  lymph::options::PolynomialLift deg2{};                               // default
  lymph::options::PolynomialLift inter{};
  inter.interactions_only = true;
  lymph::options::PolynomialLift bias{};
  bias.include_bias = true;

  std::cout << "degree 2:\n"           << lymph::prep::PolynomialLift{deg2}.learn_map(frame) << "\n";
  std::cout << "interactions_only:\n"  << lymph::prep::PolynomialLift{inter}.learn_map(frame) << "\n";
  std::cout << "include_bias:\n"       << lymph::prep::PolynomialLift{bias}.learn_map(frame);

  return 0;
}
