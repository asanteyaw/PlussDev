#include <pluss/ai/lymph.h>
#include <pluss/mic.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::Series;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const Series x = pluss::series::linspace(-4.0, 4.0, 80, ScalarType::Float64);
  const Series z = x.sin().add(x.mul(0.5).cos());
  const Series y = z.gt(0.0).to(ScalarType::Float64);
  const DataFrame raw({{"x", Slate(x)}, {"z", Slate(z)}, {"y", Slate(y)}});

  lymph::workflow::Payload data = lymph::workflow::payload::make(raw).features({"x", "z"}).label("y");
  lymph::Workflow design = lymph::workflow::make(
      lymph::workflow::Policy{},
      std::vector<lymph::WorkflowBranchSpec>{
          lymph::workflow::branch("num", {"x", "z"}, {
              {"scale", lymph::prep::stage::standard_scaler()},
          }),
      });

  lymph::Builder models = lymph::builder::build({
      {"linear", lymph::models::logistic_regression({.regularization_strength = 0.01})},
      {"tree", lymph::models::decision_tree_classifier({.max_depth = 4, .min_samples_leaf = 2})},
      {"forest", lymph::models::random_forest_classifier({.learners = 25, .subsample = 1.0})},
  });

  lymph::selection::CrossValOptions cv{};
  cv.plan = lymph::CVPlanKind::StratifiedCrossVal;
  cv.folds = 3;
  cv.shuffle = true;
  cv.random_state = 20260811ULL;
  cv.deterministic = true;

  lymph::AssessOptions assess{};
  assess.metrics = {lymph::MetricKind::Accuracy, lymph::MetricKind::F1, lymph::MetricKind::BalancedAccuracy};

  lymph::TrainOptions plain{};
  lymph::TrainOptions weighted{};
  weighted.class_weight = lymph::ClassWeightMode::Balanced;

  lymph::spotcheck::Result ranked = lymph::spotcheck::suite(models)
      .strategy("plain", plain)
      .strategy("weighted", weighted)
      .run(design, data, cv, assess, lymph::MetricKind::F1,
           lymph::TuneObjectivePolicy::Maximize, {.fit_top = 2, .fit_bottom = 2});

  const lymph::selection::CompareResult direct =
      models.benchmark(raw, data.data(), cv, plain, assess, lymph::MetricKind::F1,
                       lymph::TuneObjectivePolicy::Maximize, {.fit_top = 1, .fit_bottom = 1});
  const lymph::models::Model best = models.best();
  const lymph::models::Model worst = models.worst();

  std::cout << "spotcheck top: " << ranked.top(2).size() << "\n";
  std::cout << "spotcheck bottom: " << ranked.bottom(2).size() << "\n";
  std::cout << "direct benchmark trials: " << direct.report.trials.size() << "\n";
  std::cout << "builder best trained: " << best.trained() << "\n";
  std::cout << "builder worst trained: " << worst.trained() << "\n";
  return 0;
}
