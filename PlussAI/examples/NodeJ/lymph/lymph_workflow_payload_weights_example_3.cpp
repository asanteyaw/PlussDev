#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// The weight-alignment guard: a frame-backed Payload with Series weights but
// no named weight column is rejected by Workflow::learn_, because Policy row
// filtering could drop rows from the frame and leave the weight vector the
// wrong length. Name the column with weights("col") instead.
int main() {
  namespace lymph = pluss::ai::lymph;
  namespace workflow = lymph::workflow;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame raw({
      {"income", Slate(pluss::make_series({1.0, 1.4, 1.8, 2.2, 2.6, 3.0}, {6}, f64))},
      {"balance", Slate(pluss::make_series({0.6, 1.0, 1.3, 1.6, 1.9, 2.2}, {6}, f64))},
      {"default", Slate(pluss::make_series({0, 0, 0, 1, 1, 1}, {6}, f64))},
  });
  const pluss::Series w = pluss::make_series({1.0, 1.0, 1.0, 3.0, 3.0, 3.0}, {6}, f64);

  const workflow::Payload bad = workflow::payload::make(raw)
                                    .features({"income", "balance"})
                                    .label("default")
                                    .weights(w);

  try {
    (void)workflow::make(lymph::models::logistic_regression()).learn(bad);
    std::cout << "learn(bad): no error (unexpected)\n";
  } catch (const std::exception& e) {
    std::cout << "learn(bad) threw: " << e.what() << "\n";
  }

  return 0;
}
