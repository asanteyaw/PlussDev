#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::Flow flow;
  const cls::Status empty = flow.load_blueprint("");
  std::cout << "empty:  ok = " << std::boolalpha << empty.ok()
            << ", message = \"" << empty.message << "\"\n";

  const cls::Status bad_header = flow.load_blueprint("not_a_real_blueprint\n");
  std::cout << "header: ok = " << bad_header.ok()
            << ", message = \"" << bad_header.message << "\"\n";

  return 0;
}
