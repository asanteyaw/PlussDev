#include <pluss/ai/lymph.h>

#include <iostream>
#include <type_traits>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

namespace {

namespace lymph = pluss::ai::lymph;

bool run_namespace_surface_checks(std::ostream* err) {
  static_assert(std::is_same_v<lymph::Model, lymph::model::Model>);
  static_assert(std::is_same_v<lymph::ModelPrep, lymph::model::ModelPrep>);
  static_assert(std::is_same_v<lymph::Builder, lymph::builder::Builder>);
  static_assert(std::is_same_v<lymph::Payload, lymph::payload::Payload>);
  static_assert(std::is_same_v<lymph::Policy, lymph::policy::Policy>);
  static_assert(std::is_same_v<lymph::Search, lymph::search::Search>);
  static_assert(std::is_same_v<lymph::Suite, lymph::spotcheck::Suite>);
  static_assert(std::is_same_v<lymph::TimeOffset, lymph::time::TimeOffset>);
  static_assert(std::is_same_v<lymph::TimeOptions, lymph::time::TimeOptions>);
  static_assert(std::is_same_v<lymph::Workflow, lymph::workflow::Workflow>);
  static_assert(std::is_same_v<lymph::WorkflowBranch, lymph::workflow::WorkflowBranch>);
  static_assert(std::is_same_v<lymph::WorkflowRoute, lymph::workflow::WorkflowRoute>);
  static_assert(std::is_same_v<lymph::WorkflowOptions, lymph::workflow::WorkflowOptions>);
  static_assert(std::is_same_v<lymph::WorkflowStepSpec, lymph::workflow::WorkflowStepSpec>);
  static_assert(std::is_same_v<lymph::WorkflowBranchSpec, lymph::workflow::WorkflowBranchSpec>);
  static_assert(std::is_same_v<lymph::ColumnRouter, lymph::columnrouter::ColumnRouter>);
  static_assert(std::is_same_v<lymph::Stage, lymph::stage::Stage>);
  static_assert(std::is_same_v<lymph::CustomStage, lymph::stage::CustomStage>);
  static_assert(std::is_same_v<lymph::TransformContract, lymph::stage::TransformContract>);
  static_assert(std::is_same_v<lymph::TargetTransform, lymph::transform::TargetTransform>);
  static_assert(std::is_same_v<lymph::MetricReport, lymph::metrics::MetricReport>);
  static_assert(std::is_same_v<lymph::SeriesCompareReport, lymph::metrics::SeriesCompareReport>);
  static_assert(std::is_same_v<lymph::LinearParameters, lymph::metrics::LinearParameters>);
  static_assert(std::is_same_v<lymph::CrossValidationOptions, lymph::metrics::CrossValidationOptions>);
  static_assert(std::is_same_v<lymph::CrossValidationReport, lymph::metrics::CrossValidationReport>);
  static_assert(std::is_same_v<lymph::FoldBatchPair, lymph::metrics::FoldBatchPair>);
  static_assert(std::is_same_v<lymph::ModelReports, lymph::reports::ModelReports>);
  static_assert(std::is_same_v<lymph::OnlineLinearClassifier, lymph::online::OnlineLinearClassifier>);
  static_assert(std::is_same_v<lymph::OnlineLinearRegressor, lymph::online::OnlineLinearRegressor>);
  static_assert(std::is_same_v<lymph::StandardScaler, lymph::prep::StandardScaler>);
  static_assert(std::is_same_v<lymph::SimpleImputer, lymph::prep::SimpleImputer>);
  static_assert(std::is_same_v<lymph::OutlierIsolator, lymph::prep::OutlierIsolator>);
  static_assert(std::is_same_v<lymph::CategoryIndexer, lymph::prep::CategoryIndexer>);
  static_assert(std::is_same_v<decltype(lymph::model::linear::linear_regression()), lymph::Model>);
  static_assert(std::is_same_v<decltype(lymph::model::linear_regression()), lymph::Model>);
  static_assert(std::is_same_v<decltype(lymph::models::linear_regression()), lymph::Model>);
  static_assert(std::is_same_v<decltype(lymph::model::tree::decision_tree_regressor()), lymph::Model>);
  static_assert(std::is_same_v<decltype(lymph::model::decision_tree_regressor()), lymph::Model>);
  static_assert(std::is_same_v<decltype(lymph::models::decision_tree_regressor()), lymph::Model>);
  static_assert(std::is_same_v<decltype(lymph::model::ensemble::random_forest_classifier()), lymph::Model>);
  static_assert(std::is_same_v<decltype(lymph::model::random_forest_classifier()), lymph::Model>);
  static_assert(std::is_same_v<decltype(lymph::builder::build()), lymph::Builder>);
  static_assert(std::is_same_v<decltype(lymph::online::linear_classifier()), lymph::OnlineLinearClassifier>);
  static_assert(std::is_same_v<decltype(lymph::online::linear_regressor()), lymph::OnlineLinearRegressor>);
  static_assert(std::is_same_v<decltype(lymph::payload::make(pluss::ai::nodej::FeatureBatch{})), lymph::Payload>);
  static_assert(std::is_same_v<decltype(lymph::spotcheck::suite(lymph::builder::build())), lymph::Suite>);
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
