#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  // Imbalanced: seven non-defaults, one default, and the classes overlap so
  // the minority row is genuinely hard to separate.
  const pluss::Series income = pluss::make_series({1.0, 1.4, 1.8, 2.2, 2.6, 3.0, 3.4, 2.8}, {8}, f64);
  const pluss::Series balance = pluss::make_series({0.3, 0.6, 0.9, 1.2, 1.5, 1.8, 2.1, 1.6}, {8}, f64);
  const pluss::Series defaulted = pluss::make_series({0, 0, 0, 0, 0, 0, 0, 1}, {8}, f64);

  // The label can be handed in as a Thread lifted straight from a frame
  // column -- the fourth accepted (features, label) input shape.
  const DataFrame frame({
      {"income", Thread(income)},
      {"balance", Thread(balance)},
      {"defaulted", Thread(defaulted)},
  });
  const DataFrame x({{"income", Thread(income)}, {"balance", Thread(balance)}});
  const Thread y = frame.features("defaulted");

  const lymph::models::Model blueprint = lymph::models::logistic_regression();

  // A plain fit is dominated by the seven negative rows.
  lymph::models::Model plain = blueprint.learn(x, y);

  // class_weight = Balanced rescales each class inversely to its frequency,
  // so the lone default row pulls on the boundary as hard as all seven
  // non-defaults together.
  lymph::options::Train balanced{};
  balanced.class_weight = lymph::ClassWeightMode::Balanced;
  lymph::models::Model rebalanced = blueprint.learn(x, y, balanced);

  // infer_scores is the raw decision score (log-odds); infer is its sign as
  // a 0/1 label.
  const DataFrame compare({
      {"actual", Thread(defaulted)},
      {"score_plain", Thread(plain.infer_scores(x))},
      {"score_balanced", Thread(rebalanced.infer_scores(x))},
      {"label_plain", Thread(plain.infer(x))},
      {"label_balanced", Thread(rebalanced.infer(x))},
  });
  std::cout << compare;

  return 0;
}
