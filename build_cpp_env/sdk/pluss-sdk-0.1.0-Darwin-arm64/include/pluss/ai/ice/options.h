#pragma once

#include "pluss/ai/ice/platform.h"
#include "pluss/nn.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

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

enum class MonitorDirection : std::uint8_t {
  Auto = 0,
  Min = 1,
  Max = 2,
};

enum class ProgressOutput : std::uint8_t {
  Stdout = 0,
  Stderr = 1,
};

struct ProgressPolicy {
  // Enables Coach's built-in epoch logger. A manually installed
  // EpochLoggerCallback writes regardless of this flag.
  bool verbose = false;
  std::int64_t log_every_n_epochs = 1;
  bool progress_bar = false;
  ProgressOutput output = ProgressOutput::Stdout;
  std::vector<std::string> metrics{"loss", "val_loss"};
  std::int64_t bar_width = 24;
  std::int64_t precision = 6;
};

struct ValidationPolicy {
  double split_ratio = 0.1;
  std::int64_t every_n_epochs = 1;
  std::int64_t start_from_epoch = 0;
  std::string monitor = "val_loss";
  MonitorDirection direction = MonitorDirection::Auto;
};

struct ArchitectureRegularization {
  double dropout = 0.0;
};

struct ObjectiveRegularization {
  double weight_decay = 0.0;
  bool decoupled_weight_decay = false;
  double l1_penalty = 0.0;
  double l2_penalty = 0.0;
  double label_smoothing = 0.0;
};

struct LoopRegularization {
  bool early_stopping = true;
  std::int64_t patience = 10;
  double min_delta = 1e-6;
  bool restore_best_weights = false;
};

struct CheckpointPolicy {
  bool enabled = false;
  bool save_last = false;
  bool save_best = false;
  bool save_weights_only = false;
  bool save_on_exception = false;
  std::int64_t every_n_epochs = 1;
  std::string dirpath{};
  std::string filename{};
  std::string resume_from{};
};

// Reuses pluss::nn::optim's own clip/diagnostics types directly rather than
// re-declaring parallel ice-native ones — same object model underneath,
// no translation cost if a caller drops to raw NodeK (ICE_BLUEPRINT.md §3.2).
struct ExecutionPolicy {
  pluss::nn::optim::GradientClipOptions grad_clip{
      .mode = pluss::nn::optim::GradientClipMode::ByNorm,
      .threshold = 1.0,
  };
  std::int64_t max_epochs = 60;
  std::int64_t accumulation_steps = 1;
  std::int64_t fast_dev_batches = 0;
  std::int64_t max_train_batches = 0;
  std::int64_t max_valid_batches = 0;
  bool fail_on_non_finite = true;
  bool deterministic = false;
  ScalarType precision = ScalarType::Float32;
  TrainPlatform platform = TrainPlatform::CPU;
};

// Central training configuration surface for Ice. This is the Phase 3
// substrate from ICE_Improvement_DESIGN.md: one object that holds optimizer,
// loss, validation cadence, loop regularization, checkpoint policy, and
// execution/runtime policy together instead of scattering them across ad hoc
// loop locals.
struct Recipe {
  std::int64_t batch_size = 32;
  double learning_rate = 1e-3;
  OptimizerKind optimizer = OptimizerKind::Adam;
  LossKind loss = LossKind::MSE;
  bool shuffle = true;
  std::uint64_t seed = 0;
  // Only consulted by the DataFrame overload: per-feature-column
  // standardization ((x - mean) / std) before training.
  bool standardize = false;
  ProgressPolicy progress{};
  ValidationPolicy validation{};
  ArchitectureRegularization architecture_regularization{};
  ObjectiveRegularization objective_regularization{};
  LoopRegularization loop_regularization{};
  CheckpointPolicy checkpoint{};
  ExecutionPolicy execution{};
};

using TrainOptions = Recipe;

struct AssessReport {
  double loss = 0.0;
  double metric = 0.0;
  std::string metric_name{};
};

} // namespace pluss::ai::ice
