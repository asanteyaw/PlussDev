#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// weights(Series sample_weights): an already-separate weight vector. Use it
// when the weights are computed rather than stored in the frame -- e.g. a
// FeatureBatch-only Payload, or a bare-batch path where Policy row filtering
// is not in play.
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
  });
  const pluss::Series y = pluss::make_series({0, 0, 0, 1, 1, 1}, {6}, f64);
  const pluss::Series w = pluss::make_series({1.0, 1.0, 1.0, 3.0, 3.0, 3.0}, {6}, f64);

  const workflow::Payload payload = workflow::payload::make(raw)
                                        .features({"income", "balance"})
                                        .label(y)
                                        .weights(w);

  std::cout << "has_weights()          = " << std::boolalpha << payload.has_weights() << "\n";
  std::cout << "has_weight_feature()   = " << payload.has_weight_feature() << "\n";
  std::cout << "has_external_weights() = " << payload.has_external_weights() << "\n";

  const lymph::LabeledBatch batch = payload.labeled_batch();
  std::cout << "labeled_batch().has_weights() = " << batch.has_weights() << "\n";

  return 0;
}
