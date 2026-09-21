// compose::Flow::add_stage example 1 -- two typed stages, each routed to one
// column.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::Flow flow{};
  const cls::Status s1 = flow.add_standard_scaler({}, {0});
  const cls::Status s2 = flow.add_one_hot_encoder({}, {1});

  std::cout << "s1.ok() = " << std::boolalpha << s1.ok() << "\n";
  std::cout << "s2.ok() = " << s2.ok() << "\n";
  std::cout << "stage_count() = " << flow.stage_count() << "\n";

  return 0;
}
