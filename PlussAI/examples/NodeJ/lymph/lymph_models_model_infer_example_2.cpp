#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  // Rows 2 and 3 sit right on the boundary and carry mixed labels, so the
  // fitted scores near the middle are close to the decision point.
  const pluss::Series income = pluss::make_series({1.0, 1.8, 2.6, 2.8, 3.6, 4.4}, {6}, f64);
  const pluss::Series balance = pluss::make_series({0.5, 1.0, 1.5, 1.6, 2.1, 2.6}, {6}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 1, 0, 1, 1}, {6}, f64);
  const DataFrame x({{"income", Slate(income)}, {"balance", Slate(balance)}});

  const lymph::models::Model model = lymph::models::logistic_regression().learn(x, y);

  // infer_scores is the raw decision score. infer thresholds it: by default a
  // score above the point that corresponds to probability 0.5 becomes label 1.
  // Raising positive_threshold makes the model demand more evidence before it
  // says 1, so borderline rows flip back to 0.
  lymph::options::Infer strict{};
  strict.positive_threshold = 0.95;

  const DataFrame compare({
      {"actual", Slate(y)},
      {"score", Slate(model.infer_scores(x))},
      {"label_at_0_5", Slate(model.infer(x))},
      {"label_at_0_95", Slate(model.infer(x, strict))},
  });
  std::cout << compare;

  return 0;
}
