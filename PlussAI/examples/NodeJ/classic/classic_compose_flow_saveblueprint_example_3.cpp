#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::Flow flow;
  const std::string blueprint = flow.save_blueprint().value();
  std::cout << "blueprint = \"" << blueprint << "\"\n";

  cls::Flow restored;
  const cls::Status loaded = restored.load_blueprint(blueprint);
  std::cout << "load ok = " << std::boolalpha << loaded.ok() << "\n";
  std::cout << "restored stage_count = " << restored.stage_count() << "\n";

  return 0;
}
