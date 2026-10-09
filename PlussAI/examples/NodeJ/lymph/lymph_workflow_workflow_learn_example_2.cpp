#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  namespace workflow = lymph::workflow;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame raw({
      {"income", Slate(pluss::make_series({1.0, 1.3, 1.7, 2.1, 3.6, 4.0, 4.4, 4.9}, {8}, f64))},
      {"balance", Slate(pluss::make_series({0.2, 0.4, 0.5, 0.9, 1.9, 2.2, 2.5, 2.8}, {8}, f64))},
  });
  const pluss::Series y = pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64);

  // A Payload built with an already-separate label Series, instead of a
  // frame + DFBatch pair -- learn(payload) applies Policy to the frame the
  // same way the raw-frame overloads do, then routes into Model::learn.
  const workflow::Payload payload = workflow::payload::make(raw).features({"income", "balance"}).label(y);

  const workflow::Workflow wf = workflow::make(lymph::models::logistic_regression());
  const workflow::Workflow trained = wf.learn(payload);

  std::cout << "wf.trained()      = " << std::boolalpha << wf.trained()
            << "  (unchanged -- learn(payload) returns a copy)\n";
  std::cout << "trained.trained() = " << trained.trained() << "\n";

  const pluss::Series pred = trained.infer(payload);
  std::cout << "pred = " << pred << "\n";

  return 0;
}
