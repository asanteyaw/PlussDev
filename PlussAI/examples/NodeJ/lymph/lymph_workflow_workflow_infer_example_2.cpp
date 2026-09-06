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
      {"income", Thread(pluss::make_series({1.0, 1.3, 1.7, 2.1, 3.6, 4.0, 4.4, 4.9}, {8}, f64))},
      {"balance", Thread(pluss::make_series({0.2, 0.4, 0.5, 0.9, 1.9, 2.2, 2.5, 2.8}, {8}, f64))},
  });
  const pluss::Series y = pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64);

  const workflow::Payload train_payload = workflow::payload::make(raw).features({"income", "balance"}).label(y);
  const workflow::Workflow trained =
      workflow::make(lymph::models::logistic_regression()).learn(train_payload);

  // infer(payload) reads whichever shape the Payload holds; a label role on
  // it, if any, is simply ignored -- only the feature side is used.
  const workflow::Payload query_payload = workflow::payload::make(raw).features({"income", "balance"});
  const pluss::Series pred = trained.infer(query_payload);
  std::cout << "pred = " << pred << "\n";

  return 0;
}
