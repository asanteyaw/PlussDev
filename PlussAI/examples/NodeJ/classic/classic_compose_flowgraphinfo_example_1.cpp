// compose::FlowGraphInfo example 1 -- a two-stage flow with a model,
// inspected through graph_info().

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::LinearFamilyOptions linear{};
  linear.objective = cls::LinearObjective::Logistic;
  cls::Flow flow = cls::make_flow(
      cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, linear}).move_value();
  flow.add_standard_scaler({}, {0});
  flow.add_one_hot_encoder({}, {1});

  const cls::FlowGraphInfo info = flow.graph_info().value();
  std::cout << "task = " << static_cast<int>(info.task) << "\n";
  std::cout << "trained = " << std::boolalpha << info.trained << "\n";
  std::cout << "has_model = " << info.has_model << "\n";
  std::cout << "stage count = " << info.stages.size() << "\n";
  for (const auto& stage : info.stages) {
    std::cout << "  stage " << stage.index << ": kind = " << static_cast<int>(stage.kind)
              << ", routed_feature_count = " << stage.routed_feature_count
              << ", route_active = " << stage.route_active << "\n";
  }

  return 0;
}
