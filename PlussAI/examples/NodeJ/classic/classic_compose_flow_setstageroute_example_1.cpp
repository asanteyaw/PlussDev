// compose::Flow::set_stage_route example 1 -- re-routing an existing stage
// to different columns.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::Flow flow{};
  flow.add_standard_scaler({}, {0});
  std::cout << "route size before = " << flow.transform_specs().front().feature_route.size() << "\n";

  const cls::Status s = flow.set_stage_route(0, {0, 1});
  std::cout << "ok = " << std::boolalpha << s.ok() << "\n";
  std::cout << "route size after = " << flow.transform_specs().front().feature_route.size() << "\n";

  return 0;
}
