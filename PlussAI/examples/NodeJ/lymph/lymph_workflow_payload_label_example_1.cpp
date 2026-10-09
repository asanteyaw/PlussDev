#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// label(std::string feature_name): supervise a frame-backed Payload by
// naming the label column. This is the everyday form -- it works with every
// raw-frame Workflow verb.
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

  const workflow::Payload payload = workflow::payload::make(raw)
                                        .features({"income", "balance"})
                                        .label("default");

  std::cout << "has_label()         = " << std::boolalpha << payload.has_label() << "\n";
  std::cout << "has_label_feature() = " << payload.has_label_feature() << "\n";

  const lymph::LabeledBatch batch = payload.labeled_batch();
  std::cout << "labeled_batch: rows=" << batch.features.sample_count()
            << " features=" << batch.features.feature_count() << "\n";

  // The Payload flows straight into any Workflow verb.
  const workflow::Workflow trained =
      workflow::make(lymph::models::logistic_regression()).learn(payload);
  std::cout << "trained.trained()   = " << trained.trained() << "\n";

  return 0;
}
