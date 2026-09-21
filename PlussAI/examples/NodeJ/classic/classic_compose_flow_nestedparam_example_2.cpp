#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::Flow flow;
  flow.add_standard_scaler();

  const cls::StatusOr<double> bad_shape = flow.nested_param("not_shaped_right");
  std::cout << "bad shape: ok = " << std::boolalpha << bad_shape.ok()
            << ", message = \"" << bad_shape.status().message << "\"\n";

  const cls::StatusOr<double> bad_stage = flow.nested_param("nonexistent::route_size");
  std::cout << "bad stage: ok = " << bad_stage.ok()
            << ", message = \"" << bad_stage.status().message << "\"\n";

  return 0;
}
