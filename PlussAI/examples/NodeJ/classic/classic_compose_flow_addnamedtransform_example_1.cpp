// compose::Flow::add_named_transform example 1 -- building a stage from a
// kind value read as if from configuration, rather than naming the typed
// method directly.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  const cls::TransformKind kind = cls::TransformKind::StandardScaler;  // "read from configuration"
  const cls::TransformOptions options = cls::prep::StandardScalerOptions{};

  cls::Flow flow{};
  const cls::Status s = flow.add_named_transform(kind, options);
  std::cout << "ok = " << std::boolalpha << s.ok() << "\n";
  std::cout << "stage_count() = " << flow.stage_count() << "\n";

  return 0;
}
