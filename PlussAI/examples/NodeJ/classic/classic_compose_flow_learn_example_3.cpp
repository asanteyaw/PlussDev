#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  using pluss::table::DataFrame;
  using pluss::table::Slate;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame frame({
      {"income", Slate(pluss::make_series({1.0, 1.5, 2.0, 2.5}, {4}, f64))},
      {"label", Slate(pluss::make_series({0.0, 0.0, 1.0, 1.0}, {4}, f64))},
  });

  cls::DataFrameBatchOptions data_options{};
  data_options.feature_names = {"income"};
  data_options.label_feature = "label";

  cls::LinearFamilyOptions linear{};
  linear.objective = cls::LinearObjective::Logistic;
  cls::StatusOr<cls::Flow> made = cls::make_flow(
      cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, linear});
  cls::Flow flow = made.move_value();

  const cls::Status learned = flow.learn(frame, data_options);
  std::cout << "learn ok  = " << std::boolalpha << learned.ok() << "\n";
  std::cout << "trained() = " << flow.trained() << "\n";

  return 0;
}
