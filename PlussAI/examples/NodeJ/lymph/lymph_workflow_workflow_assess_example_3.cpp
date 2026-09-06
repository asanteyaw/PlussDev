#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  namespace workflow = lymph::workflow;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame x({
      {"income", Thread(pluss::make_series({1.0, 1.3, 1.7, 2.1}, {4}, f64))},
      {"balance", Thread(pluss::make_series({0.2, 0.4, 0.5, 0.9}, {4}, f64))},
  });
  const pluss::Series y = pluss::make_series({0, 0, 1, 1}, {4}, f64);

  // Same restriction as learn_(x, y): a Policy that can drop rows is
  // rejected on assess(x, y), because dropping a row from x alone would
  // desynchronize it from the already-separate y.
  workflow::Policy policy{};
  policy.reject_any_missing({"income"});

  const workflow::Workflow trained =
      workflow::make(lymph::models::logistic_regression()).learn(x, y);
  const workflow::Workflow with_policy = workflow::make(trained.model(), policy);

  try {
    (void)with_policy.assess(x, y);
    std::cout << "assess(x, y): no error (unexpected)\n";
  } catch (const std::exception& e) {
    std::cout << "assess(x, y) threw: " << e.what() << "\n";
  }

  return 0;
}
