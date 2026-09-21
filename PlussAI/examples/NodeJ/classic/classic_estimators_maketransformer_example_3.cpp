// estimators::make_transformer example 3 -- family/options mismatch.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::ModelSpec mismatched{};
  mismatched.task = cls::TaskKind::Transform;
  mismatched.family = cls::ModelFamily::Cluster;
  mismatched.options = cls::DecomposeFamilyOptions{};  // says Cluster, holds Decompose

  const cls::StatusOr<std::unique_ptr<cls::TransformerBase>> made = cls::make_transformer(mismatched);
  std::cout << "ok = " << std::boolalpha << made.ok() << "\n";
  std::cout << "message = \"" << made.status().message << "\"\n";

  return 0;
}
