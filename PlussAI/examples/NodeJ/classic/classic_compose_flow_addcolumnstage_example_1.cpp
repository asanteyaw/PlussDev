// compose::Flow::add_column_stage example 1 -- naming a stage at
// construction, then reading it back by stage_key.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::Flow flow{};
  const cls::Status s = flow.add_column_stage(
      "income_scaler", cls::TransformKind::StandardScaler, cls::prep::StandardScalerOptions{}, {0});
  std::cout << "ok = " << std::boolalpha << s.ok() << "\n";
  std::cout << "stage_key = " << flow.transform_specs().front().stage_key << "\n";

  return 0;
}
