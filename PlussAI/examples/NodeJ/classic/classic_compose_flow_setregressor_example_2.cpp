// compose::Flow::set_regressor example 2 -- a null model is rejected.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::Flow flow{};
  const cls::Status s = flow.set_regressor(nullptr);
  std::cout << "ok = " << std::boolalpha << s.ok() << "\n";
  std::cout << "message = \"" << s.message << "\"\n";

  return 0;
}
