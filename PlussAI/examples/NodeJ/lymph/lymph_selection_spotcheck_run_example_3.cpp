#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// run(design, payload, cv) wraps every strategy x model candidate in a
// workflow::Workflow before cross-validating -- so the spot-check judges the
// full designed pipeline, not the bare estimator. The ranked result shape is
// unchanged.
int main() {
  namespace lymph = pluss::ai::lymph;
  namespace workflow = pluss::ai::lymph::workflow;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series x0 = pluss::make_series(
      {1., 2., 3., 4., 5., 6., 7., 8., 9., 10., 11., 12., 13., 14., 15., 16.}, {16}, f64);
  const pluss::Series x1 = pluss::make_series(
      {1., 2., 3., 4., 5., 6., 7., 8., 7., 10., 11., 12., 13., 14., 15., 16.}, {16}, f64);
  const pluss::Series y = pluss::make_series(
      {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1}, {16}, f64);
  const DataFrame frame({{"x0", Slate(x0)}, {"x1", Slate(x1)}, {"target", Slate(y)}});

  lymph::models::Builder registry = lymph::models::build({
      {"bayes", lymph::models::gaussian_nb_classifier()},
      {"tree", lymph::models::tree::decision_tree_classifier({.max_depth = 2})},
  });

  const workflow::Payload payload =
      workflow::payload::make(frame).features({"x0", "x1"}).label("target");
  const workflow::Workflow design = workflow::make(workflow::Policy{});

  lymph::selection::CrossValOptions cv{};
  cv.folds = 2;
  cv.shuffle = false;
  cv.max_parallel = 1;

  const lymph::selection::spotcheck::Result ranked =
      lymph::selection::suite(registry)
          .strategy("plain", lymph::TrainOptions{})
          .run(design, payload, cv, lymph::AssessOptions{},
               lymph::MetricKind::Accuracy, lymph::TuneObjectivePolicy::Maximize,
               lymph::selection::FitOptions{.fit_top = 1});

  std::cout << "trials = " << ranked.report.trials.size()
            << ", best = " << ranked.report.best_name
            << ", best trained = " << std::boolalpha << ranked.best().trained() << "\n";

  return 0;
}
