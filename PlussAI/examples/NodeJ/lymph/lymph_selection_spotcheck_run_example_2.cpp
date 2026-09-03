#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// run(batch, cv, ...) takes a pre-split LabeledBatch. FitOptions.fit_top /
// fit_bottom control how many leaders and laggards get re-fitted on the whole
// set, exposed through CompareResult::top(k) / bottom(k) and best() / worst().
int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  lymph::LabeledBatch batch{};
  batch.features.x = pluss::make_series(
      std::vector<double>{1., 1., 2., 1., 3., 2., 4., 2., 5., 3., 6., 3., 7., 4., 8., 4.,
                          9., 8., 10., 8., 11., 9., 12., 9., 13., 10., 14., 10., 15., 11., 16., 11.},
      {16, 2}, f64);
  batch.features.layout = lymph::ObservationLayout::Rows;
  batch.y = pluss::make_series(
      std::vector<double>{0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1}, {16}, f64);

  lymph::models::Builder registry = lymph::models::build({
      {"bayes", lymph::models::gaussian_nb_classifier()},
      {"tree", lymph::models::tree::decision_tree_classifier({.max_depth = 2})},
  });

  lymph::selection::CrossValOptions cv{};
  cv.folds = 2;
  cv.shuffle = false;
  cv.max_parallel = 1;

  const lymph::selection::spotcheck::Result ranked =
      lymph::selection::suite(registry)
          .strategy("plain", lymph::TrainOptions{})
          .run(batch, cv, lymph::AssessOptions{}, lymph::MetricKind::Accuracy,
               lymph::TuneObjectivePolicy::Maximize,
               lymph::selection::FitOptions{.fit_top = 1, .fit_bottom = 2});

  std::cout << "trials = " << ranked.report.trials.size()
            << ", best = " << ranked.report.best_name << "\n";
  std::cout << "best trained = " << std::boolalpha << ranked.best().trained()
            << ", worst trained = " << ranked.worst().trained() << "\n";

  const auto top1 = ranked.top(1);
  const auto bottom2 = ranked.bottom(2);
  std::cout << "top(1): " << top1.front().first << "\n";
  std::cout << "bottom(2): " << bottom2.front().first << ", " << bottom2.back().first << "\n";

  return 0;
}
