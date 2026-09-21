// compose::Flow::add_stage example 2 -- a non-default options struct, and an
// unrouted stage.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::prep::SimpleImputerOptions imputer{};
  imputer.strategy = cls::prep::SimpleImputerStrategy::Median;

  cls::Flow flow{};
  const cls::Status s = flow.add_simple_imputer(imputer);  // no route -- every column
  std::cout << "ok = " << std::boolalpha << s.ok() << "\n";

  const std::vector<cls::TransformSpec>& specs = flow.transform_specs();
  std::cout << "feature_route.empty() = " << specs.front().feature_route.empty() << "\n";

  return 0;
}
