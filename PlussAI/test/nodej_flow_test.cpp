#include <pluss/ai/ml.h>
#include <pluss/mic.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <memory>
#include <vector>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;
using pluss::table::DataFrame;
using pluss::table::Slate;
using namespace pluss::ai::nodej;

Config f64() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

std::vector<double> as_f64(const Series& x) {
  return x.to(pluss::pCPU, ScalarType::Float64).reshape({x.numel()}).host<double>();
}

bool finite_series(const Series& x) {
  return x.isfinite().all().item<bool>();
}

DataFrame make_mixed_flow_frame() {
  const Series x0 = pluss::make_series(std::vector<double>{0.0, 1.0, 2.0, 3.0, 4.0, 5.0}, {6}, f64());
  const Series x1 = pluss::make_series(std::vector<double>{1.0, 1.0, 2.0, 0.0, 2.0, 1.0}, {6}, f64());
  const auto note = pluss::make_note({"red", "blue", "red", "green", "blue", "red"});
  const Series epoch_nanos = pluss::make_series(
      std::vector<std::int64_t>{
          1'700'000'000'000'000'000LL, 1'700'000'001'000'000'000LL, 1'700'000'002'000'000'000LL,
          1'700'000'003'000'000'000LL, 1'700'000'004'000'000'000LL, 1'700'000'005'000'000'000LL},
      {6},
      Config{}.with_dtype(ScalarType::Int64).with_platform(pluss::pCPU));
  const Slate ts = Slate(epoch_nanos).cast().timeline();
  const Series y = x0.mul(2.0).sub(x1.mul(0.5)).add(1.0);

  return DataFrame({
      {"x0", Slate(x0)},
      {"x1", Slate(x1)},
      {"category", Slate(note)},
      {"ts", ts},
      {"target", Slate(y)},
  });
}

bool run_nodej_flow_checks(std::ostream* err = nullptr) {
  {
    Flow flow{};
    if (!flow.add_standard_scaler().ok()) {
      if (err) *err << "Flow::add_standard_scaler should succeed\n";
      return false;
    }

    const FeatureBatch features{
        .x = pluss::make_series(std::vector<double>{
                                    1.0,  2.0,
                                    2.0,  1.0,
                                    3.0, -1.0,
                                    4.0,  0.5},
                                {4, 2},
                                f64()),
        .layout = ObservationLayout::Rows,
    };

    auto before_or = flow.map(features);
    if (before_or.ok() || before_or.status().code != StatusCode::Precondition) {
      if (err) *err << "Flow::map should require learn when transform has learnables\n";
      return false;
    }

    auto learned_or = flow.learn_map(features);
    auto mapped_or = flow.map(features);
    if (!learned_or.ok() || !mapped_or.ok()) {
      if (err) *err << "Flow transform-only learn_map/map should succeed\n";
      return false;
    }
    if (learned_or.value().numel() != features.x.numel() || mapped_or.value().numel() != features.x.numel()) {
      if (err) *err << "Flow transform-only map should preserve shape\n";
      return false;
    }
    if (!finite_series(learned_or.value()) || !finite_series(mapped_or.value())) {
      if (err) *err << "Flow transform-only outputs should be finite\n";
      return false;
    }
  }

  {
    const ModelSpec spec{
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
      if (err) *err << "make_flow(transform one-class) should succeed\n";
      return false;
    }
    Flow flow = flow_or.move_value();
    if (flow.task_kind() != TaskKind::Transform || flow.stage_count() != 1 || !flow.has_model_spec()) {
      if (err) *err << "make_flow(transform one-class) should configure a transform-only spec flow\n";
      return false;
    }
    if (flow.has_opaque_transforms()) {
      if (err) *err << "make_flow(transform one-class) should remain serializable, not opaque\n";
      return false;
    }

    const FeatureBatch train{
        .x = pluss::make_series(std::vector<double>{-1.0, -1.0, -0.9, -0.8, 1.0, 1.0, 7.0, 7.0}, {4, 2}, f64()),
        .layout = ObservationLayout::Rows,
    };
    auto labels_or = flow.learn_map(train);
    if (!labels_or.ok()) {
      if (err) *err << "transform spec flow learn_map should succeed\n";
      return false;
    }
    const auto labels = as_f64(labels_or.value());
    if (labels.size() != 4 || labels.back() >= 0.0) {
      if (err) *err << "transform spec flow should reject far one-class sample\n";
      return false;
    }

    auto states_or = flow.dump_transform_states();
    if (!states_or.ok()) {
      if (err) *err << "transform spec flow dump_transform_states should succeed\n";
      return false;
    }
    auto reload_or = make_flow(spec);
    if (!reload_or.ok()) {
      if (err) *err << "make_flow(transform one-class reload) should succeed\n";
      return false;
    }
    Flow reloaded = reload_or.move_value();
    if (!reloaded.load_transform_states(states_or.value()).ok()) {
      if (err) *err << "transform spec flow load_transform_states should succeed\n";
      return false;
    }
    reloaded.mark_trained(true);
    auto reload_or_labels = reloaded.map(train);
    if (!reload_or_labels.ok()) {
      if (err) *err << "reloaded transform spec flow map should succeed\n";
      return false;
    }
    const auto reload_labels = as_f64(reload_or_labels.value());
    if (reload_labels.size() != labels.size()) {
      if (err) *err << "reloaded transform spec flow label count mismatch\n";
      return false;
    }
    for (std::size_t i = 0; i < labels.size(); ++i) {
      if (std::abs(reload_labels[i] - labels[i]) > 1e-9) {
        if (err) *err << "reloaded transform spec flow should reproduce labels\n";
        return false;
      }
    }
  }

  {
    Flow flow{};
    if (!flow.add_one_hot_encoder(prep::OneHotEncoderOptions{}, std::vector<std::int64_t>{1}).ok()) {
      if (err) *err << "Flow routed one-hot stage should be accepted\n";
      return false;
    }
    const FeatureBatch features{
        .x = pluss::make_series(std::vector<double>{
                                    10.0, 0.0, 100.0,
                                    20.0, 1.0, 200.0,
                                    30.0, 0.0, 300.0,
                                    40.0, 2.0, 400.0},
                                {4, 3},
                                f64()),
        .layout = ObservationLayout::Rows,
    };
    auto out_or = flow.learn_map(features);
    if (!out_or.ok()) {
      if (err) *err << "Flow routed one-hot learn_map should succeed\n";
      return false;
    }
    const Series out = out_or.value().reshape({4, out_or.value().sizes()[1]});
    if (out.sizes().size() != 2 || out.sizes()[0] != 4 || out.sizes()[1] <= 3) {
      if (err) *err << "Flow routed one-hot should expand only the routed column\n";
      return false;
    }
    const Series idx0 = pluss::make_series(std::vector<std::int64_t>{0}, {1}, Config{}.with_dtype(ScalarType::Int64).with_platform(pluss::pCPU));
    const Series last = pluss::make_series(std::vector<std::int64_t>{out.sizes()[1] - 1}, {1}, Config{}.with_dtype(ScalarType::Int64).with_platform(pluss::pCPU));
    const Series left = out.index_select(1, idx0).reshape({4});
    const Series right = out.index_select(1, last).reshape({4});
    if (!left.eq(features.x.index_select(1, idx0).reshape({4})).all().item<bool>()
        || !right.eq(features.x.index_select(1, pluss::make_series(std::vector<std::int64_t>{2}, {1}, Config{}.with_dtype(ScalarType::Int64).with_platform(pluss::pCPU))).reshape({4})).all().item<bool>()) {
      if (err) *err << "Flow routed stage should preserve non-routed columns\n";
      return false;
    }
  }

  {
    Flow flow{};
    prep::FeatureTransformer column(prep::FeatureRemainderPolicy::PassThrough);
    if (!column.add_stage(
             std::make_unique<prep::SimpleImputer>(prep::SimpleImputerOptions{
                 .strategy = prep::SimpleImputerStrategy::Mean,
             }),
             {0})
             .ok()
        || !column.add_stage(std::make_unique<prep::OneHotEncoder>(), {1}).ok()) {
      if (err) *err << "FeatureTransformer stage setup should succeed\n";
      return false;
    }
    if (!flow.add_feature_transformer(std::move(column)).ok()) {
      if (err) *err << "Flow::add_feature_transformer should succeed\n";
      return false;
    }
    if (!flow.add_function_transformer(prep::FunctionTransformer([](const Series& in) -> StatusOr<Series> {
           return StatusOr<Series>(in.mul(0.5));
         }))
             .ok()) {
      if (err) *err << "Flow::add_function_transformer should succeed\n";
      return false;
    }
    if (!flow.has_opaque_transforms()) {
      if (err) *err << "Flow should mark callable/column stages as opaque\n";
      return false;
    }

    const FeatureBatch features{
        .x = pluss::make_series(std::vector<double>{
                                    1.0, 0.0, 100.0,
                                    2.0, 1.0, 200.0,
                                    3.0, 0.0, 300.0,
                                    4.0, 2.0, 400.0},
                                {4, 3},
                                f64()),
        .layout = ObservationLayout::Rows,
    };
    auto out_or = flow.learn_map(features);
    if (!out_or.ok()) {
      if (err) *err << "Flow learn_map should succeed with FeatureTransformer + FunctionTransformer\n";
      return false;
    }
    const auto sizes = out_or.value().sizes();
    if (sizes.size() != 2 || sizes[0] != 4 || sizes[1] <= 3) {
      if (err) *err << "Flow output should preserve rows and expand routed encoded columns\n";
      return false;
    }
    if (!finite_series(out_or.value())) {
      if (err) *err << "Flow output from FeatureTransformer + FunctionTransformer should be finite\n";
      return false;
    }
  }

  {
    Flow flow{};
    const auto stage_status = flow.add_column_stage(
        "cat",
        TransformKind::OneHotEncoder,
        TransformOptions{prep::OneHotEncoderOptions{}},
        std::vector<std::int64_t>{1},
        RouteRemainderPolicy::Drop);
    if (!stage_status.ok()) {
      if (err) *err << "Flow::add_column_stage should support named routed stages\n";
      return false;
    }
    auto graph_or = flow.graph_info();
    if (!graph_or.ok() || graph_or.value().stages.size() != 1 || graph_or.value().stages.front().stage_key != "cat") {
      if (err) *err << "Flow::graph_info should expose named stage metadata\n";
      return false;
    }
    auto remainder_or = flow.nested_param("cat::remainder");
    if (!remainder_or.ok() || remainder_or.value() != 1.0) {
      if (err) *err << "Flow::nested_param should expose stage remainder policy\n";
      return false;
    }
    auto max_cat_before_or = flow.nested_param("cat::opt.max_categories");
    if (!max_cat_before_or.ok() || max_cat_before_or.value() != 20.0) {
      if (err) *err << "Flow::nested_param should expose stage option parameters\n";
      return false;
    }
    if (!flow.set_nested_param("cat::opt.max_categories", 3.0).ok()) {
      if (err) *err << "Flow::set_nested_param should update stage option parameters\n";
      return false;
    }
    auto max_cat_after_or = flow.nested_param("cat::opt.max_categories");
    if (!max_cat_after_or.ok() || max_cat_after_or.value() != 3.0) {
      if (err) *err << "Flow::set_nested_param should persist stage option updates\n";
      return false;
    }
    const auto keys = flow.nested_param_keys();
    if (std::find(keys.begin(), keys.end(), "cat::opt.max_categories") == keys.end()) {
      if (err) *err << "Flow::nested_param_keys should include stage option keys\n";
      return false;
    }

    const FeatureBatch features{
        .x = pluss::make_series(std::vector<double>{
                                    10.0, 0.0, 100.0,
                                    20.0, 1.0, 200.0,
                                    30.0, 0.0, 300.0,
                                    40.0, 2.0, 400.0},
                                {4, 3},
                                f64()),
        .layout = ObservationLayout::Rows,
    };

    auto drop_or = flow.learn_map(features);
    if (!drop_or.ok()) {
      if (err) *err << "Flow::learn_map should succeed for named routed drop stage\n";
      return false;
    }
    const auto drop_sizes = drop_or.value().sizes();
    if (drop_sizes.size() != 2 || drop_sizes[0] != 4 || drop_sizes[1] <= 0) {
      if (err) *err << "RouteRemainderPolicy::Drop should exclude non-routed features\n";
      return false;
    }

    if (!flow.set_nested_param("cat::remainder", 0.0).ok()) {
      if (err) *err << "Flow::set_nested_param should allow stage remainder updates\n";
      return false;
    }
    auto pass_or = flow.learn_map(features);
    if (!pass_or.ok()) {
      if (err) *err << "Flow::learn_map should succeed after nested remainder update\n";
      return false;
    }
    const auto pass_sizes = pass_or.value().sizes();
    if (pass_sizes.size() != 2 || pass_sizes[0] != 4 || pass_sizes[1] <= drop_sizes[1]) {
      if (err) *err << "RouteRemainderPolicy::PassThrough should preserve non-routed features\n";
      return false;
    }

    auto blueprint_or = flow.save_blueprint();
    if (!blueprint_or.ok()) {
      if (err) *err << "Flow::save_blueprint should succeed for non-opaque transforms\n";
      return false;
    }
    if (blueprint_or.value().find("pluss_flow_blueprint_v2\n") != 0) {
      if (err) *err << "Flow::save_blueprint should emit v2 header\n";
      return false;
    }
    Flow restored{};
    if (!restored.load_blueprint(blueprint_or.value()).ok()) {
      if (err) *err << "Flow::load_blueprint should rebuild transform graph\n";
      return false;
    }
    if (restored.stage_count() != 1 || restored.transform_specs().front().stage_key != "cat") {
      if (err) *err << "Flow::load_blueprint should preserve stage key and count\n";
      return false;
    }
    auto max_cat_restored_or = restored.nested_param("cat::opt.max_categories");
    if (!max_cat_restored_or.ok() || max_cat_restored_or.value() != 3.0) {
      if (err) *err << "Flow::load_blueprint should preserve serialized transform options\n";
      return false;
    }
  }

  {
    Flow flow{};
    const ModelSpec spec{
        .task = TaskKind::Classification,
        .family = ModelFamily::Linear,
        .options = LinearFamilyOptions{.objective = LinearObjective::Logistic, .regularization_strength = 1e-3},
    };
    if (!flow.set_model(spec).ok()) {
      if (err) *err << "Flow::set_model should configure model nested params\n";
      return false;
    }
    auto reg_or = flow.nested_param("model::opt.regularization_strength");
    if (!reg_or.ok() || std::abs(reg_or.value() - 1e-3) > 1e-12) {
      if (err) *err << "Flow::nested_param should expose model option parameters\n";
      return false;
    }
    if (!flow.set_nested_param("model::opt.regularization_strength", 2e-3).ok()) {
      if (err) *err << "Flow::set_nested_param should update model option parameters\n";
      return false;
    }
    auto reg2_or = flow.nested_param("model::opt.regularization_strength");
    if (!reg2_or.ok() || std::abs(reg2_or.value() - 2e-3) > 1e-12) {
      if (err) *err << "Flow model option update should be queryable\n";
      return false;
    }
    const auto keys = flow.nested_param_keys();
    if (std::find(keys.begin(), keys.end(), "model::opt.regularization_strength") == keys.end()) {
      if (err) *err << "Flow::nested_param_keys should include model option keys\n";
      return false;
    }
  }

  {
    Flow flow{};
    if (!flow.add_neighbor_imputer(prep::NeighborImputerOptions{.neighbors = 1}).ok()
        || !flow.add_power_map(prep::PowerMapOptions{.mode = prep::PowerMapMode::YeoJohnson, .standardize = false, .lambda = 1.0}).ok()
        || !flow.add_quantile_map(prep::QuantileMapOptions{.output = prep::QuantileMapOutput::Uniform}).ok()
        || !flow.add_vector_normalizer(prep::VectorNormalizerOptions{.norm = prep::VectorNorm::L2}).ok()
        || !flow.add_polynomial_lift(prep::PolynomialLiftOptions{.degree = 2, .include_bias = false}).ok()
        || !flow.add_maxabs_scaler().ok()
        || !flow.add_threshold_binarizer(prep::ThresholdBinarizerOptions{.threshold = 0.5, .low_value = 0.0, .high_value = 1.0}).ok()
        || !flow.add_ordinal_encoder(prep::OrdinalEncoderOptions{.include_unknown_bucket = true, .one_based = true},
                                     std::vector<std::int64_t>{1})
                .ok()
        || !flow.add_label_encoder(prep::LabelEncoderOptions{.include_unknown_bucket = true, .one_based = false},
                                   std::vector<std::int64_t>{1})
                .ok()) {
      if (err) *err << "Flow should accept all new prep stage builders\n";
      return false;
    }

    const FeatureBatch features{
        .x = pluss::make_series(std::vector<double>{
                                    1.0, std::numeric_limits<double>::quiet_NaN(),
                                    2.0, 2.0,
                                    3.0, 3.0,
                                    4.0, 4.0},
                                {4, 2},
                                f64()),
        .layout = ObservationLayout::Rows,
    };
    auto out_or = flow.learn_map(features);
    if (!out_or.ok()) {
      if (err) *err << "Flow learn_map should succeed with new prep stack\n";
      return false;
    }
    if (!finite_series(out_or.value())) {
      if (err) *err << "Flow outputs from new prep stack should be finite\n";
      return false;
    }
  }

  {
    const Series x = pluss::make_series(std::vector<double>{
                                            -3.0, -1.0,
                                            -2.0, -1.0,
                                             std::numeric_limits<double>::quiet_NaN(), -0.8,
                                            -1.0, -0.6,
                                             1.0,  0.6,
                                             2.0,  0.8,
                                             3.0,  1.0,
                                             std::numeric_limits<double>::quiet_NaN(),  0.7},
                                        {8, 2},
                                        f64());
    const Series y = pluss::make_series(std::vector<double>{0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0}, {8}, f64());
    const LabeledBatch train{
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
      if (err) *err << "make_flow(classifier) should succeed\n";
      return false;
    }
    Flow flow = flow_or.move_value();
    if (!flow.add_simple_imputer(prep::SimpleImputerOptions{.strategy = prep::SimpleImputerStrategy::Mean}).ok()) {
      if (err) *err << "Flow::add_simple_imputer should succeed\n";
      return false;
    }
    if (!flow.add_standard_scaler().ok()) {
      if (err) *err << "Flow::add_standard_scaler should succeed\n";
      return false;
    }

    auto infer_before_or = flow.infer(train.features);
    if (infer_before_or.ok() || infer_before_or.status().code != StatusCode::Precondition) {
      if (err) *err << "Flow::infer should require learn\n";
      return false;
    }

    TrainOptions train_options{};
    train_options.max_iterations = 3000;
    train_options.tolerance = 1e-8;
    if (!flow.learn(train, train_options).ok()) {
      if (err) *err << "Flow::learn classification should succeed\n";
      return false;
    }

    auto pred_or = flow.infer(train.features);
    auto score_or = flow.infer_scores(train.features);
    auto metric_or = flow.assess(train, AssessOptions{.metrics = {MetricKind::Accuracy, MetricKind::LogLoss}});
    if (!pred_or.ok() || !score_or.ok() || !metric_or.ok()) {
      if (err) *err << "Flow classification infer/score/assess should succeed\n";
      return false;
    }
    const auto metric = as_f64(metric_or.value());
    if (metric.size() != 2 || metric[0] < 0.75) {
      if (err) *err << "Flow classification should reach useful accuracy\n";
      return false;
    }
    if (!finite_series(score_or.value()) || !finite_series(metric_or.value())) {
      if (err) *err << "Flow classification outputs should be finite\n";
      return false;
    }

    auto clone_or = flow.clone();
    if (!clone_or.ok()) {
      if (err) *err << "Flow::clone should succeed for trained serializable classification flows\n";
      return false;
    }
    Flow cloned = clone_or.move_value();
    if (!cloned.trained() || cloned.stage_count() != flow.stage_count() || cloned.task_kind() != flow.task_kind()) {
      if (err) *err << "Flow::clone should preserve trained state, stage count, and task kind\n";
      return false;
    }
    auto cloned_pred_or = cloned.infer(train.features);
    if (!cloned_pred_or.ok()) {
      if (err) *err << "Flow::clone inferred predictions should be available without retraining\n";
      return false;
    }
    const auto original_pred = as_f64(pred_or.value());
    const auto cloned_pred = as_f64(cloned_pred_or.value());
    if (original_pred.size() != cloned_pred.size()) {
      if (err) *err << "Flow::clone predictions should preserve row count\n";
      return false;
    }
    for (std::size_t i = 0; i < original_pred.size(); ++i) {
      if (std::abs(original_pred[i] - cloned_pred[i]) > 1e-9) {
        if (err) *err << "Flow::clone prediction " << i << " diverged from original\n";
        return false;
      }
    }
    if (!cloned.set_stage_key(0, "clone_only_imputer").ok()) {
      if (err) *err << "Flow::clone should allow metadata mutation on the clone\n";
      return false;
    }
    if (!flow.transform_specs().front().stage_key.empty()) {
      if (err) *err << "Flow::clone mutation should not affect original stage metadata\n";
      return false;
    }
  }

  {
    const Series x = pluss::make_series(std::vector<double>{
                                            -2.0,  1.0,
                                            -1.0,  std::numeric_limits<double>::quiet_NaN(),
                                             0.0,  0.0,
                                             1.0, -1.0,
                                             2.0, -2.0,
                                             3.0, -3.0},
                                        {6, 2},
                                        f64());
    const Series y = pluss::make_series(std::vector<double>{-4.0, -1.0, 1.0, 4.0, 7.0, 10.0}, {6}, f64());
    const LabeledBatch train{
        .features = FeatureBatch{.x = x, .layout = ObservationLayout::Rows},
        .y = y,
    };

    ModelSpec spec{
        .task = TaskKind::Regression,
        .family = ModelFamily::Linear,
        .options = LinearFamilyOptions{
            .objective = LinearObjective::LeastSquares,
            .regularization = RegularizationKind::L2,
            .regularization_strength = 1e-8,
            .fit_intercept = true,
        },
    };

    auto flow_or = make_flow(spec);
    if (!flow_or.ok()) {
      if (err) *err << "make_flow(regressor) should succeed\n";
      return false;
    }
    Flow flow = flow_or.move_value();
    if (!flow.add_simple_imputer(prep::SimpleImputerOptions{.strategy = prep::SimpleImputerStrategy::Mean}).ok()) {
      if (err) *err << "Flow::add_simple_imputer should succeed for regression\n";
      return false;
    }
    if (!flow.add_standard_scaler().ok()) {
      if (err) *err << "Flow::add_standard_scaler should succeed for regression\n";
      return false;
    }

    if (!flow.learn(train).ok()) {
      if (err) *err << "Flow::learn regression should succeed\n";
      return false;
    }

    auto pred_or = flow.infer(train.features);
    auto metrics_or = flow.assess(train, AssessOptions{.metrics = {MetricKind::Mse, MetricKind::QuantileLoss}, .regression_quantile = 0.8});
    if (!pred_or.ok() || !metrics_or.ok()) {
      if (err) *err << "Flow regression infer/assess should succeed\n";
      return false;
    }
    const auto metrics = as_f64(metrics_or.value());
    if (metrics.size() != 2 || metrics[0] > 0.6 || metrics[1] > 0.6) {
      if (err) *err << "Flow regression should produce low training losses\n";
      return false;
    }
    auto scores_or = flow.infer_scores(train.features);
    if (scores_or.ok() || scores_or.status().code != StatusCode::InvalidArgument) {
      if (err) *err << "Flow::infer_scores should reject regression flows\n";
      return false;
    }
  }

  {
    const DataFrame frame = make_mixed_flow_frame();
    DataFrameBatchOptions data_options{};
    data_options.feature_names = {"x0", "x1", "category", "ts", "target"};
    data_options.label_feature = "target";
    data_options.note_policy = DataNotePolicy::Hash;
    data_options.slate_policy = DataSlatePolicy::CastTemporalToSeries;

    ModelSpec spec{
        .task = TaskKind::Regression,
        .family = ModelFamily::Linear,
        .options = LinearFamilyOptions{
            .objective = LinearObjective::LeastSquares,
            .regularization = RegularizationKind::L2,
            .regularization_strength = 1e-8,
            .fit_intercept = true,
        },
    };
    auto flow_or = make_flow(spec);
    if (!flow_or.ok()) {
      if (err) *err << "make_flow(regression) should succeed for dataframe overloads\n";
      return false;
    }
    Flow flow = flow_or.move_value();
    if (!flow.learn(frame, data_options).ok()) {
      if (err) *err << "Flow::learn(DataFrame) should succeed for mixed-schema frame\n";
      return false;
    }

    auto pred_or = flow.infer(frame, data_options);
    auto metrics_or = flow.assess(frame, data_options, AssessOptions{.metrics = {MetricKind::Mse, MetricKind::R2}});
    if (!pred_or.ok() || !metrics_or.ok()) {
      if (err) *err << "Flow dataframe infer/assess overloads should succeed\n";
      return false;
    }
    if (!finite_series(pred_or.value()) || !finite_series(metrics_or.value())) {
      if (err) *err << "Flow dataframe overload outputs should be finite\n";
      return false;
    }
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("nodej flow compose lifecycle") {
  REQUIRE(run_nodej_flow_checks(&std::cerr));
}
#else
int main() {
  if (!run_nodej_flow_checks(&std::cerr)) {
    return 1;
  }
  std::cout << "nodej_flow_test passed\n";
  return 0;
}
#endif
