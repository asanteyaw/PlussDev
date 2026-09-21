// compose::Flow::add_named_transform example 3 -- a kind / options mismatch:
// passing OneHotEncoderOptions for TransformKind::StandardScaler.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::Flow flow{};
  const cls::Status s = flow.add_named_transform(
      cls::TransformKind::StandardScaler, cls::prep::OneHotEncoderOptions{});
  std::cout << "ok = " << std::boolalpha << s.ok() << "\n";
  std::cout << "message = \"" << s.message << "\"\n";

  return 0;
}
