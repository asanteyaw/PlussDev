#pragma once

#include "pluss/ai/ice/platform.h"
#include "pluss/nn.h"

#include <cstdint>
#include <string>

namespace pluss::ai::ice {

// Enum, not string: picking the wrong optimizer/loss is a compile error, not
// a runtime surprise discovered mid-training (see ICE_BLUEPRINT.md §3 — a
// deliberate tradeoff of a longer type name for compiler-checked correctness,
// with IDE completion easing the typing cost).
enum class OptimizerKind : std::uint8_t {
  Adam = 0,
  AdamW = 1,
  SGD = 2,
  RMSProp = 3,
  Adagrad = 4,
  NAdam = 5,
};

enum class LossKind : std::uint8_t {
  MSE = 0,
  BCEWithLogits = 1,
  CrossEntropy = 2,
};

// Reuses pluss::nn::optim's own clip/diagnostics types directly rather than
// re-declaring parallel ice-native ones — same object model underneath,
// no translation cost if a caller drops to raw NodeK (ICE_BLUEPRINT.md §3.2).
struct TrainOptions {
  std::int64_t max_epochs = 60;
  std::int64_t batch_size = 32;
  double learning_rate = 1e-3;
  OptimizerKind optimizer = OptimizerKind::Adam;
  LossKind loss = LossKind::MSE;
  pluss::nn::optim::GradientClipOptions grad_clip{
      .mode = pluss::nn::optim::GradientClipMode::ByNorm,
      .threshold = 1.0,
  };
  // Fails loudly (throws) the moment a loss/gradient/parameter goes
  // non-finite instead of silently continuing or degrading — on by default,
  // not opt-in (ICE_BLUEPRINT.md §3.5 and the fallback-behavior discussion
  // it followed from).
  bool fail_on_non_finite = true;
  bool shuffle = true;
  std::uint64_t seed = 0;

  // Only consulted by the two-array `learn(x, y, options)` overload: carves
  // an internal validation split for early stopping. Set to 0 to disable
  // (trains on every row, no early stopping possible from this overload).
  double val_ratio = 0.1;
  bool early_stopping = true;
  std::int64_t patience = 10;
  double min_delta = 1e-6;

  // Only consulted by the DataFrame overload: per-feature-column
  // standardization ((x - mean) / std) before training.
  bool standardize = false;

  // Which device to train on. Resolved once at the start of learn() via
  // resolve_platform(); the model's parameters and the training/validation
  // data are both moved there for you, regardless of what platform they
  // started on. See TrainPlatform for fallback behavior.
  TrainPlatform platform = TrainPlatform::CPU;
};

struct AssessReport {
  double loss = 0.0;
  double metric = 0.0;
  std::string metric_name{};
};

} // namespace pluss::ai::ice
