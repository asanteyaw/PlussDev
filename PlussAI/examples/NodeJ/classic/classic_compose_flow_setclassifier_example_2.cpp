// compose::Flow::set_classifier example 2 -- a null model is rejected.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::Flow flow{};
  const cls::Status s = flow.set_classifier(nullptr);
  std::cout << "ok = " << std::boolalpha << s.ok() << "\n";
  std::cout << "message = \"" << s.message << "\"\n";
  std::cout << "has_model() = " << flow.has_model() << "\n";

  return 0;
}
