#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::StatusOr<cls::Flow> made = cls::make_flow(
      cls::ModelSpec{cls::TaskKind::Regression, cls::ModelFamily::Linear, cls::LinearFamilyOptions{}});
  cls::Flow flow = made.move_value();  // has a model, never learned

  const cls::StatusOr<std::vector<pluss::Series>> state = flow.dump_model_state();
  std::cout << "trained() = " << std::boolalpha << flow.trained() << "\n";
  std::cout << "dump ok   = " << state.ok() << "\n";
  std::cout << "tensors   = " << state.value().size() << "\n";

  return 0;
}
