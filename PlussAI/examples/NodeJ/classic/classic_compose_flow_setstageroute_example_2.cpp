// compose::Flow::set_stage_route example 2 -- an out-of-range stage index is
// rejected; an empty route is accepted (means "every column").

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::Flow flow{};
  flow.add_standard_scaler({}, {0});

  const cls::Status out_of_range = flow.set_stage_route(3, {0});
  std::cout << "out-of-range ok = " << std::boolalpha << out_of_range.ok() << "\n";
  std::cout << "out-of-range message = \"" << out_of_range.message << "\"\n";

  const cls::Status cleared = flow.set_stage_route(0, {});
  std::cout << "clear-route ok = " << cleared.ok() << "\n";
  std::cout << "route.empty() after clearing = " << flow.transform_specs().front().feature_route.empty() << "\n";

  return 0;
}
