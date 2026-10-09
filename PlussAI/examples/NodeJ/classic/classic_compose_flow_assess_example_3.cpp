#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  using pluss::table::DataFrame;
  using pluss::table::Slate;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::StatusOr<cls::Flow> made = cls::make_flow(
      cls::ModelSpec{cls::TaskKind::Regression, cls::ModelFamily::Linear, cls::LinearFamilyOptions{}});
  cls::Flow flow = made.move_value();

  const DataFrame train_frame({
      {"x", Slate(pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4}, f64))},
      {"y", Slate(pluss::make_series({2.0, 4.0, 6.0, 8.0}, {4}, f64))},
  });
  cls::DataFrameBatchOptions data_options{};
  data_options.feature_names = {"x"};
  data_options.label_feature = "y";
  flow.learn(train_frame, data_options);

  const DataFrame held_out({
      {"x", Slate(pluss::make_series({5.0}, {1}, f64))},
      {"y", Slate(pluss::make_series({10.0}, {1}, f64))},
  });
  cls::AssessOptions opts{};
  opts.metrics = {cls::MetricKind::Mse};
  const cls::StatusOr<pluss::Series> scored = flow.assess(held_out, data_options, opts);
  std::cout << "ok    = " << std::boolalpha << scored.ok() << "\n";
  std::cout << "score = " << scored.value() << "\n";

  return 0;
}
