#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>
#include <memory>

int main() {
  namespace cls = pluss::ai::classic;

  cls::Flow flow;
  flow.append_transform(std::make_unique<cls::prep::StandardScaler>());
  const cls::StatusOr<std::string> blueprint = flow.save_blueprint();
  std::cout << "ok = " << std::boolalpha << blueprint.ok() << "\n";
  std::cout << "message = \"" << blueprint.status().message << "\"\n";

  return 0;
}
