#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::Flow source;
  source.add_standard_scaler();
  const std::vector<pluss::Series> empty_state = source.dump_model_state().value();

  cls::Flow target;
  target.add_standard_scaler();
  const cls::Status loaded = target.load_model_state(empty_state);
  std::cout << "ok = " << std::boolalpha << loaded.ok() << "\n";

  return 0;
}
