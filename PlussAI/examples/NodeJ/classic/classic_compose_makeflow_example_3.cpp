// compose::make_flow example 3 -- a mismatched ModelSpec.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::ModelSpec mismatched{};
  mismatched.task = cls::TaskKind::Classification;
  mismatched.family = cls::ModelFamily::Tree;
  mismatched.options = cls::LinearFamilyOptions{};  // says Tree, holds Linear

  const cls::StatusOr<cls::Flow> made = cls::make_flow(mismatched);
  std::cout << "ok = " << std::boolalpha << made.ok() << "\n";
  std::cout << "message = \"" << made.status().message << "\"\n";

  return 0;
}
