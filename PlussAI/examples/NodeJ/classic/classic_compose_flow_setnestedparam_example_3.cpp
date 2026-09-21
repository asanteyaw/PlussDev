#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::Flow flow;
  flow.add_standard_scaler({}, {0});
  flow.set_stage_key(0, "scale");

  const cls::Status set = flow.set_nested_param("scale::route_size", 5.0);
  std::cout << "ok = " << std::boolalpha << set.ok() << "\n";
  std::cout << "message = \"" << set.message << "\"\n";

  return 0;
}
