#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::Flow flow;
  flow.add_standard_scaler();
  for (const std::string& key : flow.nested_param_keys()) {
    std::cout << key << "\n";
  }

  return 0;
}
