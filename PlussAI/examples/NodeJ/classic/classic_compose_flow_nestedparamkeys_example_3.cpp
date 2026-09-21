#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::Flow flow;
  flow.add_standard_scaler();
  std::cout << "keys with 1 stage: " << flow.nested_param_keys().size() << "\n";

  flow.add_one_hot_encoder();
  std::cout << "keys with 2 stages: " << flow.nested_param_keys().size() << "\n";
  for (const std::string& key : flow.nested_param_keys()) {
    std::cout << "  " << key << "\n";
  }

  return 0;
}
