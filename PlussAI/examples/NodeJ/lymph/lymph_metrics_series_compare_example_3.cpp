#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// The reproducibility-check idiom: run a computation twice, compare the two
// output Series with compare_series, and assert the gap is negligible. Two
// identical Series give an all-zero abs_error.
int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series x = pluss::make_series(
      {1.0, 1.4, 1.8, 2.2, 2.6, 3.0, 3.4, 3.8, 4.2, 4.6}, {10}, f64);
  const pluss::Series y = pluss::make_series(
      {0, 0, 0, 0, 0, 1, 1, 1, 1, 1}, {10}, f64);
  const DataFrame frame({{"x", Slate(x)}, {"target", Slate(y)}});

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"x"};
  data_options.label_feature = "target";

  lymph::TrainOptions fixed{};
  fixed.random = {.seed = 7ULL, .deterministic = true};

  const lymph::models::Model a =
      lymph::models::logistic_regression().learn(frame, data_options, fixed);
  const lymph::models::Model b =
      lymph::models::logistic_regression().learn(frame, data_options, fixed);

  const pluss::Series pred_a = a.infer(frame.features({"x"}));
  const pluss::Series pred_b = b.infer(frame.features({"x"}));

  const lymph::metrics::SeriesCompareReport diff =
      lymph::metrics::compare_series(pred_a, pred_b);
  std::cout << "identical run: max_abs_diff = " << diff.max_abs_diff.item<double>() << "\n";
  std::cout << "abs_error all zero? " << std::boolalpha
            << (diff.abs_error.to_vec<double>() == std::vector<double>(10, 0.0)) << "\n";

  return 0;
}
