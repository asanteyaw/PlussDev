// compose::Flow::set_stage_remainder example 1 -- switching an existing
// stage from the default PassThrough to Drop.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::Flow flow{};
  flow.add_standard_scaler({}, {0});
  std::cout << "remainder before == PassThrough: " << std::boolalpha
            << (flow.transform_specs().front().remainder == cls::RouteRemainderPolicy::PassThrough) << "\n";

  const cls::Status s = flow.set_stage_remainder(0, cls::RouteRemainderPolicy::Drop);
  std::cout << "ok = " << s.ok() << "\n";
  std::cout << "remainder after == Drop: "
            << (flow.transform_specs().front().remainder == cls::RouteRemainderPolicy::Drop) << "\n";

  return 0;
}
