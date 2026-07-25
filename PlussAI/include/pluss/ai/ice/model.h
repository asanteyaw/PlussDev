#pragma once

#include "pluss/ai/ice/options.h"
#include "pluss/nn.h"

#include <string_view>
#include <vector>

namespace pluss::table {
class DataFrame;
} // namespace pluss::table

namespace pluss::ai::ice {

// The generic escape hatch (ICE_BLUEPRINT.md §4): define parameters plus
// forward/loss, get learn/infer/assess/save/load for free. `ice::Outline`
// is the common-case convenience built on top of this for models that are
// just a chain of existing layers; `Model` is for anything else.
//
// Deliberately generic: nothing here is text-, image-, or task-specific.
// `forward`/`compute_loss` operate on plain `Series` — any NodeK-backed
// model with a single input tensor and single output tensor fits.
class Model {
public:
  virtual ~Model() = default;

  [[nodiscard]] virtual Series forward(const Series& x) const = 0;

  // Default dispatches on `kind` to the matching pluss::nn::losses:: free
  // function. Override for models that need a custom objective.
  [[nodiscard]] virtual Series compute_loss(const Series& predictions, const Series& y, LossKind kind) const;

  // Trains on `x_train`/`y_train`. When `options.val_ratio > 0`, an internal
  // validation split is carved out for early stopping (see TrainOptions).
  void learn(const Series& x_train, const Series& y_train, const TrainOptions& options = {});

  // Trains on an already-split train/val pair — for callers who manage
  // their own splitting (e.g. NodeB's per-task validation conventions).
  void learn(const Series& x_train, const Series& y_train,
            const Series& x_val, const Series& y_val,
            const TrainOptions& options = {});

  // Tabular convenience: pulls every column except `target` as features via
  // one batched DataFrame::matrix() call (no manual per-column plumbing),
  // optionally standardizes them, then delegates to the Series overload.
  // When `options.standardize` is set, the fitted per-column mean/std are
  // kept on the model (persisted through save()/load()) and reapplied
  // automatically by the DataFrame overloads of infer()/assess() below —
  // callers never need to recompute them.
  void learn(const pluss::table::DataFrame& df, std::string_view target, const TrainOptions& options = {});

  // Runs inference in eval mode. `chunk_size > 0` bounds memory for large
  // `x` by looping over row-chunks (each chunk is still one batched forward
  // call, not a per-row loop) and concatenating the results.
  [[nodiscard]] Series infer(const Series& x, std::int64_t chunk_size = 0) const;

  // Tabular convenience mirroring learn(DataFrame, ...): pulls every column
  // except `target` as a feature (empty by default — genuinely new data to
  // predict on typically has no target column at all; pass the same name
  // used in learn() when reusing that same DataFrame, e.g. right after
  // assess(), so its target/label column isn't mistaken for a feature).
  // Applies the same standardization learn() fit (a no-op if the model
  // wasn't trained with standardize=true), then delegates to the Series
  // overload.
  [[nodiscard]] Series infer(const pluss::table::DataFrame& df, std::string_view target = {},
                             std::int64_t chunk_size = 0) const;

  // One batched forward + loss call, plus a metric derived from `loss`
  // (accuracy for the classification losses, RMSE for MSE). `loss` is
  // required rather than remembered from the last `learn()` call on
  // purpose: a remembered value wouldn't survive save()/load() (the
  // artifact has no metadata channel for it — see ice::io), so it would be
  // silently wrong for a freshly loaded model. Pass the same LossKind used
  // to train.
  [[nodiscard]] AssessReport assess(const Series& x, const Series& y, LossKind loss) const;

  // Tabular convenience mirroring learn(DataFrame, ...): pulls every column
  // except `target` as features, applies whatever standardization learn()
  // fit (a no-op if the model wasn't trained with standardize=true), then
  // delegates to the Series overload.
  [[nodiscard]] AssessReport assess(const pluss::table::DataFrame& df, std::string_view target, LossKind loss) const;

  // Persists registered parameters, plus the fitted standardization stats
  // (as a small sidecar artifact) when the model was trained with
  // standardize=true.
  void save(std::string_view filepath) const;
  // Restores trained values into an already-constructed model with an
  // identical parameter list (same architecture/options as when saved) —
  // construct-then-load, the same convention as most deep learning
  // frameworks' state_dict loading, not a polymorphic factory. Also
  // restores standardization stats from the sidecar artifact if one exists
  // next to `filepath`.
  void load(std::string_view filepath);

  [[nodiscard]] bool ready() const noexcept { return trained_; }

  // Moves every registered parameter and module onto `platform` in place.
  // learn() calls this for you (see TrainOptions::platform); exposed
  // publicly as an escape hatch for callers who want to place a model
  // before/without going through learn() — e.g. to warm it up on a GPU
  // ahead of a save()/load() round trip.
  Model& to(Platform platform);

protected:
  // Called by subclasses (from their constructor) to register the raw
  // trainable Series and/or NodeK Modules that make up their parameters.
  // Both are non-owning references into the subclass's own members.
  void register_parameter(Series& param);
  void register_module(pluss::nn::Module& module);

private:
  std::vector<Series*> owned_parameters_{};
  std::vector<pluss::nn::Module*> owned_modules_{};
  bool trained_ = false;

  // Fitted by learn(DataFrame, ..., standardize=true); applied by the
  // DataFrame overloads of infer()/assess() and reloaded by load(). Left
  // empty/unused (standardized_ stays false) for models that never go
  // through the DataFrame overload, or that do with standardize=false.
  Series standardize_mean_ = Series::empty({0}, ScalarType::Float32, pluss::pCPU);
  Series standardize_std_ = Series::empty({0}, ScalarType::Float32, pluss::pCPU);
  bool standardized_ = false;

  [[nodiscard]] std::vector<Series*> all_parameters_() const;
  [[nodiscard]] Series dataframe_matrix_(const pluss::table::DataFrame& df, std::string_view exclude) const;
  [[nodiscard]] Series fit_standardize_(const Series& x);
  [[nodiscard]] Series apply_standardize_(const Series& x) const;
  void train_loop_(const Series& x_train, const Series& y_train,
                   const Series* x_val, const Series* y_val,
                   const TrainOptions& options);
};

} // namespace pluss::ai::ice
