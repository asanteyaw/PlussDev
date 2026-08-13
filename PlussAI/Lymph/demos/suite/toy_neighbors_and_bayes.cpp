#include <pluss/ai/lymph.h>
#include <pluss/mic.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::Series;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const Series x = pluss::series::linspace(-2.0, 2.0, 64, ScalarType::Float64);
  const Series y = x.sin().gt(0.0).to(ScalarType::Float64);
  const DataFrame raw({{"x", Thread(x)}, {"sin_x", Thread(x.sin())}, {"y", Thread(y)}});
  lymph::Payload data = lymph::payload::make(raw).features({"x", "sin_x"}).label("y");

  lymph::NeighborFamilyOptions knn{};
  knn.k = 5;
  lymph::Builder models = lymph::builder::build({
      {"neighbor", lymph::models::knn_classifier(knn)},
      {"gauss", lymph::models::gaussian_nb_classifier()},
      {"tree", lymph::models::decision_tree_classifier({.max_depth = 4})},
  });

  lymph::CrossValidationOptions cv{};
  cv.plan = lymph::CVPlanKind::StratifiedCrossVal;
  cv.folds = 3;
  const lymph::spotcheck::Result ranked = lymph::spotcheck::suite(models)
      .strategy("plain")
      .run(raw, data.data(), cv, {}, lymph::MetricKind::Accuracy,
           lymph::TuneObjectivePolicy::Maximize, {.fit_top = 2, .fit_bottom = 1});

  std::cout << "neighbor/bayes shortlist: " << ranked.top(2).size() << "\n";
  return 0;
}
