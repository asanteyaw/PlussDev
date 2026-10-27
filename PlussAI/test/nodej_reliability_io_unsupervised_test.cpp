#include <pluss/ai/ml.h>

#include <cmath>
#include <cstdint>
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
using namespace pluss::ai::nodej;

Config f64() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

std::vector<double> as_f64(const Series& x) {
  return x.to(pluss::pCPU, ScalarType::Float64).reshape({x.numel()}).host<double>();
}

std::int64_t positive_count(const Series& labels) {
  return labels.to(pluss::pCPU, ScalarType::Float64).ge(0.5).to(ScalarType::Int64).sum().item<std::int64_t>();
}

bool run_nodej_reliability_io_unsupervised_checks(std::ostream* err = nullptr) {
  {
    std::vector<double> xv{};
    std::vector<double> yv{};
    for (int i = 0; i < 160; ++i) {
      const double x0 = -2.0 + 4.0 * static_cast<double>(i) / 159.0;
      const double x1 = 0.5 * x0;
      const bool is_pos = (i % 8) == 0;
      xv.push_back(x0);
      xv.push_back(x1);
      yv.push_back(is_pos ? 1.0 : 0.0);
    }
    const LabeledBatch batch{
        .features = FeatureBatch{.x = pluss::make_series(xv, {160, 2}, f64()), .layout = ObservationLayout::Rows},
        .y = pluss::make_series(yv, {160}, f64()),
    };

    ModelSpec spec{
        .task = TaskKind::Classification,
        .family = ModelFamily::Linear,
        .options = LinearFamilyOptions{
            .objective = LinearObjective::Logistic,
            .regularization = RegularizationKind::L2,
            .regularization_strength = 1e-6,
            .fit_intercept = true,
        },
    };

    SplitOptions split{};
    split.plan = CVPlanKind::Holdout;
    split.holdout_fraction = 0.2;
    split.shuffle = true;
    split.stratified = false;
    split.reliability_defaults = true;
    split.random.seed = 42;
    split.random.deterministic = true;

    AssessOptions assess{};
    assess.metrics = {MetricKind::Recall, MetricKind::Precision};

    auto cv_or = cross_assess(spec, batch, split, TrainOptions{}, assess);
    if (!cv_or.ok()) {
      if (err) *err << "cross_assess should succeed with reliability defaults\n";
      return false;
    }
    const auto& fold = cv_or.value().folds.front();
    const Series y = batch.y.reshape({batch.y.numel()});
    const Series test_y = y.index_select(0, fold.test_indices.to(ScalarType::Int64));
    const Series train_y = y.index_select(0, fold.train_indices.to(ScalarType::Int64));
    if (!(test_y.sum().item<double>() > 0.0 && train_y.sum().item<double>() > 0.0)) {
      if (err) *err << "reliability stratified defaults should keep positive class in both splits\n";
      return false;
    }

    auto plain_or = make_classifier(spec);
    auto weighted_or = make_classifier(spec);
    auto isotonic_or = make_classifier(spec);
    auto temperature_or = make_classifier(spec);
    if (!plain_or.ok() || !weighted_or.ok() || !isotonic_or.ok() || !temperature_or.ok()) {
      if (err) *err << "make_classifier should succeed for reliability checks\n";
      return false;
    }
    auto plain = plain_or.move_value();
    auto weighted = weighted_or.move_value();
    auto isotonic = isotonic_or.move_value();
    auto temperature = temperature_or.move_value();
    TrainOptions plain_opts{};
    plain_opts.max_iterations = 3000;
    plain_opts.tolerance = 1e-8;
    TrainOptions weighted_opts = plain_opts;
    weighted_opts.class_weight = ClassWeightMode::Balanced;
    weighted_opts.calibration = ScoreCalibration::Platt;
    weighted_opts.calibration_iterations = 300;
    weighted_opts.calibration_tolerance = 1e-7;
    TrainOptions isotonic_opts = weighted_opts;
    isotonic_opts.calibration = ScoreCalibration::Isotonic;
    TrainOptions temperature_opts = weighted_opts;
    temperature_opts.calibration = ScoreCalibration::Temperature;

    if (!plain->learn(batch, plain_opts).ok()
        || !weighted->learn(batch, weighted_opts).ok()
        || !isotonic->learn(batch, isotonic_opts).ok()
        || !temperature->learn(batch, temperature_opts).ok()) {
      if (err) *err << "plain/calibrated learn should succeed\n";
      return false;
    }
    auto pred_plain_or = plain->infer(batch.features, InferOptions{.positive_threshold = 0.8});
    auto pred_weight_or = weighted->infer(batch.features, InferOptions{.positive_threshold = 0.8});
    auto pred_lo_or = weighted->infer(batch.features, InferOptions{.positive_threshold = 0.3});
    auto pred_iso_hi_or = isotonic->infer(batch.features, InferOptions{.positive_threshold = 0.8});
    auto pred_iso_lo_or = isotonic->infer(batch.features, InferOptions{.positive_threshold = 0.3});
    auto pred_tmp_hi_or = temperature->infer(batch.features, InferOptions{.positive_threshold = 0.8});
    auto pred_tmp_lo_or = temperature->infer(batch.features, InferOptions{.positive_threshold = 0.3});
    if (!pred_plain_or.ok()
        || !pred_weight_or.ok()
        || !pred_lo_or.ok()
        || !pred_iso_hi_or.ok()
        || !pred_iso_lo_or.ok()
        || !pred_tmp_hi_or.ok()
        || !pred_tmp_lo_or.ok()) {
      if (err) *err << "thresholded infer should succeed\n";
      return false;
    }
    const std::int64_t hi_plain = positive_count(pred_plain_or.value());
    const std::int64_t hi_weight = positive_count(pred_weight_or.value());
    const std::int64_t lo_weight = positive_count(pred_lo_or.value());
    const std::int64_t hi_iso = positive_count(pred_iso_hi_or.value());
    const std::int64_t lo_iso = positive_count(pred_iso_lo_or.value());
    const std::int64_t hi_tmp = positive_count(pred_tmp_hi_or.value());
    const std::int64_t lo_tmp = positive_count(pred_tmp_lo_or.value());
    if (!(hi_weight >= hi_plain
          && lo_weight >= hi_weight
          && lo_iso >= hi_iso
          && lo_tmp >= hi_tmp)) {
      if (err) *err << "class weighting + threshold hooks should produce monotonic positive prediction behavior\n";
      return false;
    }
  }

  {
    const Series x = pluss::make_series(std::vector<double>{
                                            -2.0, -1.0,
                                            -1.0, -0.5,
                                             1.0,  0.5,
                                             2.0,  1.0},
                                        {4, 2},
                                        f64());
    const Series y = pluss::make_series(std::vector<double>{0.0, 0.0, 1.0, 1.0}, {4}, f64());
    const LabeledBatch batch{
        .features = FeatureBatch{.x = x, .layout = ObservationLayout::Rows},
        .y = y,
    };
    ModelSpec spec{
        .task = TaskKind::Classification,
        .family = ModelFamily::Linear,
        .options = LinearFamilyOptions{
            .objective = LinearObjective::Logistic,
            .regularization = RegularizationKind::L2,
            .regularization_strength = 1e-6,
            .fit_intercept = true,
        },
    };
    auto flow_or = make_flow(spec);
    if (!flow_or.ok()) {
      if (err) *err << "make_flow should succeed for io bundle roundtrip\n";
      return false;
    }
    Flow flow = flow_or.move_value();
    if (!flow.add_standard_scaler().ok()) {
      if (err) *err << "add_standard_scaler should succeed\n";
      return false;
    }
    TrainOptions train{};
    train.max_iterations = 2500;
    train.tolerance = 1e-8;
    train.random.seed = 9;
    train.random.deterministic = true;
    if (!flow.learn(batch, train).ok()) {
      if (err) *err << "flow learn should succeed\n";
      return false;
    }
    auto pred_before_or = flow.infer(batch.features);
    if (!pred_before_or.ok()) {
      if (err) *err << "flow infer should succeed before persistence\n";
      return false;
    }
    const std::string bundle_path = "/tmp/nodej_reliability_io_unsup.bundle";
    auto save_status = pluss::ai::nodej::io::save(bundle_path, flow, LabeledBatch{}, train);
    if (!save_status.ok()) {
      if (err) *err << "save should succeed via fitted snapshot path without train-batch fallback\n";
      return false;
    }
    auto restored_or = pluss::ai::nodej::io::load_and_restore(bundle_path);
    if (!restored_or.ok()) {
      if (err) *err << "load_and_restore should succeed\n";
      return false;
    }
    auto pred_after_or = restored_or.value().infer(batch.features);
    if (!pred_after_or.ok()) {
      if (err) *err << "restored flow infer should succeed\n";
      return false;
    }
    const auto before = as_f64(pred_before_or.value());
    const auto after = as_f64(pred_after_or.value());
    if (before.size() != after.size()) {
      if (err) *err << "flow bundle roundtrip should preserve prediction size\n";
      return false;
    }
    for (std::size_t i = 0; i < before.size(); ++i) {
      if (std::abs(before[i] - after[i]) > 1e-9) {
        if (err) *err << "flow bundle roundtrip should preserve predictions\n";
        return false;
      }
    }
  }

  {
    ModelSpec spec{
        .task = TaskKind::Transform,
        .family = ModelFamily::Kernel,
        .options = KernelFamilyOptions{
            .kernel = KernelKind::Rbf,
            .form = KernelForm::OneClass,
            .gamma = 0.8,
            .nu = 0.5,
        },
    };
    auto flow_or = make_flow(spec);
    if (!flow_or.ok()) {
      if (err) *err << "make_flow(transform one-class) should succeed for io bundle roundtrip\n";
      return false;
    }
    Flow flow = flow_or.move_value();
    if (flow.has_opaque_transforms()) {
      if (err) *err << "transform one-class flow should be serializable, not opaque\n";
      return false;
    }
    FeatureBatch batch{
        .x = pluss::make_series(std::vector<double>{
                                    -1.0, -1.0,
                                    -0.9, -0.8,
                                     1.0,  1.0,
                                     7.0,  7.0},
                                {4, 2},
                                f64()),
        .layout = ObservationLayout::Rows,
    };
    auto labels_before_or = flow.learn_map(batch);
    if (!labels_before_or.ok()) {
      if (err) *err << "transform one-class flow learn_map should succeed before persistence\n";
      return false;
    }
    const std::string bundle_path = "/tmp/nodej_one_class_transform_flow.bundle";
    auto save_status = pluss::ai::nodej::io::save(bundle_path, flow, LabeledBatch{}, TrainOptions{});
    if (!save_status.ok()) {
      if (err) *err << "transform one-class flow save should use fitted snapshot path\n";
      return false;
    }
    auto restored_or = pluss::ai::nodej::io::load_and_restore(bundle_path);
    if (!restored_or.ok()) {
      if (err) *err << "transform one-class flow load_and_restore should succeed\n";
      return false;
    }
    auto labels_after_or = restored_or.value().map(batch);
    if (!labels_after_or.ok()) {
      if (err) *err << "restored transform one-class flow map should succeed\n";
      return false;
    }
    const auto before = as_f64(labels_before_or.value());
    const auto after = as_f64(labels_after_or.value());
    if (before.size() != after.size()) {
      if (err) *err << "transform one-class flow roundtrip should preserve label size\n";
      return false;
    }
    for (std::size_t i = 0; i < before.size(); ++i) {
      if (std::abs(before[i] - after[i]) > 1e-9) {
        if (err) *err << "transform one-class flow roundtrip should preserve labels\n";
        return false;
      }
    }
  }

  {
    std::vector<double> vals{};
    for (int i = 0; i < 20; ++i) {
      vals.push_back(-2.0 + 0.05 * i);
      vals.push_back(-1.5 + 0.03 * i);
    }
    for (int i = 0; i < 20; ++i) {
      vals.push_back(1.8 + 0.04 * i);
      vals.push_back(2.2 + 0.02 * i);
    }
    FeatureBatch xb{
        .x = pluss::make_series(vals, {40, 2}, f64()),
        .layout = ObservationLayout::Rows,
    };
    ModelSpec kmeans_spec{
        .task = TaskKind::Transform,
        .family = ModelFamily::Cluster,
        .options = ClusterFamilyOptions{
            .method = ClusterMethod::KMeans,
            .clusters = 2,
            .max_iterations = 200,
            .restarts = 4,
            .tolerance = 1e-5,
        },
    };
    auto kmeans_or = make_transformer(kmeans_spec);
    if (!kmeans_or.ok()) {
      if (err) *err << "make_transformer(kmeans) should succeed\n";
      return false;
    }
    auto kmeans = kmeans_or.move_value();
    if (!kmeans->learn(xb).ok()) {
      if (err) *err << "kmeans learn should succeed\n";
      return false;
    }
    auto labels_or = kmeans->map(xb);
    if (!labels_or.ok()) {
      if (err) *err << "kmeans map should succeed\n";
      return false;
    }
    const auto labels = as_f64(labels_or.value());
    bool seen0 = false;
    bool seen1 = false;
    for (double v : labels) {
      if (std::llround(v) == 0) seen0 = true;
      if (std::llround(v) == 1) seen1 = true;
    }
    if (!(seen0 && seen1)) {
      if (err) *err << "kmeans should assign points to at least two clusters\n";
      return false;
    }

    std::vector<double> pvals{};
    for (int i = 0; i < 60; ++i) {
      const double x0 = -2.0 + 4.0 * static_cast<double>(i) / 59.0;
      const double x1 = 0.8 * x0;
      const double x2 = -0.4 * x0 + 0.6 * x1;
      pvals.push_back(x0);
      pvals.push_back(x1);
      pvals.push_back(x2);
    }
    FeatureBatch pbatch{
        .x = pluss::make_series(pvals, {60, 3}, f64()),
        .layout = ObservationLayout::Rows,
    };
    ModelSpec pca_spec{
        .task = TaskKind::Transform,
        .family = ModelFamily::Decompose,
        .options = DecomposeFamilyOptions{
            .method = DecomposeMethod::Pca,
            .components = 2,
            .whiten = false,
            .max_iterations = 400,
            .tolerance = 1e-6,
        },
    };
    auto pca_or = make_transformer(pca_spec);
    if (!pca_or.ok()) {
      if (err) *err << "make_transformer(pca) should succeed\n";
      return false;
    }
    auto pca = pca_or.move_value();
    if (!pca->learn(pbatch).ok()) {
      if (err) *err << "pca learn should succeed\n";
      return false;
    }
    auto mapped_or = pca->map(pbatch);
    if (!mapped_or.ok()) {
      if (err) *err << "pca map should succeed\n";
      return false;
    }
    const auto sz = mapped_or.value().sizes();
    if (!(sz.size() == 2 && sz[0] == 60 && sz[1] == 2)) {
      if (err) *err << "pca mapped output should have [samples, components] shape\n";
      return false;
    }
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("nodej reliability + io + unsupervised") {
  REQUIRE(run_nodej_reliability_io_unsupervised_checks(&std::cerr));
}
#else
int main() {
  if (!run_nodej_reliability_io_unsupervised_checks(&std::cerr)) {
    return 1;
  }
  std::cout << "nodej_reliability_io_unsupervised_test passed\n";
  return 0;
}
#endif
