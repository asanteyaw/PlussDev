#include <pluss/ai/ml.h>

#include <cmath>
#include <iostream>
#include <vector>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;
using pluss::ai::nodej::BayesFamilyOptions;
using pluss::ai::nodej::BayesLikelihood;
using pluss::ai::nodej::EnsembleFamilyOptions;
using pluss::ai::nodej::EnsembleMethod;
using pluss::ai::nodej::FeatureBatch;
using pluss::ai::nodej::LabeledBatch;
using pluss::ai::nodej::LinearFamilyOptions;
using pluss::ai::nodej::LinearObjective;
using pluss::ai::nodej::MetricKind;
using pluss::ai::nodej::ModelFamily;
using pluss::ai::nodej::ModelSpec;
using pluss::ai::nodej::ObservationLayout;
using pluss::ai::nodej::StatusCode;
using pluss::ai::nodej::TaskKind;

Config f64() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

bool approx(double a, double b, double tol = 1e-6) {
  return std::abs(a - b) <= tol;
}

bool run_nodej_ensemble_bayes_regression_checks(std::ostream* err = nullptr) {
  {
    const Series x = pluss::make_series(std::vector<double>{
                                            -2.0, -1.0,
                                            -1.5, -1.0,
                                            -2.0, -2.0,
                                            -1.0, -1.5,
                                             1.0,  1.0,
                                             2.0,  1.0,
                                             1.0,  2.0,
                                             1.5,  1.0},
                                        {8, 2},
                                        f64());
    const Series y = pluss::make_series(std::vector<double>{0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0}, {8}, f64());
    const LabeledBatch batch{
        .features = FeatureBatch{.x = x, .layout = ObservationLayout::Rows},
        .y = y,
    };

    const ModelSpec spec{
        .task = TaskKind::Classification,
        .family = ModelFamily::Ensemble,
        .options = EnsembleFamilyOptions{
            .method = EnsembleMethod::RandomForest,
            .learners = 21,
            .subsample = 1.0,
        },
    };
    auto cls_or = pluss::ai::nodej::make_classifier(spec);
    if (!cls_or.ok()) {
      if (err) *err << "make_classifier should succeed for ensemble classifier\n";
      return false;
    }
    auto cls = cls_or.move_value();
    if (!cls->learn(batch).ok()) {
      if (err) *err << "ensemble classifier learn should succeed\n";
      return false;
    }

    auto pred_or = cls->infer(batch.features);
    if (!pred_or.ok()) {
      if (err) *err << "ensemble classifier infer should succeed\n";
      return false;
    }

    const auto pred = pred_or.value().to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    const auto truth = y.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    std::size_t correct = 0;
    for (std::size_t i = 0; i < pred.size(); ++i) {
      if (approx(pred[i], truth[i])) ++correct;
    }
    const double accuracy = static_cast<double>(correct) / static_cast<double>(pred.size());
    if (accuracy < 0.95) {
      if (err) *err << "ensemble classifier should fit separable train set with high accuracy\n";
      return false;
    }

    auto metric_or = cls->assess(batch, pluss::ai::nodej::AssessOptions{.metrics = {MetricKind::Accuracy}});
    if (!metric_or.ok()) {
      if (err) *err << "ensemble classifier assess should succeed\n";
      return false;
    }
    const auto metric = metric_or.value().to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    if (metric.empty() || metric[0] < 0.95) {
      if (err) *err << "ensemble classifier assess should report high accuracy\n";
      return false;
    }
  }

  {
    const Series x = pluss::make_series(std::vector<double>{-2.0, -1.5, -1.0, -0.5, 0.0, 0.5, 1.0, 1.5, 2.0}, {9, 1}, f64());
    const Series y = pluss::make_series(std::vector<double>{4.0, 2.25, 1.0, 0.25, 0.0, 0.25, 1.0, 2.25, 4.0}, {9}, f64());
    const LabeledBatch batch{
        .features = FeatureBatch{.x = x, .layout = ObservationLayout::Rows},
        .y = y,
    };

    const ModelSpec spec{
        .task = TaskKind::Regression,
        .family = ModelFamily::Ensemble,
        .options = EnsembleFamilyOptions{
            .method = EnsembleMethod::RandomForest,
            .learners = 31,
            .subsample = 0.9,
        },
    };
    auto reg_or = pluss::ai::nodej::make_regressor(spec);
    if (!reg_or.ok()) {
      if (err) *err << "make_regressor should succeed for ensemble regressor\n";
      return false;
    }
    auto reg = reg_or.move_value();
    if (!reg->learn(batch).ok()) {
      if (err) *err << "ensemble regressor learn should succeed\n";
      return false;
    }

    auto metric_or = reg->assess(batch, pluss::ai::nodej::AssessOptions{.metrics = {MetricKind::Mse, MetricKind::R2}});
    if (!metric_or.ok()) {
      if (err) *err << "ensemble regressor assess should succeed\n";
      return false;
    }
    const auto metric = metric_or.value().to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    if (metric.size() != 2 || metric[0] > 0.5 || metric[1] < 0.7) {
      if (err) *err << "ensemble regressor should model smooth nonlinear train signal\n";
      return false;
    }
  }

  {
    const Series x = pluss::make_series(std::vector<double>{
                                            -2.0, 0.0,
                                            -1.0, 1.0,
                                             0.0, 0.0,
                                             1.0, 1.0,
                                             2.0, 0.0,
                                             3.0, 1.0},
                                        {6, 2},
                                        f64());
    const Series y = pluss::make_series(std::vector<double>{-3.0, -1.5, 1.0, 2.5, 5.0, 6.5}, {6}, f64());
    const LabeledBatch batch{
        .features = FeatureBatch{.x = x, .layout = ObservationLayout::Rows},
        .y = y,
    };

    const ModelSpec spec{
        .task = TaskKind::Regression,
        .family = ModelFamily::Bayes,
        .options = BayesFamilyOptions{
            .use_gaussian = true,
            .smoothing = 1e-6,
        },
    };
    auto reg_or = pluss::ai::nodej::make_regressor(spec);
    if (!reg_or.ok()) {
      if (err) *err << "make_regressor should succeed for bayes regressor\n";
      return false;
    }
    auto reg = reg_or.move_value();
    if (!reg->learn(batch).ok()) {
      if (err) *err << "bayes regressor learn should succeed\n";
      return false;
    }

    auto pred_or = reg->infer(batch.features);
    if (!pred_or.ok()) {
      if (err) *err << "bayes regressor infer should succeed\n";
      return false;
    }
    const auto pred = pred_or.value().to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    const auto truth = y.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    for (std::size_t i = 0; i < pred.size(); ++i) {
      if (!approx(pred[i], truth[i], 1e-3)) {
        if (err) *err << "bayes regressor should recover near-linear relationship on train set\n";
        return false;
      }
    }
  }

  {
    const Series x = pluss::make_series(std::vector<double>{
                                            -2.0, -1.0,
                                            -1.0, -0.5,
                                             0.0,  0.0,
                                             1.0,  0.5,
                                             2.0,  1.0,
                                             3.0,  1.5},
                                        {6, 2},
                                        f64());
    const Series y = pluss::make_series(std::vector<double>{0.0, 0.0, 0.0, 1.0, 1.0, 1.0}, {6}, f64());
    const LabeledBatch batch{
        .features = FeatureBatch{.x = x, .layout = ObservationLayout::Rows},
        .y = y,
    };
    const ModelSpec spec{
        .task = TaskKind::Regression,
        .family = ModelFamily::Bayes,
        .options = BayesFamilyOptions{
            .likelihood = BayesLikelihood::Bernoulli,
            .smoothing = 1e-6,
        },
    };
    auto reg_or = pluss::ai::nodej::make_regressor(spec);
    if (!reg_or.ok()) {
      if (err) *err << "make_regressor should succeed for bayes bernoulli regressor\n";
      return false;
    }
    auto reg = reg_or.move_value();
    if (!reg->learn(batch, pluss::ai::nodej::TrainOptions{.max_iterations = 1500, .tolerance = 1e-8}).ok()) {
      if (err) *err << "bayes bernoulli regressor learn should succeed\n";
      return false;
    }
    auto pred_or = reg->infer(batch.features);
    if (!pred_or.ok()) {
      if (err) *err << "bayes bernoulli regressor infer should succeed\n";
      return false;
    }
    const auto pred = pred_or.value().to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    for (double v : pred) {
      if (!(v >= -1e-9 && v <= 1.0 + 1e-9)) {
        if (err) *err << "bayes bernoulli regressor outputs should stay in [0,1]\n";
        return false;
      }
    }
  }

  {
    const Series x = pluss::make_series(std::vector<double>{
                                            0.0, 0.0,
                                            0.0, 1.0,
                                            1.0, 0.0,
                                            1.0, 1.0,
                                            2.0, 1.0,
                                            2.0, 2.0},
                                        {6, 2},
                                        f64());
    const Series y = pluss::make_series(std::vector<double>{0.0, 1.0, 1.0, 3.0, 4.0, 7.0}, {6}, f64());
    const LabeledBatch batch{
        .features = FeatureBatch{.x = x, .layout = ObservationLayout::Rows},
        .y = y,
    };
    const ModelSpec spec{
        .task = TaskKind::Regression,
        .family = ModelFamily::Bayes,
        .options = BayesFamilyOptions{
            .likelihood = BayesLikelihood::Multinomial,
            .smoothing = 1e-6,
        },
    };
    auto reg_or = pluss::ai::nodej::make_regressor(spec);
    if (!reg_or.ok()) {
      if (err) *err << "make_regressor should succeed for bayes multinomial regressor\n";
      return false;
    }
    auto reg = reg_or.move_value();
    if (!reg->learn(batch).ok()) {
      if (err) *err << "bayes multinomial regressor learn should succeed\n";
      return false;
    }
    auto pred_or = reg->infer(batch.features);
    if (!pred_or.ok()) {
      if (err) *err << "bayes multinomial regressor infer should succeed\n";
      return false;
    }
    const auto pred = pred_or.value().to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    for (double v : pred) {
      if (!(v >= -1e-9 && std::isfinite(v))) {
        if (err) *err << "bayes multinomial regressor outputs should be finite and non-negative\n";
        return false;
      }
    }
  }

  {
    const Series x = pluss::make_series(std::vector<double>{0.0, 1.0, 2.0, 3.0}, {4, 1}, f64());
    const Series y_cls = pluss::make_series(std::vector<double>{0.0, 0.0, 1.0, 1.0}, {4}, f64());
    const Series y_reg = pluss::make_series(std::vector<double>{0.0, 1.0, 2.0, 3.0}, {4}, f64());
    const FeatureBatch features{.x = x, .layout = ObservationLayout::Rows};

    auto bad_cls = pluss::ai::nodej::make_classifier(ModelSpec{
        .task = TaskKind::Classification,
        .family = ModelFamily::Ensemble,
        .options = EnsembleFamilyOptions{
            .method = EnsembleMethod::GradientBoost,
            .learners = 30,
            .subsample = 1.0,
            .learning_rate = 0.1,
        },
    });
    if (!bad_cls.ok()) {
      if (err) *err << "builder should still construct ensemble classifier\n";
      return false;
    }
    auto boost_cls = bad_cls.move_value();
    const auto boost_cls_status = boost_cls->learn(LabeledBatch{.features = features, .y = y_cls});
    if (!boost_cls_status.ok()) {
      if (err) *err << "ensemble classifier should support gradient-boost mode\n";
      return false;
    }
    auto boost_cls_pred_or = boost_cls->infer(features);
    if (!boost_cls_pred_or.ok()) {
      if (err) *err << "gradient-boost classifier infer should succeed\n";
      return false;
    }
    const auto boost_cls_pred = boost_cls_pred_or.value().to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    const auto truth_cls = y_cls.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    std::size_t cls_correct = 0;
    for (std::size_t i = 0; i < boost_cls_pred.size(); ++i) {
      if (approx(boost_cls_pred[i], truth_cls[i])) ++cls_correct;
    }
    if (cls_correct < 3) {
      if (err) *err << "gradient-boost classifier should fit simple train signal better than chance\n";
      return false;
    }

    auto cls_state_or = boost_cls->dump_state();
    if (!cls_state_or.ok()) {
      if (err) *err << "gradient-boost classifier dump_state should succeed\n";
      return false;
    }
    auto cls_reload_or = pluss::ai::nodej::make_classifier(ModelSpec{
        .task = TaskKind::Classification,
        .family = ModelFamily::Ensemble,
        .options = EnsembleFamilyOptions{
            .method = EnsembleMethod::GradientBoost,
            .learners = 30,
            .subsample = 1.0,
            .learning_rate = 0.1,
        },
    });
    if (!cls_reload_or.ok()) {
      if (err) *err << "builder should construct gradient-boost classifier for reload\n";
      return false;
    }
    auto cls_reloaded = cls_reload_or.move_value();
    if (!cls_reloaded->load_state(cls_state_or.value()).ok()) {
      if (err) *err << "gradient-boost classifier load_state should succeed\n";
      return false;
    }
    auto cls_reload_pred_or = cls_reloaded->infer(features);
    if (!cls_reload_pred_or.ok()) {
      if (err) *err << "gradient-boost classifier infer after load_state should succeed\n";
      return false;
    }
    const auto cls_reload_pred = cls_reload_pred_or.value().to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    if (cls_reload_pred.size() != boost_cls_pred.size()) {
      if (err) *err << "reloaded gradient-boost classifier prediction size mismatch\n";
      return false;
    }
    for (std::size_t i = 0; i < cls_reload_pred.size(); ++i) {
      if (!approx(cls_reload_pred[i], boost_cls_pred[i], 1e-9)) {
        if (err) *err << "reloaded gradient-boost classifier should reproduce predictions\n";
        return false;
      }
    }

    auto cls_metric_or =
        boost_cls->assess(LabeledBatch{.features = features, .y = y_cls},
                          pluss::ai::nodej::AssessOptions{.metrics = {MetricKind::Accuracy}});
    if (!cls_metric_or.ok()) {
      if (err) *err << "gradient-boost classifier assess should succeed\n";
      return false;
    }
    const auto cls_metric = cls_metric_or.value().to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    if (cls_metric.empty() || cls_metric.front() < 0.5) {
      if (err) *err << "gradient-boost classifier should report usable training accuracy\n";
      return false;
    }

    auto ada_cls_or = pluss::ai::nodej::make_classifier(ModelSpec{
        .task = TaskKind::Classification,
        .family = ModelFamily::Ensemble,
        .options = EnsembleFamilyOptions{
            .method = EnsembleMethod::AdaBoost,
            .learners = 25,
            .subsample = 1.0,
            .learning_rate = 0.5,
        },
    });
    if (!ada_cls_or.ok()) {
      if (err) *err << "builder should construct adaboost classifier\n";
      return false;
    }
    auto ada_cls = ada_cls_or.move_value();
    if (!ada_cls->learn(LabeledBatch{.features = features, .y = y_cls}).ok()) {
      if (err) *err << "adaboost classifier learn should succeed\n";
      return false;
    }
    auto ada_cls_pred_or = ada_cls->infer(features);
    if (!ada_cls_pred_or.ok()) {
      if (err) *err << "adaboost classifier infer should succeed\n";
      return false;
    }
    const auto ada_cls_pred = ada_cls_pred_or.value().to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    std::size_t ada_cls_correct = 0;
    for (std::size_t i = 0; i < ada_cls_pred.size(); ++i) {
      if (approx(ada_cls_pred[i], truth_cls[i])) ++ada_cls_correct;
    }
    if (ada_cls_correct < 3) {
      if (err) *err << "adaboost classifier should fit simple train signal better than chance\n";
      return false;
    }

    auto bad_reg = pluss::ai::nodej::make_regressor(ModelSpec{
        .task = TaskKind::Regression,
        .family = ModelFamily::Ensemble,
        .options = EnsembleFamilyOptions{
            .method = EnsembleMethod::GradientBoost,
            .learners = 40,
            .subsample = 1.0,
            .learning_rate = 0.1,
        },
    });
    if (!bad_reg.ok()) {
      if (err) *err << "builder should still construct ensemble regressor\n";
      return false;
    }
    auto boost_reg = bad_reg.move_value();
    const auto boost_reg_status = boost_reg->learn(LabeledBatch{.features = features, .y = y_reg});
    if (!boost_reg_status.ok()) {
      if (err) *err << "ensemble regressor should support gradient-boost mode\n";
      return false;
    }
    auto boost_pred_or = boost_reg->infer(features);
    if (!boost_pred_or.ok()) {
      if (err) *err << "gradient-boost regressor infer should succeed\n";
      return false;
    }
    const auto boost_pred = boost_pred_or.value().to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    const auto truth_reg = y_reg.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    double mse = 0.0;
    for (std::size_t i = 0; i < boost_pred.size(); ++i) {
      const double e = boost_pred[i] - truth_reg[i];
      mse += e * e;
    }
    mse /= static_cast<double>(boost_pred.size());
    if (mse > 0.25) {
      if (err) *err << "gradient-boost regressor should fit smooth training signal with low mse\n";
      return false;
    }

    auto ada_reg_or = pluss::ai::nodej::make_regressor(ModelSpec{
        .task = TaskKind::Regression,
        .family = ModelFamily::Ensemble,
        .options = EnsembleFamilyOptions{
            .method = EnsembleMethod::AdaBoost,
            .learners = 25,
            .subsample = 1.0,
            .learning_rate = 0.5,
        },
    });
    if (!ada_reg_or.ok()) {
      if (err) *err << "builder should construct adaboost regressor\n";
      return false;
    }
    auto ada_reg = ada_reg_or.move_value();
    if (!ada_reg->learn(LabeledBatch{.features = features, .y = y_reg}).ok()) {
      if (err) *err << "adaboost regressor learn should succeed\n";
      return false;
    }
    auto ada_reg_pred_or = ada_reg->infer(features);
    if (!ada_reg_pred_or.ok()) {
      if (err) *err << "adaboost regressor infer should succeed\n";
      return false;
    }
    const auto ada_reg_pred = ada_reg_pred_or.value().to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    double ada_mse = 0.0;
    for (std::size_t i = 0; i < ada_reg_pred.size(); ++i) {
      const double e = ada_reg_pred[i] - truth_reg[i];
      ada_mse += e * e;
    }
    ada_mse /= static_cast<double>(ada_reg_pred.size());
    if (!(ada_mse < 2.0)) {
      if (err) *err << "adaboost regressor should produce bounded training error\n";
      return false;
    }

    auto state_or = boost_reg->dump_state();
    if (!state_or.ok()) {
      if (err) *err << "gradient-boost regressor dump_state should succeed\n";
      return false;
    }
    auto reload_or = pluss::ai::nodej::make_regressor(ModelSpec{
        .task = TaskKind::Regression,
        .family = ModelFamily::Ensemble,
        .options = EnsembleFamilyOptions{
            .method = EnsembleMethod::GradientBoost,
            .learners = 40,
            .subsample = 1.0,
            .learning_rate = 0.1,
        },
    });
    if (!reload_or.ok()) {
      if (err) *err << "builder should construct gradient-boost regressor for reload\n";
      return false;
    }
    auto reloaded = reload_or.move_value();
    if (!reloaded->load_state(state_or.value()).ok()) {
      if (err) *err << "gradient-boost regressor load_state should succeed\n";
      return false;
    }
    auto reload_pred_or = reloaded->infer(features);
    if (!reload_pred_or.ok()) {
      if (err) *err << "gradient-boost regressor infer after load_state should succeed\n";
      return false;
    }
    const auto reload_pred = reload_pred_or.value().to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    if (reload_pred.size() != boost_pred.size()) {
      if (err) *err << "reloaded gradient-boost regressor prediction size mismatch\n";
      return false;
    }
    for (std::size_t i = 0; i < reload_pred.size(); ++i) {
      if (!approx(reload_pred[i], boost_pred[i], 1e-9)) {
        if (err) *err << "reloaded gradient-boost regressor should reproduce predictions\n";
        return false;
      }
    }

    auto bad_bayes = pluss::ai::nodej::make_regressor(ModelSpec{
        .task = TaskKind::Regression,
        .family = ModelFamily::Bayes,
        .options = BayesFamilyOptions{
            .use_gaussian = false,
            .smoothing = 1e-6,
        },
    });
    if (!bad_bayes.ok()) {
      if (err) *err << "builder should still construct bayes regressor\n";
      return false;
    }
    const auto bad_bayes_status = bad_bayes.value()->learn(LabeledBatch{.features = features, .y = y_reg});
    if (bad_bayes_status.ok() || bad_bayes_status.code != StatusCode::NotImplemented) {
      if (err) *err << "bayes regressor should reject unsupported non-gaussian mode\n";
      return false;
    }
  }

  {
    const Series x = pluss::make_series(std::vector<double>{
                                            -4.0,
                                            -3.0,
                                            -2.0,
                                            -1.0,
                                             0.0,
                                             1.0,
                                             2.0,
                                             3.0,
                                             4.0,
                                            10.0,
                                            11.0,
                                            12.0},
                                        {12, 1},
                                        f64());
    const Series y = pluss::make_series(std::vector<double>{
                                            -8.0,
                                            -6.2,
                                            -3.9,
                                            -1.8,
                                             0.1,
                                             2.2,
                                             4.1,
                                             5.8,
                                             8.0,
                                            35.0,
                                           -12.0,
                                            40.0},
                                        {12},
                                        f64());
    const FeatureBatch features{.x = x, .layout = ObservationLayout::Rows};
    const LabeledBatch batch{.features = features, .y = y};

    auto robust_or = pluss::ai::nodej::make_regressor(ModelSpec{
        .task = TaskKind::Regression,
        .family = ModelFamily::Linear,
        .options = LinearFamilyOptions{
            .objective = LinearObjective::RobustLeast,
            .regularization = pluss::ai::nodej::RegularizationKind::L2,
            .regularization_strength = 1e-4,
            .robust_transition = 1.35,
            .fit_intercept = true,
        },
    });
    if (!robust_or.ok()) {
      if (err) *err << "builder should construct robust linear regressor\n";
      return false;
    }
    auto robust = robust_or.move_value();
    if (!robust->learn(batch).ok()) {
      if (err) *err << "robust linear regressor learn should succeed\n";
      return false;
    }
    auto robust_pred_or = robust->infer(features);
    if (!robust_pred_or.ok()) {
      if (err) *err << "robust linear regressor infer should succeed\n";
      return false;
    }
    const auto robust_pred = robust_pred_or.value().to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    if (robust_pred.size() != 12) {
      if (err) *err << "robust linear regressor prediction size mismatch\n";
      return false;
    }
    if (std::abs(robust_pred[4] - 0.1) > 1.5 || std::abs(robust_pred[8] - 8.0) > 2.0) {
      if (err) *err << "robust linear regressor should track inlier trend despite outliers\n";
      return false;
    }

    auto state_or = robust->dump_state();
    if (!state_or.ok()) {
      if (err) *err << "robust linear regressor dump_state should succeed\n";
      return false;
    }
    auto robust_reload_or = pluss::ai::nodej::make_regressor(ModelSpec{
        .task = TaskKind::Regression,
        .family = ModelFamily::Linear,
        .options = LinearFamilyOptions{
            .objective = LinearObjective::RobustLeast,
            .regularization = pluss::ai::nodej::RegularizationKind::L2,
            .regularization_strength = 1e-4,
            .robust_transition = 1.35,
            .fit_intercept = true,
        },
    });
    if (!robust_reload_or.ok()) {
      if (err) *err << "builder should construct robust linear regressor for reload\n";
      return false;
    }
    auto robust_reloaded = robust_reload_or.move_value();
    if (!robust_reloaded->load_state(state_or.value()).ok()) {
      if (err) *err << "robust linear regressor load_state should succeed\n";
      return false;
    }
    auto robust_reload_pred_or = robust_reloaded->infer(features);
    if (!robust_reload_pred_or.ok()) {
      if (err) *err << "robust linear regressor infer after load_state should succeed\n";
      return false;
    }
    const auto robust_reload_pred = robust_reload_pred_or.value().to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    if (robust_reload_pred.size() != robust_pred.size()) {
      if (err) *err << "reloaded robust linear regressor prediction size mismatch\n";
      return false;
    }
    for (std::size_t i = 0; i < robust_pred.size(); ++i) {
      if (!approx(robust_reload_pred[i], robust_pred[i], 1e-9)) {
        if (err) *err << "reloaded robust linear regressor should reproduce predictions\n";
        return false;
      }
    }
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("nodej ensemble and bayes regression families") {
  REQUIRE(run_nodej_ensemble_bayes_regression_checks(&std::cerr));
}
#else
int main() {
  if (!run_nodej_ensemble_bayes_regression_checks(&std::cerr)) {
    return 1;
  }
  std::cout << "nodej_ensemble_bayes_regression_test passed\n";
  return 0;
}
#endif
