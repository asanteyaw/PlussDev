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
      {"default", Thread(pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64))},
  });

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"income", "balance"};
  data_options.label_feature = "default";

  // with(model) never mutates the receiver -- design stays untrained even
  // after candidate is trained through it, exactly like the value-returning
  // learn(...) leaves its own receiver untouched.
  workflow::Workflow design = workflow::make(lymph::models::logistic_regression());
  design.on({"income", "balance"}).add(lymph::prep::stage::standard_scaler());
  design.learn_(raw, data_options);
  std::cout << "design.trained() before with(...) = " << std::boolalpha << design.trained() << "\n";

  const workflow::Workflow candidate = design.with(lymph::models::logistic_regression());
  std::cout << "design.trained() after with(...)   = " << design.trained() << "  (unchanged)\n";
  std::cout << "candidate.trained()                = " << candidate.trained() << "  (fresh base model)\n";

  const workflow::Workflow retrained = candidate.learn(raw, data_options);
  std::cout << "retrained.trained()                = " << retrained.trained() << "\n";

  return 0;
}
