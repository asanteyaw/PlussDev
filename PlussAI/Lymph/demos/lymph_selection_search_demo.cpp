#include <pluss/ai/lymph.h>
#include <pluss/mic.h>

#include <iostream>
#include <vector>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::Config;
  using pluss::ScalarType;
  using pluss::Series;

  const Config f64 = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const lymph::LabeledBatch batch{
      .features = lymph::FeatureBatch{
          .x = pluss::make_series(std::vector<double>{
                                      1.0, 1.0,
                                      2.0, 1.0,
                                      3.0, 2.0,
                                      4.0, 2.0,
                                      7.0, 8.0,
                                      8.0, 8.0,
                                      9.0, 9.0,
                                      10.0, 9.0,
                                  },
                                  {8, 2},
                                  f64),
          .layout = lymph::ObservationLayout::Rows,
      },
      .y = pluss::make_series(std::vector<double>{0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64),
  };

  lymph::Builder candidates = lymph::builder::build({
      {"bayes", lymph::models::gaussian_nb_classifier()},
      {"tree", lymph::models::decision_tree_classifier({.max_depth = 2})},
      {"knn", lymph::models::knn_classifier({.k = 3})},
  });

  lymph::CrossValidationOptions cv{};
  cv.folds = 2;
  cv.shuffle = false;
  cv.max_parallel = 1;

  lymph::AssessOptions assess{};
  assess.metrics = {lymph::MetricKind::Accuracy, lymph::MetricKind::F1, lymph::MetricKind::BalancedAccuracy};

  const lymph::selection::CompareResult ranked =
      candidates.benchmark(batch, cv, lymph::TrainOptions{}, assess,
                           lymph::MetricKind::Accuracy, lymph::TuneObjectivePolicy::Maximize,
                           lymph::selection::FitOptions{.fit_top = 1, .fit_bottom = 1});

  lymph::spotcheck::Result checked = lymph::spotcheck::suite(candidates)
                                       .strategy("plain", lymph::TrainOptions{})
                                       .strategy("weighted", lymph::TrainOptions{
                                                                 .class_weight = lymph::ClassWeightMode::Balanced,
                                                             })
                                       .run(batch, cv, assess, lymph::MetricKind::Accuracy,
                                            lymph::TuneObjectivePolicy::Maximize,
                                            lymph::selection::FitOptions{.fit_top = 2, .fit_bottom = 2});

  const std::vector<lymph::GridAxis> axes = {
      {.name = "regularization_strength", .values = {0.01, 0.1, 1.0}},
  };
  const auto make_model = [](const lymph::GridPoint& point) {
    lymph::Model model = lymph::models::logistic_regression({
        .regularization_strength = point.at("regularization_strength"),
        .fit_intercept = true,
    });
    model.standard_scaler();
    return model;
  };

  lymph::search::Result tuned = lymph::search::grid(axes, cv)
                                  .run(make_model, batch, lymph::TrainOptions{}, assess,
                                       lymph::MetricKind::Accuracy,
                                       lymph::TuneObjectivePolicy::Maximize,
                                       lymph::selection::FitOptions{.fit_top = 2});

  const double cv_mean = lymph::validate::stratified(cv).score(
      tuned.best(), batch, lymph::MetricKind::Accuracy, lymph::TrainOptions{}, assess);

  std::cout << "builder models: " << candidates.size() << "\n";
  std::cout << "benchmark trials: " << ranked.report.trials.size() << "\n";
  std::cout << "spotcheck top: " << checked.top(2).size() << "\n";
  std::cout << "spotcheck bottom: " << checked.bottom(2).size() << "\n";
  std::cout << "search trials: " << tuned.report.trials.size() << "\n";
  std::cout << "selected cv mean: " << cv_mean << "\n";
  return 0;
}
