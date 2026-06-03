#include <pluss/ai/ml.h>
#include <pluss/ai/nodej/sparse_runtime.h>

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;
using pluss::ai::nodej::AssessOptions;
using pluss::ai::nodej::FeatureBatch;
using pluss::ai::nodej::LabeledBatch;
using pluss::ai::nodej::LexiconMapper;
using pluss::ai::nodej::LexiconMapOptions;
using pluss::ai::nodej::LinearFamilyOptions;
using pluss::ai::nodej::LinearObjective;
using pluss::ai::nodej::MetricKind;
using pluss::ai::nodej::ModelFamily;
using pluss::ai::nodej::ModelSpec;
using pluss::ai::nodej::ObservationLayout;
using pluss::ai::nodej::ScoreRankOptions;
using pluss::ai::nodej::SparseLinearBinaryClassifier;
using pluss::ai::nodej::SparseLinearMultiClassifier;
using pluss::ai::nodej::SparseLinearRegressor;
using pluss::ai::nodej::SparseObjectivePolicy;
using pluss::ai::nodej::TaskKind;

Config f64() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

bool run_nodej_feature_inspect_text_checks(std::ostream* err = nullptr) {
  {
    const Series x_mc = pluss::make_series(std::vector<double>{
                                               -2.0, 0.1, 0.0,
                                               -1.5, 0.2, 0.1,
                                               -1.0, 0.1, 0.0,
                                                0.0, 1.2, 0.9,
                                                0.2, 1.1, 1.1,
                                                0.1, 1.0, 1.0,
                                                1.5, 2.2, 2.0,
                                                2.0, 2.1, 1.9,
                                                1.8, 2.0, 2.1},
                                           {9, 3},
                                           f64());
    const Series y_mc = pluss::make_series(std::vector<double>{0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 2.0, 2.0, 2.0}, {9}, f64());
    const LabeledBatch batch_mc{
        .features = FeatureBatch{.x = x_mc, .layout = ObservationLayout::Rows},
        .y = y_mc,
    };
    auto rank_mc_or = pluss::ai::nodej::f_score_rank(batch_mc, ScoreRankOptions{.top_k = 2, .absolute_score = true});
    if (!rank_mc_or.ok()) {
      if (err) *err << "multiclass f_score_rank should succeed\n";
      return false;
    }
    if (rank_mc_or.value().numel() != 2) {
      if (err) *err << "multiclass f_score_rank should return requested top_k\n";
      return false;
    }

    const Series x = pluss::make_series(std::vector<double>{
                                            0.0, 1.0, 3.0,
                                            0.0, 2.0, 3.1,
                                            0.0, 3.0, 2.9,
                                            0.0, 4.0, 3.0,
                                            0.0, 5.0, 3.2,
                                            0.0, 6.0, 2.8},
                                        {6, 3},
                                        f64());
    const Series y = pluss::make_series(std::vector<double>{0.0, 0.0, 0.0, 1.0, 1.0, 1.0}, {6}, f64());
    const LabeledBatch batch{
        .features = FeatureBatch{.x = x, .layout = ObservationLayout::Rows},
        .y = y,
    };

    auto prune_or = pluss::ai::nodej::variance_prune(batch.features, 1e-9);
    if (!prune_or.ok()) {
      if (err) *err << "variance_prune should succeed\n";
      return false;
    }
    const auto keep = prune_or.value().selected_feature_indices.to(pluss::pCPU, ScalarType::Int64).to_vec<std::int64_t>();
    if (keep.size() < 2) {
      if (err) *err << "variance_prune should keep informative features\n";
      return false;
    }

    auto rank_or = pluss::ai::nodej::f_score_rank(batch, ScoreRankOptions{.top_k = 2, .absolute_score = true});
    if (!rank_or.ok()) {
      if (err) *err << "f_score_rank should succeed\n";
      return false;
    }
    const auto top2 = rank_or.value().to(pluss::pCPU, ScalarType::Int64).to_vec<std::int64_t>();
    if (top2.size() != 2) {
      if (err) *err << "f_score_rank should return requested top_k entries\n";
      return false;
    }

    const Series y_reg = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0, 6.0}, {6}, f64());
    const LabeledBatch reg_batch{
        .features = batch.features,
        .y = y_reg,
    };
    auto freg_or = pluss::ai::nodej::f_regression_rank(reg_batch, ScoreRankOptions{.top_k = 2, .absolute_score = true});
    if (!freg_or.ok()) {
      if (err) *err << "f_regression_rank should succeed\n";
      return false;
    }
    if (freg_or.value().numel() != 2) {
      if (err) *err << "f_regression_rank should return requested top_k\n";
      return false;
    }
  }

  {
    LexiconMapper mapper(LexiconMapOptions{
        .lowercase = true,
        .max_features = 16,
        .min_document_frequency = 1,
        .binary = false,
    });
    const pluss::ai::nodej::Status fit_status = mapper.learn({"red apple", "green apple", "red berry"});
    if (!fit_status.ok()) {
      if (err) *err << "LexiconMapper learn should succeed\n";
      return false;
    }

    auto dense_or = mapper.map_dense({"red apple", "berry"});
    if (!dense_or.ok()) {
      if (err) *err << "LexiconMapper map_dense should succeed\n";
      return false;
    }
    if (dense_or.value().sample_count() != 2 || dense_or.value().feature_count() <= 0) {
      if (err) *err << "LexiconMapper map_dense should return matrix with expected shape\n";
      return false;
    }

    auto sparse_or = mapper.map_sparse({"red apple", "berry"});
    if (!sparse_or.ok()) {
      if (err) *err << "LexiconMapper map_sparse should succeed\n";
      return false;
    }
    if (sparse_or.value().rows != 2 || sparse_or.value().cols <= 0) {
      if (err) *err << "LexiconMapper map_sparse should provide sparse shape metadata\n";
      return false;
    }

    auto tfidf_dense_or = mapper.map_tfidf_dense({"red apple", "berry"});
    if (!tfidf_dense_or.ok()) {
      if (err) *err << "LexiconMapper map_tfidf_dense should succeed\n";
      return false;
    }
    if (tfidf_dense_or.value().sample_count() != 2 || tfidf_dense_or.value().feature_count() <= 0) {
      if (err) *err << "LexiconMapper map_tfidf_dense should return expected shape\n";
      return false;
    }

    auto tfidf_sparse_or = mapper.map_tfidf_sparse({"red apple", "berry"});
    if (!tfidf_sparse_or.ok()) {
      if (err) *err << "LexiconMapper map_tfidf_sparse should succeed\n";
      return false;
    }
    if (tfidf_sparse_or.value().rows != 2 || tfidf_sparse_or.value().cols <= 0) {
      if (err) *err << "LexiconMapper map_tfidf_sparse should provide sparse shape metadata\n";
      return false;
    }
  }

  {
    auto sparse_bin_or = pluss::ai::nodej::make_sparse_triplet(
        6,
        3,
        {0, 1, 2, 3, 4, 5, 0, 1, 2, 3, 4, 5},
        {0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1},
        {-2.0, -1.0, 0.0, 1.0, 2.0, 3.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0});
    if (!sparse_bin_or.ok()) {
      if (err) *err << "make_sparse_triplet should build binary sparse matrix\n";
      return false;
    }
    SparseLinearBinaryClassifier cls{};
    const Series y_bin = pluss::make_series(std::vector<double>{0.0, 0.0, 0.0, 1.0, 1.0, 1.0}, {6}, f64());
    if (!cls.learn(sparse_bin_or.value(), y_bin).ok()) {
      if (err) *err << "SparseLinearBinaryClassifier learn should succeed\n";
      return false;
    }
    auto cls_metrics_or = cls.assess(sparse_bin_or.value(), y_bin, AssessOptions{.metrics = {MetricKind::Accuracy}});
    if (!cls_metrics_or.ok() || cls_metrics_or.value().to(pluss::pCPU, ScalarType::Float64).item<double>() < 0.8) {
      if (err) *err << "SparseLinearBinaryClassifier should achieve reasonable training accuracy\n";
      return false;
    }

    auto cv_or = pluss::ai::nodej::cross_assess_sparse_binary(sparse_bin_or.value(),
                                                               y_bin,
                                                               pluss::ai::nodej::SplitOptions{
                                                                   .plan = pluss::ai::nodej::CVPlanKind::KFold,
                                                                   .folds = 3,
                                                                   .shuffle = true,
                                                                   .random = pluss::ai::nodej::RandomState{.seed = 7, .deterministic = true},
                                                               },
                                                               pluss::ai::nodej::TrainOptions{.max_iterations = 400, .tolerance = 1e-7},
                                                               AssessOptions{.metrics = {MetricKind::Accuracy}});
    if (!cv_or.ok() || cv_or.value().folds.size() != 3) {
      if (err) *err << "cross_assess_sparse_binary should produce 3 folds\n";
      return false;
    }
    if (cv_or.value().mean_metrics.numel() != 1 || cv_or.value().fit_time_seconds.numel() != 3) {
      if (err) *err << "cross_assess_sparse_binary should expose metrics and timing artifacts\n";
      return false;
    }
    auto cv_native_or = pluss::ai::nodej::assess_sparse_binary_folds(
        sparse_bin_or.value(),
        y_bin,
        pluss::ai::nodej::SplitOptions{
            .plan = pluss::ai::nodej::CVPlanKind::KFold,
            .folds = 3,
            .shuffle = true,
            .random = pluss::ai::nodej::RandomState{.seed = 7, .deterministic = true},
        },
        pluss::ai::nodej::TrainOptions{.max_iterations = 400, .tolerance = 1e-7},
        AssessOptions{.metrics = {MetricKind::Accuracy}});
    if (!cv_native_or.ok() || cv_native_or.value().folds.size() != 3) {
      if (err) *err << "assess_sparse_binary_folds should match fold behavior\n";
      return false;
    }

    const std::vector<LinearFamilyOptions> opts{
        LinearFamilyOptions{.objective = LinearObjective::Logistic, .regularization = pluss::ai::nodej::RegularizationKind::L2, .regularization_strength = 1e-3},
        LinearFamilyOptions{.objective = LinearObjective::Logistic, .regularization = pluss::ai::nodej::RegularizationKind::L2, .regularization_strength = 1e-6},
    };
    auto sweep_or = pluss::ai::nodej::sweep_sparse_binary(opts,
                                                           sparse_bin_or.value(),
                                                           y_bin,
                                                           pluss::ai::nodej::SplitOptions{
                                                               .plan = pluss::ai::nodej::CVPlanKind::KFold,
                                                               .folds = 3,
                                                               .shuffle = true,
                                                               .random = pluss::ai::nodej::RandomState{.seed = 11, .deterministic = true},
                                                           },
                                                           pluss::ai::nodej::TrainOptions{.max_iterations = 300, .tolerance = 1e-7},
                                                           AssessOptions{.metrics = {MetricKind::Accuracy}},
                                                           MetricKind::Accuracy,
                                                           SparseObjectivePolicy::Maximize);
    if (!sweep_or.ok() || sweep_or.value().trials.size() != opts.size() || sweep_or.value().best_index < 0) {
      if (err) *err << "sweep_sparse_binary should evaluate options and choose best\n";
      return false;
    }
    auto tune_native_or = pluss::ai::nodej::tune_sparse_binary(
        {LinearFamilyOptions{.objective = LinearObjective::Logistic, .regularization = pluss::ai::nodej::RegularizationKind::L2, .regularization_strength = 1e-3},
         LinearFamilyOptions{.objective = LinearObjective::Logistic, .regularization = pluss::ai::nodej::RegularizationKind::L2, .regularization_strength = 1e-6}},
        sparse_bin_or.value(),
        y_bin,
        pluss::ai::nodej::SplitOptions{
            .plan = pluss::ai::nodej::CVPlanKind::KFold,
            .folds = 3,
            .shuffle = true,
            .random = pluss::ai::nodej::RandomState{.seed = 11, .deterministic = true},
        },
        pluss::ai::nodej::TrainOptions{.max_iterations = 300, .tolerance = 1e-7},
        AssessOptions{.metrics = {MetricKind::Accuracy}},
        MetricKind::Accuracy,
        SparseObjectivePolicy::Maximize);
    if (!tune_native_or.ok() || tune_native_or.value().trials.size() != 2 || tune_native_or.value().best_index < 0) {
      if (err) *err << "tune_sparse_binary should evaluate candidates and choose best\n";
      return false;
    }
  }

  {
    auto sparse_reg_or = pluss::ai::nodej::make_sparse_triplet(
        6,
        3,
        {0, 1, 2, 3, 4, 5, 0, 1, 2, 3, 4, 5},
        {0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1},
        {-2.0, -1.0, 0.0, 1.0, 2.0, 3.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0});
    if (!sparse_reg_or.ok()) {
      if (err) *err << "make_sparse_triplet should build reg sparse matrix\n";
      return false;
    }
    SparseLinearRegressor reg{};
    const Series y_reg = pluss::make_series(std::vector<double>{-1.0, -0.5, 0.0, 0.5, 1.0, 1.5}, {6}, f64());
    if (!reg.learn(sparse_reg_or.value(), y_reg).ok()) {
      if (err) *err << "SparseLinearRegressor learn should succeed\n";
      return false;
    }
    auto reg_metrics_or = reg.assess(sparse_reg_or.value(), y_reg, AssessOptions{.metrics = {MetricKind::Mse, MetricKind::R2}});
    if (!reg_metrics_or.ok()) {
      if (err) *err << "SparseLinearRegressor assess should succeed\n";
      return false;
    }
    const auto reg_metrics = reg_metrics_or.value().to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    if (reg_metrics.size() != 2 || reg_metrics[0] > 0.2 || reg_metrics[1] < 0.7) {
      if (err) *err << "SparseLinearRegressor should fit training signal reasonably\n";
      return false;
    }

    auto cv_or = pluss::ai::nodej::cross_assess_sparse_regression(sparse_reg_or.value(),
                                                                   y_reg,
                                                                   pluss::ai::nodej::SplitOptions{
                                                                       .plan = pluss::ai::nodej::CVPlanKind::KFold,
                                                                       .folds = 3,
                                                                       .shuffle = true,
                                                                       .random = pluss::ai::nodej::RandomState{.seed = 13, .deterministic = true},
                                                                   },
                                                                   pluss::ai::nodej::TrainOptions{.max_iterations = 500, .tolerance = 1e-8},
                                                                   AssessOptions{.metrics = {MetricKind::Mse, MetricKind::R2}});
    if (!cv_or.ok() || cv_or.value().folds.size() != 3 || cv_or.value().mean_metrics.numel() != 2) {
      if (err) *err << "cross_assess_sparse_regression should produce fold artifacts\n";
      return false;
    }
  }

  {
    auto sparse_multi_or = pluss::ai::nodej::make_sparse_triplet(
        9,
        2,
        {0, 1, 2, 3, 4, 5, 6, 7, 8, 0, 1, 2, 3, 4, 5, 6, 7, 8},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1},
        {-2.0, -1.5, -1.0, 0.0, 0.2, 0.1, 1.5, 2.0, 1.8, 0.1, 0.2, 0.1, 1.2, 1.1, 1.0, 2.2, 2.1, 2.0});
    if (!sparse_multi_or.ok()) {
      if (err) *err << "make_sparse_triplet should build multiclass sparse matrix\n";
      return false;
    }
    SparseLinearMultiClassifier mcls{};
    const Series y_mc = pluss::make_series(std::vector<double>{0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 2.0, 2.0, 2.0}, {9}, f64());
    if (!mcls.learn(sparse_multi_or.value(), y_mc).ok()) {
      if (err) *err << "SparseLinearMultiClassifier learn should succeed\n";
      return false;
    }
    auto pred_or = mcls.infer(sparse_multi_or.value());
    if (!pred_or.ok()) {
      if (err) *err << "SparseLinearMultiClassifier infer should succeed\n";
      return false;
    }
    auto multi_metrics_or = mcls.assess(sparse_multi_or.value(), y_mc, AssessOptions{.metrics = {MetricKind::Accuracy}});
    if (!multi_metrics_or.ok() || multi_metrics_or.value().to(pluss::pCPU, ScalarType::Float64).item<double>() < 0.85) {
      if (err) *err << "SparseLinearMultiClassifier should classify training set accurately\n";
      return false;
    }

    auto cv_or = pluss::ai::nodej::cross_assess_sparse_multiclass(sparse_multi_or.value(),
                                                                   y_mc,
                                                                   pluss::ai::nodej::SplitOptions{
                                                                       .plan = pluss::ai::nodej::CVPlanKind::StratifiedKFold,
                                                                       .folds = 3,
                                                                       .shuffle = true,
                                                                       .random = pluss::ai::nodej::RandomState{.seed = 17, .deterministic = true},
                                                                   },
                                                                   pluss::ai::nodej::TrainOptions{.max_iterations = 400, .tolerance = 1e-7},
                                                                   AssessOptions{.metrics = {MetricKind::Accuracy, MetricKind::F1}});
    if (!cv_or.ok() || cv_or.value().folds.size() != 3 || cv_or.value().mean_metrics.numel() != 2) {
      if (err) *err << "cross_assess_sparse_multiclass should produce fold artifacts\n";
      return false;
    }
  }

  {
    const Series x_train = pluss::make_series(std::vector<double>{
                                                  -2.0, 0.0,
                                                  -1.0, 0.0,
                                                   0.0, 1.0,
                                                   1.0, 1.0,
                                                   2.0, 1.0,
                                                   3.0, 1.0},
                                              {6, 2},
                                              f64());
    const Series y_train = pluss::make_series(std::vector<double>{0.0, 0.0, 0.0, 1.0, 1.0, 1.0}, {6}, f64());
    const LabeledBatch train{
        .features = FeatureBatch{.x = x_train, .layout = ObservationLayout::Rows},
        .y = y_train,
    };

    const ModelSpec cls_spec{
        .task = TaskKind::Classification,
        .family = ModelFamily::Linear,
        .options = LinearFamilyOptions{
            .objective = LinearObjective::Logistic,
        },
    };

    auto impact_or = pluss::ai::nodej::permutation_impact(cls_spec,
                                                           train,
                                                           train,
                                                           MetricKind::Accuracy,
                                                           3,
                                                           pluss::ai::nodej::RandomState{.seed = 7, .deterministic = true});
    if (!impact_or.ok()) {
      if (err) *err << "permutation_impact should succeed\n";
      return false;
    }
    if (impact_or.value().mean_drop.numel() != 2) {
      if (err) *err << "permutation_impact should return one importance per feature\n";
      return false;
    }

    auto pd_or = pluss::ai::nodej::dependence_curve(cls_spec, train, train.features, 0, 9);
    if (!pd_or.ok()) {
      if (err) *err << "dependence_curve should succeed\n";
      return false;
    }
    if (pd_or.value().grid.numel() != 9 || pd_or.value().mean_prediction.numel() != 9) {
      if (err) *err << "dependence_curve should return aligned grid and means\n";
      return false;
    }

    auto surface_or = pluss::ai::nodej::dependence_surface(cls_spec, train, train.features, 0, 1, 5, 4);
    if (!surface_or.ok()) {
      if (err) *err << "dependence_surface should succeed\n";
      return false;
    }
    if (surface_or.value().mean_prediction.numel() != 20) {
      if (err) *err << "dependence_surface should return grid_a x grid_b values\n";
      return false;
    }
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("nodej feature selection inspection and text") {
  REQUIRE(run_nodej_feature_inspect_text_checks(&std::cerr));
}
#else
int main() {
  if (!run_nodej_feature_inspect_text_checks(&std::cerr)) {
    return 1;
  }
  std::cout << "nodej_feature_inspect_text_test passed\n";
  return 0;
}
#endif
