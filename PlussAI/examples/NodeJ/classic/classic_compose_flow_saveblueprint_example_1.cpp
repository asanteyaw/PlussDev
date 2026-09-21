#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::Flow original;
  original.add_standard_scaler({}, {0});
  original.add_one_hot_encoder({}, {1});
  original.set_stage_key(0, "scale");
  original.set_stage_key(1, "encode");

  const std::string blueprint = original.save_blueprint().value();
  std::cout << blueprint;

  cls::Flow restored;
  restored.load_blueprint(blueprint);
  std::cout << "restored stage_count = " << restored.stage_count() << "\n";
  std::cout << (restored.flow_summary() == original.flow_summary() ? "summaries match\n" : "summaries differ\n");

  return 0;
}
