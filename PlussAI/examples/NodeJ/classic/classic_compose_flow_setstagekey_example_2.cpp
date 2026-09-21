// compose::Flow::set_stage_key example 2 -- duplicate stage_key values are
// rejected; an out-of-range index is rejected the same way.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::Flow flow{};
  flow.add_standard_scaler({}, {0});
  flow.add_one_hot_encoder({}, {1});
  flow.set_stage_key(0, "scale_col0");

  const cls::Status dup = flow.set_stage_key(1, "scale_col0");
  std::cout << "duplicate key ok = " << std::boolalpha << dup.ok() << "\n";
  std::cout << "duplicate key message = \"" << dup.message << "\"\n";

  const cls::Status out_of_range = flow.set_stage_key(5, "whatever");
  std::cout << "out-of-range ok = " << out_of_range.ok() << "\n";
  std::cout << "out-of-range message = \"" << out_of_range.message << "\"\n";

  return 0;
}
