#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::LinearFamilyOptions linear{};
  linear.objective = cls::LinearObjective::Logistic;
  cls::StatusOr<cls::Flow> made = cls::make_flow(
      cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, linear});
  cls::Flow flow = made.move_value();
  flow.add_standard_scaler({}, {0});
  flow.add_one_hot_encoder({}, {1});
  flow.set_stage_key(0, "scale");
  flow.set_stage_key(1, "encode");

  std::cout << flow.flow_summary() << "\n";

  return 0;
}
