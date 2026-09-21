// compose::Flow::set_stage_remainder example 2 -- an out-of-range stage
// index is rejected.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::Flow flow{};
  flow.add_standard_scaler({}, {0});

  const cls::Status s = flow.set_stage_remainder(1, cls::RouteRemainderPolicy::Drop);
  std::cout << "ok = " << std::boolalpha << s.ok() << "\n";
  std::cout << "message = \"" << s.message << "\"\n";

  return 0;
}
