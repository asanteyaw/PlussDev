// compose::Flow::set_stage_key example 1 -- naming a stage after
// construction, read back via transform_specs().

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::Flow flow{};
  flow.add_standard_scaler({}, {0});
  const cls::Status s = flow.set_stage_key(0, "income_scaler");
  std::cout << "ok = " << std::boolalpha << s.ok() << "\n";
  std::cout << "stage_key = " << flow.transform_specs().front().stage_key << "\n";

  return 0;
}
