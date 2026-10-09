// options example 3 -- a prep-stage options struct, reached through its
// options:: alias. options::SimpleImputer is prep::SimpleImputerOptions; the
// `strategy` field decides what fills a missing cell, `constant_value` supplies
// the fill when strategy == Constant.

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

  const DataFrame frame({
      {"score", Slate(pluss::make_series(
          {1.0, 2.0, std::nan(""), 4.0, 5.0}, {5}, f64))},
  });

  lymph::options::SimpleImputer median{};
  median.strategy = lymph::prep::SimpleImputerStrategy::Median;

  lymph::options::SimpleImputer constant{};
  constant.strategy = lymph::prep::SimpleImputerStrategy::Constant;
  constant.constant_value = -1.0;

  lymph::prep::SimpleImputer by_median(median);
  by_median.features({"score"});
  const DataFrame filled_median = by_median.learn_map(frame);

  lymph::prep::SimpleImputer by_constant(constant);
  by_constant.features({"score"});
  const DataFrame filled_constant = by_constant.learn_map(frame);

  std::cout << "median fill value  = " << by_median.summary().statistics << "\n";
  std::cout << "median result      = " << filled_median.features("score").as_series() << "\n";
  std::cout << "constant result    = " << filled_constant.features("score").as_series() << "\n";

  return 0;
}
