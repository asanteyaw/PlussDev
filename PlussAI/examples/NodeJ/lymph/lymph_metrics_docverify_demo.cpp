#include <pluss/lymph.h>
#include <pluss/mic.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::Series;
  using pluss::ai::nodej::ClassReportOptions;
  using pluss::ai::nodej::ConfusionOptions;
  using pluss::ai::nodej::LinearFamilyOptions;
  using pluss::ai::nodej::MetricKind;
  using pluss::ai::nodej::RegularizationKind;
  using pluss::ai::nodej::TrainOptions;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Series truth_labels = pluss::make_series({0.0, 1.0});
  const pluss::Series predicted_labels = pluss::make_series({0.0, 1.0});

  const lymph::ConfusionTable confusion = lymph::confusion_table(
      truth_labels,
      predicted_labels,
      ConfusionOptions{
          .normalize = pluss::ai::nodej::ConfusionNormalize::ByTruth,
      });
  std::cout << "== confusion_table ==\n";
  std::cout << "labels=" << confusion.labels << "\n";
  std::cout << "counts=" << confusion.counts << "\n";
  std::cout << "normalize=" << static_cast<int>(confusion.normalize) << "\n";

  const lymph::ClassReport summary = lymph::classification_summary(
      truth_labels,
      predicted_labels,
      ClassReportOptions{
          .zero_division = 0.0,
      });
  std::cout << "== classification_summary ==\n";
  std::cout << "labels=" << summary.labels << "\n";
  std::cout << "precision=" << summary.precision << "\n";
  std::cout << "recall=" << summary.recall << "\n";
  std::cout << "f1=" << summary.f1 << "\n";
  std::cout << "accuracy=" << summary.accuracy << "\n";
  std::cout << "macro_average=" << summary.macro_average << "\n";

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame frame({
      {"income", Thread(pluss::make_series({1.0, 1.4, 1.8, 2.2, 3.8, 4.2, 4.6, 5.0}, {8}, f64))},
      {"balance", Thread(pluss::make_series({0.2, 0.4, 0.6, 0.9, 1.8, 2.1, 2.4, 2.7}, {8}, f64))},
      {"late", Thread(pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64))},
      {"target", Thread(pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64))},
  });

  lymph::DataFrameBatchOptions data_options{};
  data_options.feature_names = {"income", "balance", "late"};
  data_options.label_feature = "target";

  TrainOptions train_options{};
  train_options.max_iterations = 600;
  train_options.tolerance = 1e-8;
  train_options.random = {.seed = 20260825ULL, .deterministic = true};

  LinearFamilyOptions logistic_options{};
  logistic_options.regularization = RegularizationKind::L2;
  logistic_options.regularization_strength = 1e-3;
  logistic_options.fit_intercept = true;

  const lymph::Model logistic = lymph::model::logistic_regression(logistic_options);
  const Series scores =
      logistic.learn(frame, data_options, train_options).infer_scores(frame, data_options);

  const lymph::metrics::Threshold threshold =
      lymph::metrics::threshold(scores, frame.features("target").as_series());
  std::cout << "== threshold ==\n";
  std::cout << "best_f1=" << threshold.best(MetricKind::F1) << "\n";
  std::cout << threshold.curve().thresholds.slice(0, 0, 3) << "\n";

  return 0;
}
