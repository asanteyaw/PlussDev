// Generic (non-NLP) supervised-learning coverage for ice::Model/ice::Outline,
// deliberately mirroring examples/NodeK/nn_schedule_bank_default_minimal_demo.cpp's
// shape (Fusion/ReLU/Fusion, Adam, BCEWithLogits) but via ice's one-liner API,
// on synthetic tabular data so the test has no external file dependency.
#include <pluss/ai/ice.h>
#include <pluss/mic.h>
#include <pluss/nn.h>
#include <pluss/pluss.h>

#include <filesystem>
#include <iostream>
#include <vector>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;
using pluss::table::DataFrame;
using pluss::table::Thread;
namespace ice = pluss::ai::ice;

Config float_cfg() {
  return Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
}

// A linearly-separable synthetic classification task: y = 1 iff a fixed,
// known weight vector dotted with x (plus noise) is positive. Any
// reasonable classifier should recover this cleanly.
struct ClassificationData {
  Series x;
  Series y;
};

ClassificationData make_classification_data(std::int64_t rows, std::uint64_t seed) {
  const auto cfg = float_cfg();
  const Series x = pluss::randn({rows, 4}, cfg, seed);
  const Series true_weight = pluss::make_series(
      std::vector<float>{2.0F, -3.0F, 1.0F, 0.5F}, std::vector<std::int64_t>{4, 1}, cfg);
  const Series noise = pluss::randn({rows, 1}, cfg, seed + 1).mul(0.1);
  const Series y = x.matmul(true_weight).add(noise).gt(0.0).to(cfg.platform, cfg.dtype);
  return ClassificationData{x, y};
}

bool run_ice_outline_classification_checks(std::ostream* err = nullptr) {
  const auto data = make_classification_data(600, 42);

  auto model = ice::Outline(pluss::nn::Fusion(4, 16), pluss::nn::ReLU(), pluss::nn::Fusion(16, 1));

  if (model.ready()) {
    if (err) *err << "ice Outline should not report ready before learn()\n";
    return false;
  }

  ice::TrainOptions options{};
  options.loss = ice::LossKind::BCEWithLogits;
  options.optimizer = ice::OptimizerKind::Adam;
  options.max_epochs = 150;
  options.batch_size = 64;
  options.learning_rate = 0.05;
  options.val_ratio = 0.15;
  options.seed = 7;

  model.learn(data.x, data.y, options);
  if (!model.ready()) {
    if (err) *err << "ice Outline should report ready after learn()\n";
    return false;
  }

  const auto report = model.assess(data.x, data.y, ice::LossKind::BCEWithLogits);
  if (report.metric_name != "accuracy" || report.metric < 0.9) {
    if (err) {
      *err << "ice Outline should recover a clean linearly-separable pattern; got metric="
           << report.metric_name << "=" << report.metric << " loss=" << report.loss << "\n";
    }
    return false;
  }

  // infer() should agree with what assess() computed internally.
  const auto predictions = model.infer(data.x);
  if (predictions.sizes()[0] != data.x.sizes()[0]) {
    if (err) *err << "ice Outline infer() should return one row per input row\n";
    return false;
  }

  // Chunked inference must agree with unchunked inference (a small
  // tolerance, not exact equality: different batch sizes can take
  // different BLAS summation-order paths for the same matmul).
  const auto chunked_predictions = model.infer(data.x, /*chunk_size=*/97);
  const auto flat_a = predictions.to(pluss::pCPU, ScalarType::Float64).reshape({predictions.numel()}).to_vec<double>();
  const auto flat_b =
      chunked_predictions.to(pluss::pCPU, ScalarType::Float64).reshape({chunked_predictions.numel()}).to_vec<double>();
  if (flat_a.size() != flat_b.size()) {
    if (err) *err << "ice Outline chunked infer() should return the same number of rows as unchunked infer()\n";
    return false;
  }
  for (std::size_t i = 0; i < flat_a.size(); ++i) {
    if (std::abs(flat_a[i] - flat_b[i]) > 1e-4) {
      if (err) {
        *err << "ice Outline chunked infer() should closely match unchunked infer(); row " << i
             << " differs: " << flat_a[i] << " vs " << flat_b[i] << "\n";
      }
      return false;
    }
  }

  // Save/load round trip must preserve the trained classifier exactly.
  const auto tmp = std::filesystem::temp_directory_path() / "ice_outline_classification_roundtrip.bin";
  model.save(tmp.string());

  auto restored = ice::Outline(pluss::nn::Fusion(4, 16), pluss::nn::ReLU(), pluss::nn::Fusion(16, 1));
  restored.load(tmp.string());
  std::filesystem::remove(tmp);

  const auto restored_report = restored.assess(data.x, data.y, ice::LossKind::BCEWithLogits);
  if (std::abs(restored_report.metric - report.metric) > 1e-9 ||
      std::abs(restored_report.loss - report.loss) > 1e-6) {
    if (err) {
      *err << "ice Outline save/load should preserve the trained model exactly; before metric="
           << report.metric << " after=" << restored_report.metric << "\n";
    }
    return false;
  }

  return true;
}

// A regression task via the same Outline type with MSE instead, proving
// genericity across loss kinds, not just across architectures.
bool run_ice_outline_regression_checks(std::ostream* err = nullptr) {
  const auto cfg = float_cfg();
  const std::int64_t rows = 400;
  const Series x = pluss::randn({rows, 3}, cfg, 100);
  const Series true_weight = pluss::make_series(std::vector<float>{1.5F, -2.0F, 0.5F},
                                                std::vector<std::int64_t>{3, 1}, cfg);
  const Series noise = pluss::randn({rows, 1}, cfg, 101).mul(0.05);
  const Series y = x.matmul(true_weight).add(noise);

  auto model = ice::Outline(pluss::nn::Fusion(3, 12), pluss::nn::ReLU(), pluss::nn::Fusion(12, 1));
  ice::TrainOptions options{};
  options.loss = ice::LossKind::MSE;
  options.max_epochs = 150;
  options.batch_size = 64;
  options.learning_rate = 0.02;
  options.val_ratio = 0.15;
  options.seed = 11;
  model.learn(x, y, options);

  const auto report = model.assess(x, y, ice::LossKind::MSE);
  if (report.metric_name != "rmse" || report.metric > 0.5) {
    if (err) {
      *err << "ice Outline should fit a clean linear-with-noise regression target closely; got rmse="
           << report.metric << "\n";
    }
    return false;
  }
  return true;
}

// The escape hatch: a hand-written ice::Model subclass with explicit layer
// chaining (not going through ice::Outline/pluss::nn::Schedule at all),
// proving the base class works for architectures defined by hand.
class TinyCustomClassifier final : public ice::Model {
public:
  TinyCustomClassifier() {
    register_module(*fusion1_.get());
    register_module(*relu_.get());
    register_module(*fusion2_.get());
  }

  [[nodiscard]] Series forward(const Series& x) const override {
    return fusion2_->forward(relu_->forward(fusion1_->forward(x)));
  }

private:
  pluss::nn::Fusion fusion1_{4, 8};
  pluss::nn::ReLU relu_{};
  pluss::nn::Fusion fusion2_{8, 1};
};

bool run_ice_custom_model_checks(std::ostream* err = nullptr) {
  const auto data = make_classification_data(300, 55);
  TinyCustomClassifier model{};

  ice::TrainOptions options{};
  options.loss = ice::LossKind::BCEWithLogits;
  options.max_epochs = 100;
  options.batch_size = 64;
  options.learning_rate = 0.05;
  options.val_ratio = 0.0; // exercise the no-early-stopping path too
  options.early_stopping = false;
  options.seed = 3;
  model.learn(data.x, data.y, options);

  if (!model.ready()) {
    if (err) *err << "ice custom Model subclass should report ready after learn()\n";
    return false;
  }
  const auto report = model.assess(data.x, data.y, ice::LossKind::BCEWithLogits);
  if (report.metric < 0.7) {
    if (err) *err << "ice custom Model subclass should learn something real; got accuracy=" << report.metric << "\n";
    return false;
  }
  return true;
}

// Numerical safety must be on by default: a deliberately absurd learning
// rate should fail loudly, not silently hand back a broken model.
bool run_ice_numerical_safety_checks(std::ostream* err = nullptr) {
  const auto data = make_classification_data(200, 66);
  auto model = ice::Outline(pluss::nn::Fusion(4, 32), pluss::nn::ReLU(), pluss::nn::Fusion(32, 1));

  ice::TrainOptions options{};
  options.loss = ice::LossKind::MSE;
  // Plain SGD, unlike Adam, has no per-parameter adaptive scaling to
  // dampen an absurd learning rate — Adam's normalization makes it
  // surprisingly robust to a huge nominal LR, so SGD is what reliably
  // reproduces a genuine blow-up here.
  options.optimizer = ice::OptimizerKind::SGD;
  options.max_epochs = 20;
  options.batch_size = 32;
  options.learning_rate = 1.0e8; // deliberately absurd
  // Gradient clipping is on by default specifically to neutralize scenarios
  // like this one — disable it here so this test isolates the *other*
  // safety net (fail_if_non_finite_*) instead of re-proving clipping works.
  options.grad_clip.mode = pluss::nn::optim::GradientClipMode::None;
  options.val_ratio = 0.0;
  options.early_stopping = false;
  options.seed = 1;

  bool threw = false;
  try {
    model.learn(data.x, data.y, options);
  } catch (const std::exception&) {
    threw = true;
  }
  if (!threw) {
    if (err) *err << "ice Model::learn should fail loudly on non-finite loss/gradients, not silently\n";
    return false;
  }
  return true;
}

// TrainPlatform::GPU on a machine with no GPU must warn and fall back to
// CPU, not throw — resolve_platform()'s job is to make GPU opt-in safe to
// request unconditionally. Also exercises TrainPlatform::Auto (same
// fallback, silent) and confirms infer()/assess() still work against a
// model that went through platform resolution.
bool run_ice_platform_fallback_checks(std::ostream* err = nullptr) {
  const auto data = make_classification_data(200, 21);

  for (const auto platform : {ice::TrainPlatform::GPU, ice::TrainPlatform::Auto}) {
    auto model = ice::Outline(pluss::nn::Fusion(4, 16), pluss::nn::ReLU(), pluss::nn::Fusion(16, 1));

    ice::TrainOptions options{};
    options.loss = ice::LossKind::BCEWithLogits;
    options.max_epochs = 30;
    options.batch_size = 64;
    options.learning_rate = 0.05;
    options.val_ratio = 0.15;
    options.seed = 5;
    options.platform = platform;

    try {
      model.learn(data.x, data.y, options);
    } catch (const std::exception& e) {
      if (err) *err << "ice Model::learn with a GPU-preferring TrainPlatform should fall back to CPU "
                       "rather than throw when no GPU is present; got: " << e.what() << "\n";
      return false;
    }
    if (!model.ready()) {
      if (err) *err << "ice Model should report ready() after a platform-fallback learn()\n";
      return false;
    }

    const auto report = model.assess(data.x, data.y, ice::LossKind::BCEWithLogits);
    if (report.metric_name != "accuracy") {
      if (err) *err << "ice Model::assess should still work after platform resolution\n";
      return false;
    }
    const Series predictions = model.infer(data.x);
    if (predictions.sizes()[0] != data.x.sizes()[0]) {
      if (err) *err << "ice Model::infer should still work after platform resolution\n";
      return false;
    }
  }
  return true;
}

// learn(DataFrame, target, standardize=true) must fit standardization stats
// that (a) the DataFrame overloads of infer()/assess() reapply automatically
// -- matching a manual (x - mean) / std done by hand against the same
// batch, not just "producing some prediction" -- and (b) survive a
// save()/load() round trip, so a freshly reloaded model doesn't need the
// caller to recompute anything either.
bool run_ice_dataframe_standardize_checks(std::ostream* err = nullptr) {
  const auto cfg = float_cfg();
  const std::int64_t rows = 500;
  // Deliberately very different raw scales (x1 ~1000x x0) so a caller who
  // skipped standardization would visibly struggle -- this is what
  // standardize=true exists to neutralize.
  const Series x0 = pluss::randn({rows}, cfg, 201ULL);
  const Series x1 = pluss::randn({rows}, cfg, 202ULL).mul(1000.0);
  const Series label = x0.add(x1.div(1000.0)).gt(0.0).to(ScalarType::Float32);

  const DataFrame df({
      {"x0", Thread(x0)},
      {"x1", Thread(x1)},
      {"label", Thread(label)},
  });

  auto model = ice::Outline(pluss::nn::Fusion(2, 16), pluss::nn::ReLU(), pluss::nn::Fusion(16, 1));
  ice::TrainOptions options{};
  options.loss = ice::LossKind::BCEWithLogits;
  options.optimizer = ice::OptimizerKind::Adam;
  options.max_epochs = 100;
  options.batch_size = 64;
  options.learning_rate = 0.05;
  options.standardize = true;
  options.val_ratio = 0.15;
  options.seed = 9;
  model.learn(df, "label", options);

  const auto report = model.assess(df, "label", ice::LossKind::BCEWithLogits);
  if (report.metric_name != "accuracy" || report.metric < 0.8) {
    if (err) {
      *err << "ice Model DataFrame standardize=true should learn a clean separable pattern; got accuracy="
           << report.metric << "\n";
    }
    return false;
  }

  // assess(df, ...) must exactly match a manual (x - mean) / std done by
  // hand against the same batch -- proves the DataFrame overload is really
  // reapplying the fitted stats, not silently skipping standardization.
  const std::vector<std::string_view> feature_names{"x0", "x1"};
  const Series x_matrix = df.features(pluss::msharp::RangeView::Of<std::string_view>(feature_names))
                               .matrix()
                               .as_series()
                               .to(pluss::pCPU, ScalarType::Float32);
  const Series col_mean = x_matrix.mean(0);
  const Series centered = x_matrix.sub(col_mean);
  const Series col_std = centered.mul(centered).mean(0).clamp_min(1e-12).sqrt();
  const Series x_standardized = centered.div(col_std);
  const Series label_col = label.reshape({rows, 1});

  const auto manual_report = model.assess(x_standardized, label_col, ice::LossKind::BCEWithLogits);
  if (std::abs(manual_report.loss - report.loss) > 1e-5) {
    if (err) {
      *err << "ice Model assess(DataFrame, ...) should exactly match assess() against a manually "
              "standardized batch; df loss=" << report.loss << " manual loss=" << manual_report.loss << "\n";
    }
    return false;
  }

  // infer(df, ...) must agree with infer() against that same manually
  // standardized batch.
  const Series df_predictions = model.infer(df, "label");
  const Series manual_predictions = model.infer(x_standardized);
  const double infer_diff = df_predictions.sub(manual_predictions).abs().max().item<double>();
  if (infer_diff > 1e-5) {
    if (err) *err << "ice Model infer(DataFrame, ...) should match infer() against a manually standardized batch\n";
    return false;
  }

  // Standardization stats must survive save()/load(), not just the trained
  // weights -- a freshly reloaded model should reproduce the same
  // assess(df, ...) result without the caller redoing anything.
  const auto tmp = std::filesystem::temp_directory_path() / "ice_dataframe_standardize_roundtrip.bin";
  model.save(tmp.string());

  auto restored = ice::Outline(pluss::nn::Fusion(2, 16), pluss::nn::ReLU(), pluss::nn::Fusion(16, 1));
  restored.load(tmp.string());
  std::filesystem::remove(tmp);
  std::filesystem::remove(std::filesystem::path(tmp.string() + ".standardize"));

  const auto restored_report = restored.assess(df, "label", ice::LossKind::BCEWithLogits);
  if (std::abs(restored_report.loss - report.loss) > 1e-3 ||
      std::abs(restored_report.metric - report.metric) > 1e-6) {
    if (err) {
      *err << "ice Model save/load should preserve standardization stats; before loss=" << report.loss
           << " after=" << restored_report.loss << "\n";
    }
    return false;
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("ice model tranche", "[ice][model]") {
  if (!run_ice_outline_classification_checks(&std::cerr)) FAIL("ice Outline classification checks failed");
  if (!run_ice_outline_regression_checks(&std::cerr)) FAIL("ice Outline regression checks failed");
  if (!run_ice_custom_model_checks(&std::cerr)) FAIL("ice custom Model checks failed");
  if (!run_ice_numerical_safety_checks(&std::cerr)) FAIL("ice numerical safety checks failed");
  if (!run_ice_platform_fallback_checks(&std::cerr)) FAIL("ice platform fallback checks failed");
  if (!run_ice_dataframe_standardize_checks(&std::cerr)) FAIL("ice DataFrame standardize checks failed");
}
#else
int main() {
  bool ok = true;
  ok = run_ice_outline_classification_checks(&std::cerr) && ok;
  ok = run_ice_outline_regression_checks(&std::cerr) && ok;
  ok = run_ice_custom_model_checks(&std::cerr) && ok;
  ok = run_ice_numerical_safety_checks(&std::cerr) && ok;
  ok = run_ice_platform_fallback_checks(&std::cerr) && ok;
  ok = run_ice_dataframe_standardize_checks(&std::cerr) && ok;
  if (!ok) return 1;
  std::cout << "ice model checks passed\n";
  return 0;
}
#endif
