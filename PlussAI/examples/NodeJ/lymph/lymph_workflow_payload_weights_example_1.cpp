#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// weights(std::string feature_name): read per-row training weights from a
// named column. The column is not treated as a feature. Because Policy row
// filtering can drop rows, a frame-backed Payload must name the weight
// column (rather than pass a Series) so the weights stay row-aligned.
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
      {"row_weight", Slate(pluss::make_series({1.0, 1.0, 1.0, 3.0, 3.0, 3.0}, {6}, f64))},
      {"default", Slate(pluss::make_series({0, 0, 0, 1, 1, 1}, {6}, f64))},
  });

  const workflow::Payload payload = workflow::payload::make(raw)
                                        .features({"income", "balance"})
                                        .label("default")
                                        .weights("row_weight");

  std::cout << "has_weights()          = " << std::boolalpha << payload.has_weights() << "\n";
  std::cout << "has_weight_feature()   = " << payload.has_weight_feature() << "\n";
  std::cout << "has_external_weights() = " << payload.has_external_weights() << "\n";

  const lymph::LabeledBatch batch = payload.labeled_batch();
  std::cout << "labeled_batch().has_weights() = " << batch.has_weights() << "\n";

  const workflow::Workflow trained =
      workflow::make(lymph::models::logistic_regression()).learn(payload);
  std::cout << "trained.trained() = " << trained.trained() << "\n";

  return 0;
}
