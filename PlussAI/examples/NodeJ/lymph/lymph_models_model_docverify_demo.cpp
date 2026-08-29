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
  std::cout << "== initial_model ==\n";
  std::cout << "trained = " << std::boolalpha << model.trained()
            << ", stage_count = " << model.stage_count()
            << ", task_kind = " << static_cast<int>(model.task_kind()) << "\n\n";

  lymph::models::ModelPrep prep = model.prepare();
  const lymph::prep::Route categorical = prep.names({"region"});
  prep.standardize(prep.remaining(categorical));
  prep.encode(
      categorical,
      lymph::prep::OneHotEncoderOptions{
          .max_categories = 8,
          .overflow = lymph::prep::CategoryOverflowPolicy::KeepMostFrequent,
          .include_unknown_bucket = true,
      });
  std::cout << "== after_prepare ==\n";
  std::cout << "trained = " << model.trained()
            << ", stage_count = " << model.stage_count() << "\n\n";

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"income", "balance", "late", "region"};
  data_options.label_feature = "default";
  data_options.note_policy = lymph::DataNotePolicy::Factorize;

  lymph::options::Train train_options{};
  train_options.max_iterations = 600;
  train_options.tolerance = 1e-8;
  train_options.random = {.seed = 20260825ULL, .deterministic = true};

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

  std::cout << "== learned_model ==\n";
  std::cout << "trained = " << model.trained()
            << ", stage_count = " << model.stage_count() << "\n";
  std::cout << prediction_frame << "\n";
  std::cout << "accuracy = " << report.accuracy.item<double>()
            << ", f1 = " << report.f1.item<double>() << "\n\n";

  lymph::models::Model template_model = lymph::models::linear::logistic_regression(logistic_options);
  lymph::models::ModelPrep template_prep = template_model.prepare();
  const lymph::prep::Route template_categorical = template_prep.names({"region"});
  template_prep.standardize(template_prep.remaining(template_categorical));
  template_prep.encode(
      template_categorical,
      lymph::prep::OneHotEncoderOptions{
          .max_categories = 8,
          .overflow = lymph::prep::CategoryOverflowPolicy::KeepMostFrequent,
          .include_unknown_bucket = true,
      });
  const lymph::models::Model learned_copy = template_model.learn(raw, data_options, train_options);

  std::cout << "== learned_copy ==\n";
  std::cout << "original_trained = " << template_model.trained()
            << ", original_stage_count = " << template_model.stage_count()
            << ", copy_trained = " << learned_copy.trained()
            << ", copy_stage_count = " << learned_copy.stage_count() << "\n";

  return 0;
}
