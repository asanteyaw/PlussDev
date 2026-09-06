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

  const DataFrame raw({
      {"income", Thread(pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4}, f64))},
      {"balance", Thread(pluss::make_series({10.0, 20.0, 30.0, 40.0}, {4}, f64))},
  });

  // map (unlike learn_map) only ever replays preparation that has already
  // been fitted -- it never fits anything itself. A stage attached through
  // on(...).add(...) but never resolved by a prior learn_/learn/learn_map
  // call is still "pending," and map fails loudly rather than silently
  // passing raw features through.
  workflow::Workflow wf = workflow::make(lymph::models::logistic_regression());
  wf.on({"income", "balance"}).add(lymph::prep::stage::standard_scaler());

  try {
    (void)wf.map(raw);
    std::cout << "map: no error (unexpected)\n";
  } catch (const std::exception& e) {
    std::cout << "map threw: " << e.what() << "\n";
  }

  return 0;
}
