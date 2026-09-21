#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::Flow source;
  source.add_one_hot_encoder({}, {1});
  const std::string blueprint = source.save_blueprint().value();

  cls::Flow target;
  target.add_standard_scaler();  // a different stage -- discarded on load
  std::cout << "target stage_count before load = " << target.stage_count() << "\n";
  const cls::Status loaded = target.load_blueprint(blueprint);
  std::cout << "load ok = " << std::boolalpha << loaded.ok() << "\n";
  std::cout << "target stage_count after load  = " << target.stage_count() << "\n";
  std::cout << target.flow_summary() << "\n";

  return 0;
}
