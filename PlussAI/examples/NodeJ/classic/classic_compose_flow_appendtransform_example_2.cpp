// compose::Flow::append_transform example 2 -- a null transformer is
// rejected.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::Flow flow{};
  const cls::Status s = flow.append_transform(nullptr);
  std::cout << "ok = " << std::boolalpha << s.ok() << "\n";
  std::cout << "message = \"" << s.message << "\"\n";
  std::cout << "stage_count() = " << flow.stage_count() << "\n";

  return 0;
}
