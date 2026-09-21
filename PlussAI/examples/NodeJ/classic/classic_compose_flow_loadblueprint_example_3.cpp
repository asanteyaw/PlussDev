#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  const std::string v1_text = "pluss_flow_blueprint_v1\nstandard_scaler|-|pass|\n";
  cls::Flow flow;
  const cls::Status loaded = flow.load_blueprint(v1_text);
  std::cout << "ok = " << std::boolalpha << loaded.ok() << "\n";
  std::cout << flow.flow_summary() << "\n";

  return 0;
}
