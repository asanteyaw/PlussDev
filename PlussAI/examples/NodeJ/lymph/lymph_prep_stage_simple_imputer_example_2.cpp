#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <cmath>
#include <iostream>

int main() {

  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const double NaN = std::nan("");

  // strategy selects the fill statistic. Standalone view over one gappy column:
  // mean, median, and a fixed constant.
  const pluss::Series v = pluss::make_series({1., 2., 100., NaN, 4., NaN}, {6}, f64);
  const DataFrame frame({{"v", Slate(v)}});

  lymph::options::SimpleImputer mean_opt{};                                  // default: Mean
  lymph::options::SimpleImputer median_opt{};
  median_opt.strategy = lymph::prep::SimpleImputerStrategy::Median;
  lymph::options::SimpleImputer const_opt{};
  const_opt.strategy = lymph::prep::SimpleImputerStrategy::Constant;
  const_opt.constant_value = -1.0;

  std::cout << "Mean:\n"     << lymph::prep::SimpleImputer{mean_opt}.learn_map(frame) << "\n";
  std::cout << "Median:\n"   << lymph::prep::SimpleImputer{median_opt}.learn_map(frame) << "\n";
  std::cout << "Constant -1:\n" << lymph::prep::SimpleImputer{const_opt}.learn_map(frame);

  return 0;
}
