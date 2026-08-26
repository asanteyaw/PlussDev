#include <pluss/ai/lymph.h>

#include <iostream>
#include <type_traits>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

namespace {

namespace lymph = pluss::ai::lymph;

bool run_namespace_surface_checks(std::ostream* err) {
  static_assert(std::is_same_v<lymph::workflow::Payload, lymph::workflow::payload::Payload>);
  static_assert(std::is_same_v<lymph::workflow::Policy, lymph::workflow::policy::Policy>);
  static_assert(std::is_same_v<lymph::selection::Search, lymph::selection::search::Search>);
  static_assert(std::is_same_v<lymph::selection::Suite, lymph::selection::spotcheck::Suite>);
  static_assert(std::is_same_v<lymph::workflow::TimeOffset, lymph::workflow::time::TimeOffset>);
  static_assert(std::is_same_v<lymph::workflow::TimeOptions, lymph::workflow::time::TimeOptions>);
  static_assert(std::is_same_v<lymph::workflow::Workflow, lymph::workflow::Workflow>);
  static_assert(std::is_same_v<lymph::workflow::WorkflowBranch, lymph::workflow::WorkflowBranch>);
  static_assert(std::is_same_v<lymph::workflow::WorkflowRoute, lymph::workflow::WorkflowRoute>);
  static_assert(std::is_same_v<lymph::workflow::WorkflowOptions, lymph::workflow::WorkflowOptions>);
  static_assert(std::is_same_v<lymph::workflow::WorkflowStepSpec, lymph::workflow::WorkflowStepSpec>);
  static_assert(std::is_same_v<lymph::workflow::WorkflowBranchSpec, lymph::workflow::WorkflowBranchSpec>);
  static_assert(std::is_same_v<lymph::prep::Route, lymph::prep::route::Route>);
  static_assert(std::is_same_v<lymph::prep::Stage, lymph::prep::stage::Stage>);
  static_assert(std::is_same_v<lymph::prep::CustomStage, lymph::prep::stage::CustomStage>);
  static_assert(std::is_same_v<lymph::prep::TransformContract, lymph::prep::stage::TransformContract>);
  static_assert(std::is_same_v<lymph::TargetTransform, lymph::transform::TargetTransform>);
  static_assert(std::is_same_v<lymph::MetricReport, lymph::metrics::MetricReport>);
  static_assert(std::is_same_v<lymph::SeriesCompareReport, lymph::metrics::SeriesCompareReport>);
  static_assert(std::is_same_v<lymph::LinearParameters, lymph::metrics::LinearParameters>);
  static_assert(std::is_same_v<decltype(lymph::selection::cross().options()),
                               const lymph::selection::CrossValOptions&>);
  static_assert(std::is_same_v<lymph::ModelReports, lymph::reports::ModelReports>);
  static_assert(std::is_same_v<lymph::models::OnlineLinearClassifier, lymph::models::online::OnlineLinearClassifier>);
  static_assert(std::is_same_v<lymph::models::OnlineLinearRegressor, lymph::models::online::OnlineLinearRegressor>);
  static_assert(std::is_same_v<lymph::StandardScaler, lymph::prep::StandardScaler>);
  static_assert(std::is_same_v<lymph::SimpleImputer, lymph::prep::SimpleImputer>);
  static_assert(std::is_same_v<lymph::OutlierIsolator, lymph::prep::OutlierIsolator>);
  static_assert(std::is_same_v<lymph::CategoryIndexer, lymph::prep::CategoryIndexer>);
  static_assert(std::is_same_v<decltype(lymph::models::linear::linear_regression()), lymph::models::Model>);
  static_assert(std::is_same_v<decltype(lymph::models::linear_regression()), lymph::models::Model>);
  static_assert(std::is_same_v<decltype(lymph::models::tree::decision_tree_regressor()), lymph::models::Model>);
  static_assert(std::is_same_v<decltype(lymph::models::decision_tree_regressor()), lymph::models::Model>);
  static_assert(std::is_same_v<decltype(lymph::models::ensemble::random_forest_classifier()), lymph::models::Model>);
  static_assert(std::is_same_v<decltype(lymph::models::random_forest_classifier()), lymph::models::Model>);
  static_assert(std::is_same_v<decltype(lymph::models::builder::build()), lymph::models::Builder>);
  static_assert(std::is_same_v<decltype(lymph::models::build()), lymph::models::Builder>);
  static_assert(std::is_same_v<decltype(lymph::models::online::linear_classifier()), lymph::models::OnlineLinearClassifier>);
  static_assert(std::is_same_v<decltype(lymph::models::online::linear_regressor()), lymph::models::OnlineLinearRegressor>);
  static_assert(std::is_same_v<decltype(lymph::models::linear_classifier()), lymph::models::OnlineLinearClassifier>);
  static_assert(std::is_same_v<decltype(lymph::models::linear_regressor()), lymph::models::OnlineLinearRegressor>);
  static_assert(std::is_same_v<decltype(lymph::workflow::payload::make(pluss::ai::nodej::FeatureBatch{})),
                               lymph::workflow::Payload>);
  static_assert(std::is_same_v<decltype(lymph::selection::suite(lymph::models::build())), lymph::selection::Suite>);
  (void)err;

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("lymph namespace surface is grouped without breaking shortcuts", "[lymph][namespace]") {
  REQUIRE(run_namespace_surface_checks(&std::cerr));
}
#else
int main() {
  if (!run_namespace_surface_checks(&std::cerr)) return 1;
  std::cout << "lymph namespace surface checks passed\n";
  return 0;
}
#endif
