#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::Flow flow;
  flow.add_standard_scaler();
  flow.set_stage_key(0, "prep");
  std::cout << "before: " << flow.flow_summary() << "\n";

  flow.set_nested_param("prep::kind", static_cast<double>(static_cast<int>(cls::TransformKind::MinMaxScaler)));
  std::cout << "after:  " << flow.flow_summary() << "\n";

  return 0;
}
