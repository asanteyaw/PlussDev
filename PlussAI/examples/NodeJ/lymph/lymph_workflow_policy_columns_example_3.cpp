#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// forbid(names): a "this is leakage if it reaches here" guard. A forbidden
// column present in the frame is a hard error -- unless it is also listed in
// drop(), which says "yes, it is here on purpose, strip it".
int main() {
  namespace lymph = pluss::ai::lymph;
  namespace workflow = lymph::workflow;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame raw({
      {"income", Thread(pluss::make_series({1.0, 1.4, 1.8, 2.2, 3.8, 4.2}, {6}, f64))},
      {"future_outcome", Thread(pluss::make_series({0, 0, 0, 1, 1, 1}, {6}, f64))},
      {"default", Thread(pluss::make_series({0, 0, 0, 0, 1, 1}, {6}, f64))},
  });

  workflow::Policy strict{};
  strict.forbid({"future_outcome"});
  try {
    (void)strict.apply(raw);
    std::cout << "forbid alone: no error (unexpected)\n";
  } catch (const std::exception& e) {
    std::cout << "forbid alone threw: " << e.what() << "\n";
  }

  // forbid + drop the same column: acknowledged, stripped, no error.
  workflow::Policy acknowledged{};
  acknowledged.forbid({"future_outcome"}).drop({"future_outcome"});
  const DataFrame admitted = acknowledged.apply(raw);
  std::cout << "forbid + drop: admitted_features=" << admitted.nfeatures() << "\n";

  return 0;
}
