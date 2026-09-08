#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// empty(): true only when every rule is at its default. A Workflow built
// with an empty Policy is a pure pass-through -- apply() returns the frame
// unchanged.
int main() {
  namespace lymph = pluss::ai::lymph;
  namespace workflow = lymph::workflow;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame raw({
      {"income", Thread(pluss::make_series({1.0, 1.4, 1.8, 2.2}, {4}, f64))},
      {"balance", Thread(pluss::make_series({0.2, 0.4, 0.6, 0.9}, {4}, f64))},
  });

  workflow::Policy blank{};
  std::cout << "Policy{}.empty()        = " << std::boolalpha << blank.empty() << "\n";
  const DataFrame passed = blank.apply(raw);
  std::cout << "pass-through unchanged  = "
            << (passed.nfeatures() == raw.nfeatures() && passed.ncases() == raw.ncases()) << "\n";

  workflow::Policy one_rule{};
  one_rule.drop({"balance"});
  std::cout << "after drop(): empty()   = " << one_rule.empty() << "\n";

  return 0;
}
