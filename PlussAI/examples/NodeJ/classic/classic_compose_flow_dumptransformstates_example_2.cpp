#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::Flow flow;  // no stages
  const cls::StatusOr<std::vector<std::vector<pluss::Series>>> states = flow.dump_transform_states();
  std::cout << "ok    = " << std::boolalpha << states.ok() << "\n";
  std::cout << "stages = " << states.value().size() << "\n";

  return 0;
}
