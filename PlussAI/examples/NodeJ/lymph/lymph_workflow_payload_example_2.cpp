#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// Exercises the Payload setters Example 1 does not: weights (by name and by
// Series), groups, time(TimeOptions), and the feature_batch()/labeled_batch()
// reduction methods -- printing each one's effect so Payload.md can quote
// real, verified state instead of a hypothetical illustration.
int main() {
  namespace lymph = pluss::ai::lymph;
  namespace workflow = lymph::workflow;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame raw({
      {"income", Thread(pluss::make_series({1.0, 1.4, 1.8, 2.2}, {4}, f64))},
      {"balance", Thread(pluss::make_series({0.6, 1.0, 1.3, 1.6}, {4}, f64))},
      {"sample_weight", Thread(pluss::make_series({1.0, 1.0, 2.0, 2.0}, {4}, f64))},
      {"default", Thread(pluss::make_series({0, 0, 1, 1}, {4}, f64))},
      {"observed_at", Thread(pluss::make_timeline({
          "2026-01-01T00:00:00Z", "2026-01-02T00:00:00Z",
          "2026-01-03T00:00:00Z", "2026-01-04T00:00:00Z"}))},
  });

  // weights(feature_name): a column supplies per-row weights.
  workflow::Payload by_column = workflow::payload::make(raw)
                                    .features({"income", "balance"})
                                    .label("default")
                                    .weights("sample_weight");
  std::cout << "weights(\"sample_weight\"): has_weights=" << std::boolalpha << by_column.has_weights()
            << " has_weight_feature=" << by_column.has_weight_feature()
            << " has_external_weights=" << by_column.has_external_weights() << "\n";

  // weights(Series): an already-separate weight vector instead.
  const pluss::Series w = pluss::make_series({1.0, 1.0, 2.0, 2.0}, {4}, f64);
  workflow::Payload by_series = workflow::payload::make(raw)
                                    .features({"income", "balance"})
                                    .label("default")
                                    .weights(w);
  std::cout << "weights(Series):    has_weights=" << by_series.has_weights()
            << " has_weight_feature=" << by_series.has_weight_feature()
            << " has_external_weights=" << by_series.has_external_weights() << "\n";

  // groups(Series): entity keys for grouped cross-validation.
  const pluss::Series groups = pluss::make_series({0.0, 0.0, 1.0, 1.0}, {4}, f64);
  workflow::Payload grouped = workflow::payload::make(raw)
                                  .features({"income", "balance"})
                                  .label("default")
                                  .groups(groups);
  std::cout << "groups(Series):     has_groups=" << grouped.has_groups() << "\n";

  // time(TimeOptions): the richer time role, vs. time(name) which only sets
  // the index column.
  workflow::TimeOptions time_options{};
  time_options.index = "observed_at";
  time_options.horizon = workflow::steps(1);
  workflow::Payload with_time_options = workflow::payload::make(raw)
                                            .features({"income", "balance"})
                                            .label("default")
                                            .time(time_options);
  std::cout << "time(TimeOptions):  time_feature=" << with_time_options.time_feature()
            << " time_options().has_value()=" << with_time_options.time_options().has_value()
            << " horizon=" << with_time_options.time_options()->horizon << "\n";

  // feature_batch()/labeled_batch(): the two reduction methods every
  // Workflow verb calls internally.
  const lymph::FeatureBatch fb = by_column.feature_batch();
  const lymph::LabeledBatch lb = by_column.labeled_batch();
  std::cout << "feature_batch(): rows=" << fb.sample_count() << " cols=" << fb.feature_count() << "\n";
  std::cout << "labeled_batch(): rows=" << lb.features.sample_count()
            << " has_weights=" << lb.has_weights() << "\n";

  return 0;
}
