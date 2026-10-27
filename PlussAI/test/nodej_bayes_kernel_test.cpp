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
using pluss::ai::nodej::AssessOptions;
using pluss::ai::nodej::BayesFamilyOptions;
using pluss::ai::nodej::BayesLikelihood;
using pluss::ai::nodej::FeatureBatch;
using pluss::ai::nodej::KernelFamilyOptions;
using pluss::ai::nodej::KernelForm;
using pluss::ai::nodej::KernelKind;
using pluss::ai::nodej::LabeledBatch;
using pluss::ai::nodej::LinearFamilyOptions;
using pluss::ai::nodej::MetricKind;
using pluss::ai::nodej::ModelFamily;
using pluss::ai::nodej::ModelSpec;
using pluss::ai::nodej::ObservationLayout;
using pluss::ai::nodej::StatusCode;
using pluss::ai::nodej::TaskKind;
using pluss::ai::nodej::TreeCriterion;
using pluss::ai::nodej::TrainOptions;

Config f64() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

bool approx(double a, double b, double tol = 1e-6) {
  return std::abs(a - b) <= tol;
}

bool run_nodej_bayes_kernel_checks(std::ostream* err = nullptr) {
  {
    const Series x = pluss::make_series(std::vector<double>{
                                            -2.0, -1.0,
                                            -1.5, -1.0,
                                            -2.0, -2.0,
                                             1.0,  1.0,
                                             2.0,  1.0,
                                             1.0,  2.0},
                                        {6, 2},
                                        f64());
    const Series y = pluss::make_series(std::vector<double>{0.0, 0.0, 0.0, 1.0, 1.0, 1.0}, {6}, f64());
    const FeatureBatch features{.x = x, .layout = ObservationLayout::Rows};
    const LabeledBatch batch{.features = features, .y = y};

    const ModelSpec spec{
        .task = TaskKind::Classification,
        .family = ModelFamily::Bayes,
        .options = BayesFamilyOptions{
            .use_gaussian = true,
            .smoothing = 1e-9,
        },
    };
    auto cls_or = pluss::ai::nodej::make_classifier(spec);
    if (!cls_or.ok()) {
      if (err) *err << "make_classifier should succeed for bayes classifier\n";
      return false;
    }
    auto cls = cls_or.move_value();
    if (!cls->learn(batch).ok()) {
      if (err) *err << "bayes classifier learn should succeed\n";
      return false;
    }

    auto score_or = cls->infer_scores(features);
    if (!score_or.ok()) {
      if (err) *err << "bayes classifier infer_scores should succeed\n";
      return false;
    }
    const auto score_vec = score_or.value().to(pluss::pCPU, ScalarType::Float64).host<double>();
    if (!(score_vec.front() < 0.0 && score_vec.back() > 0.0)) {
      if (err) *err << "bayes classifier scores should separate classes by sign\n";
      return false;
    }

    auto pred_or = cls->infer(features);
    if (!pred_or.ok()) {
      if (err) *err << "bayes classifier infer should succeed\n";
      return false;
    }
    const auto pred = pred_or.value().to(pluss::pCPU, ScalarType::Float64).host<double>();
    const auto truth = y.to(pluss::pCPU, ScalarType::Float64).host<double>();
    for (std::size_t i = 0; i < pred.size(); ++i) {
      if (!approx(pred[i], truth[i])) {
        if (err) *err << "bayes classifier should fit this separable training set\n";
        return false;
      }
    }

    pluss::ai::nodej::AssessOptions assess{};
    assess.metrics = {MetricKind::Accuracy, MetricKind::LogLoss};
    auto metrics_or = cls->assess(batch, assess);
    if (!metrics_or.ok()) {
      if (err) *err << "bayes classifier assess should succeed\n";
      return false;
    }
    const auto metrics = metrics_or.value().to(pluss::pCPU, ScalarType::Float64).host<double>();
    if (metrics.size() != 2 || metrics[0] < 0.99 || metrics[1] > 0.1) {
      if (err) *err << "bayes classifier metrics should show strong fit and low log-loss\n";
      return false;
    }
  }

  {
    const Series x_train = pluss::make_series(std::vector<double>{
                                                  -1.0, -1.0,
                                                  -0.8, -0.9,
                                                  -0.9, -0.7,
                                                   0.9,  0.8,
                                                   1.1,  0.9,
                                                   0.8,  1.0},
                                              {6, 2},
                                              f64());
    const FeatureBatch train{.x = x_train, .layout = ObservationLayout::Rows};
    const Series x_query = pluss::make_series(std::vector<double>{
                                                  -1.0, -1.0,
                                                   1.0,  1.0,
                                                   6.0,  6.0},
                                              {3, 2},
                                              f64());
    const FeatureBatch query{.x = x_query, .layout = ObservationLayout::Rows};

    const ModelSpec spec{
        .task = TaskKind::Transform,
        .family = ModelFamily::Kernel,
        .options = KernelFamilyOptions{
            .kernel = KernelKind::Rbf,
            .form = KernelForm::OneClass,
            .gamma = 0.7,
            .nu = 0.34,
        },
    };
    auto tr_or = pluss::ai::nodej::make_transformer(spec);
    if (!tr_or.ok()) {
      if (err) *err << "make_transformer should build kernel one-class transformer\n";
      return false;
    }
    auto tr = tr_or.move_value();
    if (!tr->learn(train).ok()) {
      if (err) *err << "kernel one-class transformer learn should succeed\n";
      return false;
    }
    auto map_or = tr->map(query);
    if (!map_or.ok()) {
      if (err) *err << "kernel one-class transformer map should succeed\n";
      return false;
    }
    const auto labels = map_or.value().to(pluss::pCPU, ScalarType::Float64).reshape({3}).host<double>();
    if (labels.size() != 3 || !std::isfinite(labels[0]) || !std::isfinite(labels[1]) || !std::isfinite(labels[2])) {
      if (err) *err << "kernel one-class transformer should tag far sample as outlier\n";
      return false;
    }
    if (!(labels[2] < 0.0 && (labels[0] > 0.0 || labels[1] > 0.0))) {
      if (err) *err << "kernel one-class transformer should reject far sample and retain a near sample\n";
      return false;
    }

    auto state_or = tr->dump_state();
    if (!state_or.ok()) {
      if (err) *err << "kernel one-class transformer dump_state should succeed\n";
      return false;
    }
    auto reload_or = pluss::ai::nodej::make_transformer(spec);
    if (!reload_or.ok()) {
      if (err) *err << "make_transformer should build one-class transformer for reload\n";
      return false;
    }
    auto reloaded = reload_or.move_value();
    if (!reloaded->load_state(state_or.value()).ok()) {
      if (err) *err << "kernel one-class transformer load_state should succeed\n";
      return false;
    }
    auto reload_map_or = reloaded->map(query);
    if (!reload_map_or.ok()) {
      if (err) *err << "reloaded kernel one-class transformer map should succeed\n";
      return false;
    }
    const auto reload_labels = reload_map_or.value().to(pluss::pCPU, ScalarType::Float64).reshape({3}).host<double>();
    for (std::size_t i = 0; i < labels.size(); ++i) {
      if (!approx(reload_labels[i], labels[i])) {
        if (err) *err << "reloaded one-class transformer should reproduce labels\n";
        return false;
      }
    }
  }

  {
    const Series x = pluss::make_series(std::vector<double>{
                                            -1.0, -1.0,
                                            -1.0,  1.0,
                                             1.0, -1.0,
                                             1.0,  1.0,
                                            -0.8, -0.8,
                                            -0.8,  0.8,
                                             0.8, -0.8,
                                             0.8,  0.8},
                                        {8, 2},
                                        f64());
    const Series y = pluss::make_series(std::vector<double>{0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0}, {8}, f64());
    const FeatureBatch features{.x = x, .layout = ObservationLayout::Rows};
    const LabeledBatch batch{.features = features, .y = y};

    const ModelSpec spec{
        .task = TaskKind::Classification,
        .family = ModelFamily::Kernel,
        .options = KernelFamilyOptions{
            .kernel = KernelKind::Rbf,
            .form = KernelForm::Nu,
            .c = 50.0,
            .gamma = 2.0,
            .epsilon = 0.0,
            .nu = 0.5,
        },
    };
    auto cls_or = pluss::ai::nodej::make_classifier(spec);
    if (!cls_or.ok()) {
      if (err) *err << "make_classifier should succeed for kernel classifier\n";
      return false;
    }
    auto cls = cls_or.move_value();
    if (!cls->learn(batch).ok()) {
      if (err) *err << "kernel classifier learn should succeed\n";
      return false;
    }

    auto pred_or = cls->infer(features);
    if (!pred_or.ok()) {
      if (err) *err << "kernel classifier infer should succeed\n";
      return false;
    }
    const auto pred = pred_or.value().to(pluss::pCPU, ScalarType::Float64).host<double>();
    const auto truth = y.to(pluss::pCPU, ScalarType::Float64).host<double>();
    for (std::size_t i = 0; i < pred.size(); ++i) {
      if (!approx(pred[i], truth[i])) {
        if (err) *err << "kernel classifier should model this XOR pattern\n";
        return false;
      }
    }

    auto metrics_or = cls->assess(batch);
    if (!metrics_or.ok()) {
      if (err) *err << "kernel classifier assess should succeed\n";
      return false;
    }
    const auto metrics = metrics_or.value().to(pluss::pCPU, ScalarType::Float64).host<double>();
    if (metrics.empty() || metrics[0] < 0.99) {
      if (err) *err << "kernel classifier default metric should indicate high accuracy\n";
      return false;
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
    const Series y = pluss::make_series(std::vector<double>{-1.0, -0.5, 0.0, 0.5, 1.0, 1.5}, {6}, f64());
    const FeatureBatch features{.x = x, .layout = ObservationLayout::Rows};
    const LabeledBatch batch{.features = features, .y = y};

    const ModelSpec spec{
        .task = TaskKind::Regression,
        .family = ModelFamily::Kernel,
        .options = KernelFamilyOptions{
            .kernel = KernelKind::Rbf,
            .form = KernelForm::Nu,
            .c = 100.0,
            .gamma = 0.7,
            .epsilon = 0.0,
            .nu = 0.6,
        },
    };
    auto reg_or = pluss::ai::nodej::make_regressor(spec);
    if (!reg_or.ok()) {
      if (err) *err << "make_regressor should succeed for kernel regressor\n";
      return false;
    }
    auto reg = reg_or.move_value();
    if (!reg->learn(batch).ok()) {
      if (err) *err << "kernel regressor learn should succeed\n";
      return false;
    }

    auto pred_or = reg->infer(features);
    if (!pred_or.ok()) {
      if (err) *err << "kernel regressor infer should succeed\n";
      return false;
    }
    const auto pred = pred_or.value().to(pluss::pCPU, ScalarType::Float64).host<double>();
    const auto truth = y.to(pluss::pCPU, ScalarType::Float64).host<double>();
    for (std::size_t i = 0; i < pred.size(); ++i) {
      if (!approx(pred[i], truth[i], 5e-2)) {
        if (err) *err << "kernel regressor should approximate training signal closely\n";
        return false;
      }
    }

    auto metrics_or = reg->assess(batch);
    if (!metrics_or.ok()) {
      if (err) *err << "kernel regressor assess should succeed\n";
      return false;
    }
    const auto metrics = metrics_or.value().to(pluss::pCPU, ScalarType::Float64).host<double>();
    if (metrics.size() < 3 || metrics[0] > 1e-2 || metrics[2] < 0.98) {
      if (err) *err << "kernel regressor metrics should show low error and high R2\n";
      return false;
    }
  }

  {
    const Series x = pluss::make_series(std::vector<double>{0.0, 1.0, 2.0, 3.0}, {4, 1}, f64());
    const Series y = pluss::make_series(std::vector<double>{0.0, 0.0, 1.0, 1.0}, {4}, f64());
    const FeatureBatch features{.x = x, .layout = ObservationLayout::Rows};
    const LabeledBatch batch{.features = features, .y = y};

    const ModelSpec bernoulli_bayes{
        .task = TaskKind::Classification,
        .family = ModelFamily::Bayes,
        .options = BayesFamilyOptions{
            .use_gaussian = false,
            .smoothing = 1e-9,
        },
    };
    auto bayes_or = pluss::ai::nodej::make_classifier(bernoulli_bayes);
    if (!bayes_or.ok()) {
      if (err) *err << "builder should still construct bayes classifier\n";
      return false;
    }
    auto bayes = bayes_or.move_value();
    const auto bayes_status = bayes->learn(batch);
    if (!bayes_status.ok()) {
      if (err) *err << "bayes classifier should support legacy non-gaussian Bernoulli mode\n";
      return false;
    }
    auto bernoulli_pred_or = bayes->infer(features);
    if (!bernoulli_pred_or.ok()) {
      if (err) *err << "bernoulli bayes infer should succeed\n";
      return false;
    }

    const ModelSpec multinomial_bayes{
        .task = TaskKind::Classification,
        .family = ModelFamily::Bayes,
        .options = BayesFamilyOptions{
            .likelihood = BayesLikelihood::Multinomial,
            .use_gaussian = true,
            .smoothing = 1e-9,
        },
    };
    auto multi_or = pluss::ai::nodej::make_classifier(multinomial_bayes);
    if (!multi_or.ok()) {
      if (err) *err << "builder should construct multinomial bayes classifier\n";
      return false;
    }
    auto multi = multi_or.move_value();
    const auto multi_status = multi->learn(batch);
    if (!multi_status.ok()) {
      if (err) *err << "multinomial bayes classifier learn should succeed\n";
      return false;
    }
    auto multi_metrics_or = multi->assess(batch, AssessOptions{.metrics = {MetricKind::Accuracy, MetricKind::LogLoss}});
    if (!multi_metrics_or.ok()) {
      if (err) *err << "multinomial bayes classifier assess should succeed\n";
      return false;
    }
    const auto multi_metrics = multi_metrics_or.value().to(pluss::pCPU, ScalarType::Float64).host<double>();
    if (multi_metrics.size() != 2 || multi_metrics[0] < 0.5 || multi_metrics[1] > 1.0) {
      if (err) *err << "multinomial bayes classifier metrics should be finite and reasonable\n";
      return false;
    }
    const LabeledBatch negative_count_batch{
        .features = FeatureBatch{
            .x = pluss::make_series(std::vector<double>{-1.0, 2.0}, {2, 1}, f64()),
            .layout = ObservationLayout::Rows,
        },
        .y = pluss::make_series(std::vector<double>{0.0, 1.0}, {2}, f64()),
    };
    auto bad_multi_or = pluss::ai::nodej::make_classifier(multinomial_bayes);
    if (!bad_multi_or.ok()) {
      if (err) *err << "builder should construct multinomial bayes classifier for negative-count check\n";
      return false;
    }
    const auto bad_multi_status = bad_multi_or.value()->learn(negative_count_batch);
    if (bad_multi_status.ok() || bad_multi_status.code != StatusCode::InvalidArgument) {
      if (err) *err << "multinomial bayes classifier should reject negative training counts\n";
      return false;
    }
    const auto bad_multi_infer_or = multi->infer(negative_count_batch.features);
    if (bad_multi_infer_or.ok() || bad_multi_infer_or.status().code != StatusCode::InvalidArgument) {
      if (err) *err << "multinomial bayes classifier should reject negative inference counts\n";
      return false;
    }

    const ModelSpec complement_bayes{
        .task = TaskKind::Classification,
        .family = ModelFamily::Bayes,
        .options = BayesFamilyOptions{
            .likelihood = BayesLikelihood::Complement,
            .use_gaussian = true,
            .smoothing = 1e-6,
        },
    };
    const LabeledBatch complement_batch{
        .features = FeatureBatch{
            .x = pluss::make_series(std::vector<double>{4.0, 0.0, 3.0, 0.0, 0.0, 4.0, 0.0, 3.0}, {4, 2}, f64()),
            .layout = ObservationLayout::Rows,
        },
        .y = y,
    };
    auto complement_or = pluss::ai::nodej::make_classifier(complement_bayes);
    if (!complement_or.ok()) {
      if (err) *err << "builder should construct complement bayes classifier\n";
      return false;
    }
    auto complement = complement_or.move_value();
    if (!complement->learn(complement_batch).ok()) {
      if (err) *err << "complement bayes classifier learn should succeed\n";
      return false;
    }
    const FeatureBatch complement_query{
        .x = pluss::make_series(std::vector<double>{5.0, 0.0, 0.0, 5.0}, {2, 2}, f64()),
        .layout = ObservationLayout::Rows,
    };
    auto complement_pred_or = complement->infer(complement_query);
    if (!complement_pred_or.ok()) {
      if (err) *err << "complement bayes classifier infer should succeed\n";
      return false;
    }
    const auto complement_pred = complement_pred_or.value().to(pluss::pCPU, ScalarType::Float64).host<double>();
    if (complement_pred.size() != 2 || complement_pred[0] > 0.5 || complement_pred[1] < 0.5) {
      if (err) *err << "complement bayes classifier should separate count complements\n";
      return false;
    }
    auto complement_state_or = complement->dump_state();
    auto complement_reload_or = pluss::ai::nodej::make_classifier(complement_bayes);
    if (!complement_state_or.ok() || !complement_reload_or.ok()) {
      if (err) *err << "complement bayes state dump/reload builders should succeed\n";
      return false;
    }
    auto complement_reload = complement_reload_or.move_value();
    if (!complement_reload->load_state(complement_state_or.value()).ok()) {
      if (err) *err << "complement bayes classifier load_state should succeed\n";
      return false;
    }
    auto complement_reload_pred_or = complement_reload->infer(complement_query);
    if (!complement_reload_pred_or.ok()) {
      if (err) *err << "reloaded complement bayes classifier infer should succeed\n";
      return false;
    }
    const auto complement_reload_pred = complement_reload_pred_or.value().to(pluss::pCPU, ScalarType::Float64).host<double>();
    if (complement_reload_pred.size() != complement_pred.size()) {
      if (err) *err << "reloaded complement bayes classifier prediction size mismatch\n";
      return false;
    }
    for (std::size_t i = 0; i < complement_pred.size(); ++i) {
      if (!approx(complement_reload_pred[i], complement_pred[i], 1e-9)) {
        if (err) *err << "reloaded complement bayes classifier should reproduce predictions\n";
        return false;
      }
    }
    const FeatureBatch negative_complement_query{
        .x = pluss::make_series(std::vector<double>{1.0, -1.0}, {1, 2}, f64()),
        .layout = ObservationLayout::Rows,
    };
    const auto bad_complement_infer_or = complement->infer(negative_complement_query);
    if (bad_complement_infer_or.ok() || bad_complement_infer_or.status().code != StatusCode::InvalidArgument) {
      if (err) *err << "complement bayes classifier should reject negative inference counts\n";
      return false;
    }

    const ModelSpec bad_kernel{
        .task = TaskKind::Regression,
        .family = ModelFamily::Kernel,
        .options = KernelFamilyOptions{
            .kernel = KernelKind::Rbf,
            .c = -1.0,
            .gamma = 0.5,
            .epsilon = 0.0,
        },
    };
    auto kernel_or = pluss::ai::nodej::make_regressor(bad_kernel);
    if (!kernel_or.ok()) {
      if (err) *err << "builder should still construct kernel regressor\n";
      return false;
    }
    auto kernel = kernel_or.move_value();
    const auto kernel_status = kernel->learn(batch);
    if (kernel_status.ok() || kernel_status.code != StatusCode::InvalidArgument) {
      if (err) *err << "kernel regressor should reject invalid c option\n";
      return false;
    }
  }

  {
    const Series x = pluss::make_series(std::vector<double>{
                                            -1.0, -1.0,
                                            -0.8, -0.9,
                                             0.9,  0.8,
                                             1.1,  0.9},
                                        {4, 2},
                                        f64());
    const Series y = pluss::make_series(std::vector<double>{0.0, 0.0, 1.0, 1.0}, {4}, f64());
    const FeatureBatch features{.x = x, .layout = ObservationLayout::Rows};
    const LabeledBatch batch{.features = features, .y = y};

    auto nu_model = pluss::ai::lymph::models::kernel::nu_svm_classifier(KernelFamilyOptions{
        .kernel = KernelKind::Rbf,
        .c = 25.0,
        .gamma = 1.0,
        .nu = 0.5,
    }).learn(batch);
    const auto nu_pred = nu_model.infer(features).to(pluss::pCPU, ScalarType::Float64).host<double>();
    if (nu_pred.size() != 4 || nu_pred.front() > 0.5 || nu_pred.back() < 0.5) {
      if (err) *err << "lymph nu_svm_classifier factory should train and infer through NodeJ\n";
      return false;
    }

    auto one_class = pluss::ai::lymph::models::kernel::one_class_svm(KernelFamilyOptions{
        .kernel = KernelKind::Rbf,
        .gamma = 0.8,
        .nu = 0.5,
    });
    const Series oc = one_class.learn_map(FeatureBatch{
        .x = pluss::make_series(std::vector<double>{-1.0, -1.0, -0.9, -0.8, 1.0, 1.0, 7.0, 7.0}, {4, 2}, f64()),
        .layout = ObservationLayout::Rows,
    });
    const auto oc_labels = oc.to(pluss::pCPU, ScalarType::Float64).reshape({4}).host<double>();
    if (oc_labels.size() != 4 || oc_labels.back() >= 0.0) {
      if (err) *err << "lymph one_class_svm factory should expose kernel one-class mapping\n";
      return false;
    }

    auto bernoulli = pluss::ai::lymph::models::bayes::bernoulli_nb_classifier(BayesFamilyOptions{
        .smoothing = 1e-6,
    }).learn(batch);
    const auto bernoulli_pred = bernoulli.infer(features).to(pluss::pCPU, ScalarType::Float64).host<double>();
    if (bernoulli_pred.size() != 4 || bernoulli_pred.front() > 0.5 || bernoulli_pred.back() < 0.5) {
      if (err) *err << "lymph bernoulli_nb_classifier factory should train and infer through NodeJ\n";
      return false;
    }

    auto multinomial = pluss::ai::lymph::models::bayes::multinomial_nb_classifier(BayesFamilyOptions{
        .smoothing = 1e-6,
    }).learn(LabeledBatch{
        .features = FeatureBatch{
            .x = pluss::make_series(std::vector<double>{3.0, 0.0, 2.0, 0.0, 0.0, 3.0, 0.0, 2.0}, {4, 2}, f64()),
            .layout = ObservationLayout::Rows,
        },
        .y = y,
    });
    const auto multinomial_pred = multinomial.infer(FeatureBatch{
        .x = pluss::make_series(std::vector<double>{4.0, 0.0, 0.0, 4.0}, {2, 2}, f64()),
        .layout = ObservationLayout::Rows,
    }).to(pluss::pCPU, ScalarType::Float64).host<double>();
    if (multinomial_pred.size() != 2 || multinomial_pred[0] > 0.5 || multinomial_pred[1] < 0.5) {
      if (err) *err << "lymph multinomial_nb_classifier factory should train and infer through NodeJ\n";
      return false;
    }

    auto complement = pluss::ai::lymph::models::bayes::complement_nb_classifier(BayesFamilyOptions{
        .smoothing = 1e-6,
    }).learn(LabeledBatch{
        .features = FeatureBatch{
            .x = pluss::make_series(std::vector<double>{4.0, 0.0, 3.0, 0.0, 0.0, 4.0, 0.0, 3.0}, {4, 2}, f64()),
            .layout = ObservationLayout::Rows,
        },
        .y = y,
    });
    const auto complement_pred = complement.infer(FeatureBatch{
        .x = pluss::make_series(std::vector<double>{5.0, 0.0, 0.0, 5.0}, {2, 2}, f64()),
        .layout = ObservationLayout::Rows,
    }).to(pluss::pCPU, ScalarType::Float64).host<double>();
    if (complement_pred.size() != 2 || complement_pred[0] > 0.5 || complement_pred[1] < 0.5) {
      if (err) *err << "lymph complement_nb_classifier factory should train and infer through NodeJ\n";
      return false;
    }

    const Series xr = pluss::make_series(std::vector<double>{
                                             -2.0, -1.0,
                                             -1.0, -0.5,
                                              0.0,  0.0,
                                              1.0,  0.5,
                                              2.0,  1.0},
                                         {5, 2},
                                         f64());
    const Series yr = pluss::make_series(std::vector<double>{-1.0, -0.5, 0.0, 0.5, 1.0}, {5}, f64());
    const FeatureBatch regression_features{.x = xr, .layout = ObservationLayout::Rows};
    const LabeledBatch regression_batch{.features = regression_features, .y = yr};
    auto nu_reg = pluss::ai::lymph::models::kernel::nu_svm_regressor(KernelFamilyOptions{
        .kernel = KernelKind::Rbf,
        .c = 100.0,
        .gamma = 0.7,
        .epsilon = 0.0,
        .nu = 0.6,
    }).learn(regression_batch);
    const auto nu_reg_pred = nu_reg.infer(regression_features).to(pluss::pCPU, ScalarType::Float64).host<double>();
    if (nu_reg_pred.size() != 5 || std::abs(nu_reg_pred.front() - yr.to(pluss::pCPU, ScalarType::Float64).host<double>().front()) > 0.1) {
      if (err) *err << "lymph nu_svm_regressor factory should train and infer through NodeJ\n";
      return false;
    }

    auto bernoulli_reg = pluss::ai::lymph::models::bayes::bernoulli_nb_regressor(BayesFamilyOptions{
        .smoothing = 1e-6,
    }).learn(batch, TrainOptions{.max_iterations = 100, .tolerance = 1e-5});
    const auto bernoulli_reg_pred = bernoulli_reg.infer(features).to(pluss::pCPU, ScalarType::Float64).host<double>();
    if (bernoulli_reg_pred.size() != 4) {
      if (err) *err << "lymph bernoulli_nb_regressor prediction size mismatch\n";
      return false;
    }
    for (double v : bernoulli_reg_pred) {
      if (!(v >= -1e-9 && v <= 1.0 + 1e-9)) {
        if (err) *err << "lymph bernoulli_nb_regressor should emit probabilities in [0,1]\n";
        return false;
      }
    }

    const LabeledBatch count_regression_batch{
        .features = FeatureBatch{
            .x = pluss::make_series(std::vector<double>{3.0, 0.0, 2.0, 0.0, 0.0, 3.0, 0.0, 2.0}, {4, 2}, f64()),
            .layout = ObservationLayout::Rows,
        },
        .y = pluss::make_series(std::vector<double>{1.0, 2.0, 6.0, 7.0}, {4}, f64()),
    };
    auto multinomial_reg = pluss::ai::lymph::models::bayes::multinomial_nb_regressor(BayesFamilyOptions{
        .smoothing = 1e-6,
    }).learn(count_regression_batch);
    const auto multinomial_reg_pred = multinomial_reg.infer(FeatureBatch{
        .x = pluss::make_series(std::vector<double>{4.0, 0.0, 0.0, 4.0}, {2, 2}, f64()),
        .layout = ObservationLayout::Rows,
    }).to(pluss::pCPU, ScalarType::Float64).host<double>();
    if (multinomial_reg_pred.size() != 2 || !(multinomial_reg_pred[0] < multinomial_reg_pred[1])) {
      if (err) *err << "lymph multinomial_nb_regressor factory should preserve count-driven ordering\n";
      return false;
    }
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("nodej bayes and kernel families") {
  REQUIRE(run_nodej_bayes_kernel_checks(&std::cerr));
}
#else
int main() {
  if (!run_nodej_bayes_kernel_checks(&std::cerr)) {
    return 1;
  }
  std::cout << "nodej_bayes_kernel_test passed\n";
  return 0;
}
#endif
