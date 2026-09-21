// compose::Flow::add_column_stage example 3 -- a kind / options mismatch,
// verified independently of add_named_transform's own validation.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::Flow flow{};
  const cls::Status s = flow.add_column_stage(
      "bad_stage", cls::TransformKind::OneHotEncoder, cls::prep::StandardScalerOptions{});
  std::cout << "ok = " << std::boolalpha << s.ok() << "\n";
  std::cout << "message = \"" << s.message << "\"\n";
  std::cout << "stage_count() after the failed add = " << flow.stage_count() << "\n";

  return 0;
}
