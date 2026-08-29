#include <pluss/lymph.h>
#include <pluss/mic.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame raw({
      {"income", Thread(pluss::make_series({1.0, 1.4, 1.8, 2.2, 3.8, 4.2, 4.6, 5.0}, {8}, f64))},
      {"balance", Thread(pluss::make_series({0.2, 0.4, 0.6, 0.9, 1.8, 2.1, 2.4, 2.7}, {8}, f64))},
      {"late", Thread(pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64))},
      {"region", Thread(pluss::make_note({"north", "west", "north", "south", "east", "south", "west", "east"}))},
      {"default", Thread(pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64))},
  });

  lymph::options::Linear logistic_options{};
  logistic_options.regularization = lymph::RegularizationKind::L2;
  logistic_options.regularization_strength = 1e-3;
  logistic_options.fit_intercept = true;

  lymph::models::Model model = lymph::models::linear::logistic_regression(logistic_options);
  lymph::models::ModelPrep prep = model.prepare();
  prep.standardize({"income", "balance", "late"});
  prep.encode(
      lymph::prep::route::names({"region"}),
      lymph::prep::OneHotEncoderOptions{
          .max_categories = 8,
          .overflow = lymph::prep::CategoryOverflowPolicy::KeepMostFrequent,
          .include_unknown_bucket = true,
      });

  lymph::options::Train train_options{};
  train_options.max_iterations = 600;
  train_options.tolerance = 1e-8;
  train_options.random = {.seed = 20260825ULL, .deterministic = true};

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"income", "balance", "late", "region"};
  data_options.label_feature = "default";
  data_options.note_policy = lymph::DataNotePolicy::Factorize;

  model.learn_(raw, data_options, train_options);
  const pluss::Series predicted = model.infer(raw, data_options);
  const lymph::reports::Metric report = model.assess(
      raw,
      data_options,
      lymph::options::Assess{
          .metrics = {lymph::MetricKind::Accuracy, lymph::MetricKind::F1},
      });
  const DataFrame prediction_frame({
      {"predicted", Thread(predicted)},
  });

  std::cout << "== prep ==\n";
  std::cout << "stage_count = " << model.stage_count()
            << ", trained = " << std::boolalpha << model.trained()
            << ", task_kind = " << static_cast<int>(model.task_kind()) << "\n\n";
  std::cout << prediction_frame << "\n";
  std::cout << "accuracy = " << report.accuracy.item<double>()
            << ", f1 = " << report.f1.item<double>() << "\n";

  return 0;
}
