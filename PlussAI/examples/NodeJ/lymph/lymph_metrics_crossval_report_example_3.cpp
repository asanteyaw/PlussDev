#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// report.mean.values / report.stdev.values are Series aligned to
// report.report.metrics -- the same order board() prints its rows in. Here we
// walk them ourselves. Spelled with the family-local type on the left.
int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series step = pluss::make_series(
      {1., 2., 3., 4., 5., 6., 7., 8., 9., 10., 11., 12.,
       13., 14., 15., 16., 17., 18., 19., 20.}, {20}, f64);
  const pluss::Series demand = pluss::make_series(
      {12., 13.5, 15., 16.6, 18., 19.4, 21., 22.5, 24., 25.4,
       27., 28.6, 30., 31.5, 33., 34.4, 36., 37.5, 39., 40.6}, {20}, f64);
  const DataFrame frame({{"step", Thread(step)}, {"demand", Thread(demand)}});

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"step"};
  data_options.label_feature = "demand";

  lymph::AssessOptions assess{};
  assess.metrics = {lymph::MetricKind::R2, lymph::MetricKind::Mse, lymph::MetricKind::Mae};

  const lymph::reports::CrossVal report =
      lymph::selection::cross().folds(4).seed(20260901ULL)
          .run(lymph::models::linear_regression(), frame, data_options, {}, assess);

  const std::vector<double> means = report.mean.values.to_vec<double>();
  const std::vector<double> stdevs = report.stdev.values.to_vec<double>();
  for (std::size_t i = 0; i < report.report.metrics.size(); ++i) {
    std::cout << "metric[" << i << "] mean=" << means[i]
              << " stdev=" << stdevs[i] << "\n";
  }

  return 0;
}
