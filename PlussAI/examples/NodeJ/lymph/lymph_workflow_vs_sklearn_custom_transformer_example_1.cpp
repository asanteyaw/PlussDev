#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// The lymph equivalent of sklearn's FunctionTransformer / a hand-written
// class-based custom transformer: prep::stage::custom(name, fn, contract)
// wraps any Series -> Series callable as a stage, no subclassing needed.
// Two things are different from a routed builtin stage, both shown here:
// it attaches at the whole-Workflow level (Workflow::add, not a branch's
// .add(...) -- there is no per-column routing for it), and it makes the
// Workflow non-cloneable, so only the in-place learn_ works with it; the
// value-returning learn(...) clones first and rejects it.
int main() {
  namespace lymph = pluss::ai::lymph;
  namespace workflow = lymph::workflow;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame raw({
      {"income", Thread(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0}, {6}, f64))},
      {"balance", Thread(pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0, 60.0}, {6}, f64))},
      {"target", Thread(pluss::make_series({0, 0, 0, 1, 1, 1}, {6}, f64))},
  });

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"income", "balance"};
  data_options.label_feature = "target";

  const lymph::prep::CustomStage double_features = lymph::prep::stage::custom(
      "double",
      [](const pluss::Series& input) -> pluss::Series { return input.mul(2.0); },
      lymph::prep::TransformContract{});

  workflow::Workflow wf = workflow::make(lymph::models::logistic_regression());
  wf.add(double_features);
  wf.learn_(raw, data_options);
  std::cout << "has_opaque_transforms = " << std::boolalpha << wf.has_opaque_transforms() << "\n";
  std::cout << "pred = " << wf.infer(raw, data_options) << "\n";

  workflow::Workflow wf2 = workflow::make(lymph::models::logistic_regression());
  wf2.add(double_features);
  try {
    (void)wf2.learn(raw, data_options);
    std::cout << "learn(...): no error (unexpected)\n";
  } catch (const std::exception& e) {
    std::cout << "learn(...) threw: " << e.what() << "\n";
  }

  return 0;
}
