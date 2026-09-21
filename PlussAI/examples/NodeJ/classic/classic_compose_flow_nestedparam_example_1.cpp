#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::LinearFamilyOptions linear{};
  linear.objective = cls::LinearObjective::Logistic;
  linear.regularization_strength = 0.5;
  cls::StatusOr<cls::Flow> made = cls::make_flow(
      cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, linear});
  cls::Flow flow = made.move_value();
  flow.add_standard_scaler({}, {0});
  flow.set_stage_key(0, "scale");

  std::cout << "scale::route_size          = " << flow.nested_param("scale::route_size").value() << "\n";
  std::cout << "scale::opt.variance_floor  = " << flow.nested_param("scale::opt.variance_floor").value() << "\n";
  std::cout << "model::opt.regularization_strength = "
            << flow.nested_param("model::opt.regularization_strength").value() << "\n";

  return 0;
}
