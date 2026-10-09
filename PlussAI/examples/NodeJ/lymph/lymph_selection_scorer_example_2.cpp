#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// A custom selection::Scorer as the objective of Plan::scores / Plan::score --
// cross-validate ONE model on a metric with no MetricKind. Here: the F2 score
// (F-beta with beta = 2, recall weighted 4x precision), F2 = 5PR / (4P + R).
int main() {
  namespace lymph = pluss::ai::lymph;
  namespace selection = lymph::selection;
  using pluss::ScalarType;
  using pluss::Series;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const Series score = pluss::make_series(
      {1.0, 1.2, 1.4, 1.6, 1.8, 2.0, 2.2, 2.4, 2.6, 2.8,
       3.0, 3.2, 3.4, 3.6, 3.8, 6.0, 6.4, 6.8, 7.2, 7.6}, {20}, f64);
  const Series y = pluss::make_series(
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1}, {20}, f64);
  const DataFrame frame({{"score", Slate(score)}, {"target", Slate(y)}});

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"score"};
  data_options.label_feature = "target";

  const auto f2 = [](const Series& truth, const Series& prediction) -> double {
    const Series t = truth.reshape({truth.numel()}).to(ScalarType::Float64);
    const Series p = prediction.reshape({prediction.numel()}).to(ScalarType::Float64);
    const Series tp_s = t.eq(1.0).to(ScalarType::Float64).mul(p.eq(1.0).to(ScalarType::Float64)).sum();
    const Series fp_s = t.eq(0.0).to(ScalarType::Float64).mul(p.eq(1.0).to(ScalarType::Float64)).sum();
    const Series fn_s = t.eq(1.0).to(ScalarType::Float64).mul(p.eq(0.0).to(ScalarType::Float64)).sum();
    const double tp = tp_s.item<double>(), fp = fp_s.item<double>(), fn = fn_s.item<double>();
    const double precision = (tp + fp) > 0.0 ? tp / (tp + fp) : 0.0;
    const double recall = (tp + fn) > 0.0 ? tp / (tp + fn) : 0.0;
    const double denom = 4.0 * precision + recall;
    return denom > 0.0 ? 5.0 * precision * recall / denom : 0.0;
  };

  const selection::Plan plan = selection::stratified().folds(5).seed(20260901ULL);
  const lymph::models::Model model = lymph::models::logistic_regression();

  const Series f2_per_fold =
      plan.scores(model, frame, data_options, selection::make_scorer("f2", f2));
  const double f2_mean =
      plan.score(model, frame, data_options, selection::make_scorer("f2", f2));

  std::cout << "f2 per fold = " << f2_per_fold << "\n";
  std::cout << "mean f2     = " << f2_mean << "\n";

  return 0;
}
