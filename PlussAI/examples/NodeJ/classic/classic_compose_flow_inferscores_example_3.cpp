#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  using pluss::table::DataFrame;
  using pluss::table::Slate;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::LinearFamilyOptions linear{};
  linear.objective = cls::LinearObjective::Logistic;
  cls::StatusOr<cls::Flow> made = cls::make_flow(
      cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, linear});
  cls::Flow flow = made.move_value();

  const DataFrame train_frame({
      {"x", Slate(pluss::make_series({1.0, 1.2, 2.0, 2.2}, {4}, f64))},
      {"label", Slate(pluss::make_series({0.0, 0.0, 1.0, 1.0}, {4}, f64))},
  });
  cls::DataFrameBatchOptions data_options{};
  data_options.feature_names = {"x"};
  data_options.label_feature = "label";
  flow.learn(train_frame, data_options);

  const DataFrame probe_frame({{"x", Slate(pluss::make_series({2.1}, {1}, f64))}});
  const cls::StatusOr<pluss::Series> scored = flow.infer_scores(probe_frame, data_options);
  std::cout << "ok = " << std::boolalpha << scored.ok() << "\n";
  std::cout << "scores = " << scored.value() << "\n";

  return 0;
}
