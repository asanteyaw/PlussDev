#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  // Raw features on very different scales: a currency amount in the thousands
  // and a small ratio near 1. A categorical region code rides alongside.
  const pluss::Series amount = pluss::make_series({1200.0, 1800.0, 3400.0, 4100.0, 5200.0, 6000.0}, {6}, f64);
  const pluss::Series ratio = pluss::make_series({0.9, 1.1, 0.7, 1.3, 0.6, 1.4}, {6}, f64);
  const pluss::Series region = pluss::make_series({0, 1, 2, 0, 1, 2}, {6}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 0, 1, 1, 1}, {6}, f64);
  const DataFrame x({
      {"amount", Thread(amount)},
      {"ratio", Thread(ratio)},
      {"region", Thread(region)},
  });

  lymph::models::Model model = lymph::models::logistic_regression();

  // Two prep steps, each a stage descriptor paired with a route. The stage
  // says what to do; the route says which columns. Nothing is fitted yet.
  model.append(lymph::prep::stage::standard_scaler(),
               lymph::prep::route::names({"amount", "ratio"}));
  model.append(lymph::prep::stage::one_hot_encoder(),
               lymph::prep::route::names({"region"}));
  std::cout << "stage_count before learn = " << model.stage_count() << "\n";

  // learn_ resolves each route against the frame and fits each stage in order,
  // then fits the estimator on the conditioned features.
  model.learn_(x, y);
  std::cout << "stage_count after learn  = " << model.stage_count() << "\n";
  std::cout << "trained                  = " << std::boolalpha << model.trained() << "\n\n";

  // assess runs the same fitted prep contract before scoring, so callers pass
  // raw features and never re-apply the transforms by hand.
  lymph::options::Assess options{};
  options.metrics = {lymph::MetricKind::Accuracy, lymph::MetricKind::F1};
  const lymph::MetricReport report = model.assess(x, y, options);

  const DataFrame review({
      {"amount", Thread(amount)},
      {"ratio", Thread(ratio)},
      {"region", Thread(region)},
      {"actual", Thread(y)},
      {"predicted", Thread(model.infer(x))},
  });
  std::cout << review << "\n";
  std::cout << "accuracy = " << report.accuracy.item<double>() << "\n";
  std::cout << "f1       = " << report.f1.item<double>() << "\n";

  return 0;
}
