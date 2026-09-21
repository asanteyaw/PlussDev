#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>
#include <memory>

int main() {
  namespace cls = pluss::ai::classic;

  cls::Flow flow;
  flow.append_transform(std::make_unique<cls::prep::StandardScaler>());
  const cls::StatusOr<cls::Flow> cloned = flow.clone();
  std::cout << "ok = " << std::boolalpha << cloned.ok() << "\n";
  std::cout << "message = \"" << cloned.status().message << "\"\n";

  return 0;
}
