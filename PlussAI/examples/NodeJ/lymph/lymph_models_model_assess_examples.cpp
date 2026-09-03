#include <pluss/lymph.h>
#include <pluss/mic.h>

#include <iostream>

namespace {

namespace lymph = pluss::ai::lymph;
using pluss::ScalarType;
using pluss::table::DataFrame;
using pluss::table::Thread;

// BEGIN EXAMPLE 1
void assess_separate_features_and_labels() {
  const auto f64 = pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame x({
      {"income", Thread(pluss::make_series({1.0, 1.5, 4.0, 4.5}, {4}, f64))},
      {"late_payments", Thread(pluss::make_series({0, 0, 1, 1}, {4}, f64))},
  });
  const auto y = pluss::make_series({0, 0, 1, 1}, {4}, f64);

  const auto model = lymph::models::tree::decision_tree_classifier({.max_depth = 2}).learn(x, y);
  const auto report = model.assess(x, y);
  std::cout << "assess_separate accuracy=" << report.accuracy.item<double>() << '\n';
}
// END EXAMPLE 1

// BEGIN EXAMPLE 2
void assess_labelled_dataframe_contract() {
  const auto f64 = pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame customer_rows({
      {"balance", Thread(pluss::make_series({20.0, 40.0, 500.0, 600.0}, {4}, f64))},
      {"months_active", Thread(pluss::make_series({36, 24, 2, 1}, {4}, f64))},
      {"defaulted", Thread(pluss::make_series({0, 0, 1, 1}, {4}, f64))},
  });
  lymph::options::DFBatch contract{};
  contract.feature_names = {"balance", "months_active"};
  contract.label_feature = "defaulted";

  const auto model = lymph::models::tree::decision_tree_classifier({.max_depth = 2})
                         .learn(customer_rows, contract);
  const auto report = model.assess(customer_rows, contract);
  std::cout << "assess_dataframe accuracy=" << report.accuracy.item<double>() << '\n';
}
// END EXAMPLE 2

// BEGIN EXAMPLE 3
void assess_multiple_requested_metrics() {
  const auto f64 = pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame x({
      {"utilization", Thread(pluss::make_series({0.1, 0.2, 0.8, 0.9}, {4}, f64))},
      {"delinquencies", Thread(pluss::make_series({0, 0, 2, 3}, {4}, f64))},
  });
  const auto y = pluss::make_series({0, 0, 1, 1}, {4}, f64);
  const auto model = lymph::models::tree::decision_tree_classifier({.max_depth = 2}).learn(x, y);
  const auto report = model.assess(
      x, y, lymph::options::Assess{.metrics = {lymph::MetricKind::Accuracy, lymph::MetricKind::F1}});
  std::cout << "assess_metrics accuracy=" << report.accuracy.item<double>()
            << " f1=" << report.f1.item<double>() << '\n';
}
// END EXAMPLE 3

}  // namespace

int main() {
  assess_separate_features_and_labels();
  assess_labelled_dataframe_contract();
  assess_multiple_requested_metrics();
  return 0;
}
