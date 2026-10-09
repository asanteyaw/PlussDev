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

  // mode picks Yeo-Johnson (handles zero and negative values) or Box-Cox
  // (strictly positive input). standardize toggles the trailing z-score step.
  const pluss::Series v = pluss::make_series({1., 2., 3., 5., 8., 13., 21., 34.}, {8}, f64);
  const DataFrame frame({{"v", Slate(v)}});

  lymph::options::PowerMap yj{};                                       // default: YeoJohnson, standardize=true
  lymph::options::PowerMap yj_raw{};
  yj_raw.standardize = false;

  std::cout << "YeoJohnson + standardize:\n" << lymph::prep::PowerMap{yj}.learn_map(frame) << "\n";
  std::cout << "YeoJohnson, standardize off:\n" << lymph::prep::PowerMap{yj_raw}.learn_map(frame);

  return 0;
}
