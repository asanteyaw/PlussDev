#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::LinearFamilyOptions linear{};
  linear.objective = cls::LinearObjective::Logistic;
  cls::Flow flow = cls::make_flow(
      cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, linear}).move_value();
  flow.add_standard_scaler();
  // flow.learn(...) never called -- flow.trained() is false.

  const cls::Status saved = cls::io::save("/tmp/pluss_classic_io_save_example_2.bundle", flow);
  std::cout << "saved ok = " << std::boolalpha << saved.ok() << "\n";
  std::cout << "message = \"" << saved.message << "\"\n";

  return 0;
}
