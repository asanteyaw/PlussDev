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
using pluss::ai::nodej::HashMapOptions;
using pluss::ai::nodej::HashMapper;
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
    const auto keep = prune_or.value().selected_feature_indices.to(pluss::pCPU, ScalarType::Int64).host<std::int64_t>().array().data;
    if (keep.size() < 2) {
      if (err) *err << "variance_prune should keep informative features\n";
      return false;
    }

    auto rank_or = pluss::ai::nodej::f_score_rank(batch, ScoreRankOptions{.top_k = 2, .absolute_score = true});
    if (!rank_or.ok()) {
      if (err) *err << "f_score_rank should succeed\n";
      return false;
    }
    const auto top2 = rank_or.value().to(pluss::pCPU, ScalarType::Int64).host<std::int64_t>().array().data;
    if (top2.size() != 2) {
      if (err) *err << "f_score_rank should return requested top_k entries\n";
      return false;
    }
    auto rank_report_or = pluss::ai::nodej::rank_features(
        batch,
        pluss::ai::nodej::FeatureRankOptions{
            .mode = pluss::ai::nodej::FeatureScoreMode::ClassF,
            .absolute_score = true,
        });
    if (!rank_report_or.ok()
        || rank_report_or.value().scores.numel() != 3
        || rank_report_or.value().ranked_feature_indices.numel() != 3) {
      if (err) *err << "rank_features should return all scores and a full rank order\n";
      return false;
    }
    auto pick_or = pluss::ai::nodej::pick_features(
        batch,
        pluss::ai::nodej::FeaturePickOptions{
            .mode = pluss::ai::nodej::FeatureScoreMode::ClassF,
            .top_k = 2,
            .fraction = 0.0,
            .absolute_score = true,
        });
    if (!pick_or.ok()
        || pick_or.value().selected_feature_indices.numel() != 2
        || pick_or.value().reduced.feature_count() != 2) {
      if (err) *err << "pick_features should return reduced data and selected feature ids\n";
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
    auto pick_fraction_or = pluss::ai::nodej::pick_features(
        reg_batch,
        pluss::ai::nodej::FeaturePickOptions{
            .mode = pluss::ai::nodej::FeatureScoreMode::RegressionF,
            .top_k = 10,
            .fraction = 0.5,
            .absolute_score = true,
        });
    if (!pick_fraction_or.ok()
        || pick_fraction_or.value().selected_feature_indices.numel() != 2
        || pick_fraction_or.value().scores.numel() != 3) {
      if (err) *err << "pick_features fraction mode should select a rounded feature fraction\n";
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

    LexiconMapper ngram_mapper(LexiconMapOptions{
        .lowercase = true,
        .max_features = 16,
        .min_document_frequency = 1,
        .binary = false,
        .min_ngram = 1,
        .max_ngram = 2,
    });
    if (!ngram_mapper.learn({"new york city", "new york state"}).ok()) {
      if (err) *err << "LexiconMapper should learn unigram+bigram vocabularies\n";
      return false;
    }
    const auto& ngram_vocab = ngram_mapper.vocabulary();
    if (std::find(ngram_vocab.begin(), ngram_vocab.end(), "new york") == ngram_vocab.end()) {
      if (err) *err << "LexiconMapper vocabulary should include generated bigrams\n";
      return false;
    }

    LexiconMapper technical_mapper(LexiconMapOptions{
        .lowercase = true,
        .max_features = 16,
        .min_document_frequency = 1,
        .binary = false,
    });
    const auto technical_status =
        technical_mapper.learn({"C++ compiler", "C# tooling", "email help@pluss.dev", "visit https://pluss.dev/docs"});
    if (!technical_status.ok()) {
      if (err) *err << "LexiconMapper should learn technical tokens through the shared Note tokenizer\n";
      return false;
    }
    const auto& vocab = technical_mapper.vocabulary();
    const auto has_token = [&](const std::string& token) {
      return std::find(vocab.begin(), vocab.end(), token) != vocab.end();
    };
    if (!has_token("c++") || !has_token("c#") ||
        !has_token("help@pluss.dev") || !has_token("https://pluss.dev/docs")) {
      if (err) *err << "LexiconMapper vocabulary should preserve C++, C#, emails, and URLs as full tokens\n";
      return false;
    }

    HashMapper hasher(HashMapOptions{
        .lowercase = true,
        .n_features = 32,
        .binary = false,
        .alternate_sign = false,
        .min_ngram = 1,
        .max_ngram = 2,
    });
    auto hashed_dense_or = hasher.map_dense({"new york city", "new york state"});
    if (!hashed_dense_or.ok()) {
      if (err) *err << "HashMapper map_dense should succeed\n";
      return false;
    }
    if (hashed_dense_or.value().sample_count() != 2 || hashed_dense_or.value().feature_count() != 32) {
      if (err) *err << "HashMapper dense output should respect the configured feature width\n";
      return false;
    }
    auto hashed_sparse_or = hasher.map_sparse({"new york city", "new york state"});
    if (!hashed_sparse_or.ok()) {
      if (err) *err << "HashMapper map_sparse should succeed\n";
      return false;
    }
    if (hashed_sparse_or.value().rows != 2 || hashed_sparse_or.value().cols != 32) {
      if (err) *err << "HashMapper sparse output should expose the configured feature width\n";
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
    if (!cls_metrics_or.ok() || cls_metrics_or.value().to(pluss::pCPU, ScalarType::Float64).host<double>().front() < 0.8) {
      if (err) *err << "SparseLinearBinaryClassifier should achieve reasonable training accuracy\n";
      return false;
    }

    auto cv_or = pluss::ai::nodej::cross_assess_sparse_binary(sparse_bin_or.value(),
                                                               y_bin,
                                                               pluss::ai::nodej::SplitOptions{
                                                                   .plan = pluss::ai::nodej::CVPlanKind::CrossVal,
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
            .plan = pluss::ai::nodej::CVPlanKind::CrossVal,
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
                                                               .plan = pluss::ai::nodej::CVPlanKind::CrossVal,
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
    if (!sweep_or.value().ledger.objective.defined()
        || sweep_or.value().ledger.encoded_params.sizes() != std::vector<std::int64_t>{2, 6}
        || sweep_or.value().ledger.split_test_metrics.sizes().size() != 3) {
      if (err) *err << "sweep_sparse_binary should expose ledger-grade tuning artifacts\n";
      return false;
    }
    auto tune_native_or = pluss::ai::nodej::tune_sparse_binary(
        {LinearFamilyOptions{.objective = LinearObjective::Logistic, .regularization = pluss::ai::nodej::RegularizationKind::L2, .regularization_strength = 1e-3},
         LinearFamilyOptions{.objective = LinearObjective::Logistic, .regularization = pluss::ai::nodej::RegularizationKind::L2, .regularization_strength = 1e-6}},
        sparse_bin_or.value(),
        y_bin,
        pluss::ai::nodej::SplitOptions{
            .plan = pluss::ai::nodej::CVPlanKind::CrossVal,
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
    const auto reg_metrics = reg_metrics_or.value().to(pluss::pCPU, ScalarType::Float64).host<double>().array().data;
    if (reg_metrics.size() != 2 || reg_metrics[0] > 0.2 || reg_metrics[1] < 0.7) {
      if (err) *err << "SparseLinearRegressor should fit training signal reasonably\n";
      return false;
    }

    auto cv_or = pluss::ai::nodej::cross_assess_sparse_regression(sparse_reg_or.value(),
                                                                   y_reg,
                                                                   pluss::ai::nodej::SplitOptions{
                                                                       .plan = pluss::ai::nodej::CVPlanKind::CrossVal,
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
    if (!multi_metrics_or.ok() || multi_metrics_or.value().to(pluss::pCPU, ScalarType::Float64).host<double>().front() < 0.85) {
      if (err) *err << "SparseLinearMultiClassifier should classify training set accurately\n";
      return false;
    }

    auto cv_or = pluss::ai::nodej::cross_assess_sparse_multiclass(sparse_multi_or.value(),
                                                                   y_mc,
                                                                   pluss::ai::nodej::SplitOptions{
                                                                       .plan = pluss::ai::nodej::CVPlanKind::StratifiedCrossVal,
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

  {
    // Feature 0 is a clean linear signal for y; feature 1 is pure noise
    // (independent of y). A correct permutation importance must rank
    // feature 0 well above feature 1 -- and must do so consistently whether
    // the requested metric is a score (Accuracy, higher-is-better) or a loss
    // (LogLoss, lower-is-better): shuffling the informative feature should
    // always look like a *cost*, never a benefit, regardless of which
    // direction the metric itself points.
    const Series x_train = pluss::make_series(std::vector<double>{
                                                  -4.0, 0.3, -3.5, -0.2, -3.0, 0.1, -2.5, -0.4, -2.0, 0.2,
                                                  -1.5, -0.1, -1.0, 0.4, -0.5, -0.3, 2.0, 0.0, 2.5, -0.2,
                                                   3.0, 0.3, 3.5, -0.1, 4.0, 0.2, 1.5, 0.1, 1.0, 0.0,
                                                   0.5, -0.2, 5.0, 0.4, 5.5, -0.3, 6.0, 0.1, 6.5, -0.4},
                                              {20, 2},
                                              f64());
    const Series y_train = pluss::make_series(std::vector<double>{
                                                  0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
                                                  1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0},
                                              {20},
                                              f64());
    const LabeledBatch train{
        .features = FeatureBatch{.x = x_train, .layout = ObservationLayout::Rows},
        .y = y_train,
    };
    const ModelSpec cls_spec{
        .task = TaskKind::Classification,
        .family = ModelFamily::Linear,
        .options = LinearFamilyOptions{.objective = LinearObjective::Logistic},
    };

    auto acc_or = pluss::ai::nodej::permutation_impact(cls_spec, train, train, MetricKind::Accuracy, 8,
                                                        pluss::ai::nodej::RandomState{.seed = 42, .deterministic = true});
    if (!acc_or.ok()) {
      if (err) *err << "permutation_impact(Accuracy) should succeed on the signal/noise fixture\n";
      return false;
    }
    const auto acc_mean = acc_or.value().mean_drop.to(pluss::pCPU, ScalarType::Float64).host<double>().array().data;
    if (acc_mean.size() != 2 || !(acc_mean[0] > acc_mean[1])) {
      if (err) *err << "permutation_impact(Accuracy) should rank the informative feature above the noise feature\n";
      return false;
    }

    // Same fixture, a loss metric this time -- exercises the metric-direction
    // fix. Before the fix, classification always computed baseline - permuted
    // regardless of metric, which is backwards for a loss: shuffling the
    // informative feature would have looked like a NEGATIVE (beneficial)
    // change instead of a cost.
    auto loss_or = pluss::ai::nodej::permutation_impact(cls_spec, train, train, MetricKind::LogLoss, 8,
                                                         pluss::ai::nodej::RandomState{.seed = 42, .deterministic = true});
    if (!loss_or.ok()) {
      if (err) *err << "permutation_impact(LogLoss) should succeed on the signal/noise fixture\n";
      return false;
    }
    const auto loss_mean = loss_or.value().mean_drop.to(pluss::pCPU, ScalarType::Float64).host<double>().array().data;
    if (loss_mean.size() != 2 || !(loss_mean[0] > 0.0) || !(loss_mean[0] > loss_mean[1])) {
      if (err) *err << "permutation_impact(LogLoss) should show a positive, dominant cost for the informative feature\n";
      return false;
    }

    // Deterministic seeding must be reproducible across independent calls
    // (the parallel-over-features restructuring must not introduce
    // run-to-run nondeterminism).
    auto acc_again_or = pluss::ai::nodej::permutation_impact(cls_spec, train, train, MetricKind::Accuracy, 8,
                                                              pluss::ai::nodej::RandomState{.seed = 42, .deterministic = true});
    if (!acc_again_or.ok()) {
      if (err) *err << "permutation_impact(Accuracy) repeat call should succeed\n";
      return false;
    }
    const auto acc_mean_again = acc_again_or.value().mean_drop.to(pluss::pCPU, ScalarType::Float64).host<double>().array().data;
    for (std::size_t i = 0; i < acc_mean.size(); ++i) {
      if (std::abs(acc_mean[i] - acc_mean_again[i]) > 1e-12) {
        if (err) *err << "permutation_impact with a deterministic seed should be reproducible across calls\n";
        return false;
      }
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
