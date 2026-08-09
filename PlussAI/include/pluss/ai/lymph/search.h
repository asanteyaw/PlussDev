#pragma once

#include "pluss/ai/lymph/common.h"
#include "pluss/ai/lymph/model.h"
#include "pluss/ai/lymph/selection.h"

#include <functional>
#include <numeric>
#include <optional>
#include <string>
#include <utility>
#include <vector>

// search.h -- Search / SearchResult (LYMPH_REDESIGN_PLAN.md phase 5).
//
// A reusable hyperparameter-search configuration wrapping today's old
// GridAxis/TuneOptions pair behind an object. The deliberate improvement:
// the trial factory closure builds a
// full Model (prep stages included), not a bare ModelSpec, so Search::run
// cross-validates a genuine end-to-end workflow path per trial instead of a raw,
// unscaled/un-one-hot-encoded model -- closing the scope cut documented in
// LYMPH_BLUEPRINT.md 8.2.
//
// Why this can't just call the existing spec-level search helpers: those go
// through NodeJ's own sweep/tune/probe engine, which only ever builds a bare
// ModelSpec per trial -- no notion of an attached Stage pipeline exists at
// that layer. Search::run's (trial x fold) dispatch is new, real lymph-level
// engineering (not a rename), built here mirroring the exact shape
// selection::compare_batch_ already uses for its own (candidate x fold)
// dispatch -- same pluss::aux::transmit flattening, same
// materialize_fold_batches_/cross_validation_report_from_folds_ helpers,
// same fit_top-controlled final-refit-on-the-whole-batch step -- just with
// each grid cell's Model built on demand via make_model(point) instead of
// pulled from a pre-built candidate list.
//
// Cost model: LYMPH_REDESIGN_PLAN.md 6.2 chose Option A (each trial
// independently rebuilds and refits its whole Model, prep included, from
// scratch, per fold) over Option B (hoist prep-fitting out of the trial loop
// into the fold loop) deliberately -- Option A is exactly the cost
// selection::compare already pays per candidate, so this isn't a new kind of
// expense, just one Search didn't previously pay because it never had prep
// stages to run. Option B is real restructuring (has to keep "prep-invariant
// across trials in a fold" and "hyperparameter-varying" pieces distinct
// while still respecting per-fold CV hygiene) and stays a documented,
// deliberate follow-up -- built only if profiling ever shows prep cost
// actually dominating trial cost on a real workload.
//
// GridAxis/GridPoint/grid_size/grid_point_at/grid_points/
// grid_sample_indices_ (selection.h, already built and tested) are reused
// as-is, still physically defined inside selection::selection -- re-exported
// unqualified into pluss::ai::lymph here via ordinary using-declarations
// (legal for namespace members, unlike the Type::static_method case
// model.h's header comment documents) so `lymph::GridAxis`/`lymph::
// GridPoint` read the way LYMPH_REDESIGN_PLAN.md 2's top-level type map
// describes them, with nothing physically duplicated or moved.
//
// SearchResult reuses selection::CompareReport/Trial/FitOptions the same
// way -- SearchResult itself lands at the top level per the plan's target
// shape, its .report field's *type* stays selection::CompareReport under
// the hood until select.h's own Comparison/SelectionResult promotion lands
// (not yet started), at which point the qualification just drops.
//
// Construction goes through lymph::search::grid(...)/search::random(...)
// or search::build(axes, TuneOptions) per the construction-vs-method rule
// (LYMPH_REDESIGN_PLAN.md 2.0). Split policy belongs to the Search object;
// callers can still pass an explicit SplitOptions to one .run(...) when they
// intentionally want to override the stored policy.
// Search's own constructor stays private, friended to the search factories
// only -- construction remains inside the `search::` family namespace, unlike
// Model where several genuinely different construction paths coexist.

namespace pluss::ai::lymph {

using selection::GridAxis;
using selection::GridPoint;

class Search;

namespace search {
[[nodiscard]] Search grid(std::vector<GridAxis> axes, SplitOptions split = {});
[[nodiscard]] Search grid(std::vector<GridAxis> axes, CrossValidationOptions cv);
[[nodiscard]] Search random(std::vector<GridAxis> axes,
                            std::int64_t trials,
                            RandomState random = {},
                            SplitOptions split = {});
[[nodiscard]] Search random(std::vector<GridAxis> axes,
                            std::int64_t trials,
                            CrossValidationOptions cv);
[[nodiscard]] Search random(std::vector<GridAxis> axes,
                            std::int64_t trials,
                            RandomState random,
                            CrossValidationOptions cv);
[[nodiscard]] Search build(std::vector<GridAxis> axes, TuneOptions options);
} // namespace search

// Returned by Search::run(...). .top(k) hands back cheap-copy Models, so the
// result does not have to outlive what the caller keeps.
struct SearchResult {
  selection::CompareReport report{};
  std::optional<Model> best_model{};
  std::vector<Model> fitted{};

  [[nodiscard]] const Model& best() const {
    if (!best_model.has_value()) {
      ::pluss::throw_error("Search::run: no best model has been computed.");
    }
    return *best_model;
  }
  [[nodiscard]] Model& best() {
    if (!best_model.has_value()) {
      ::pluss::throw_error("Search::run: no best model has been computed.");
    }
    return *best_model;
  }

  [[nodiscard]] std::vector<std::pair<std::string, Model>> top(std::int64_t k) const {
    if (k < 0) {
      ::pluss::throw_error("Search::run: top(k) requires k >= 0.");
    }
    if (k > static_cast<std::int64_t>(fitted.size())) {
      ::pluss::throw_error("Search::run: requested k exceeds fitted trial count; increase FitOptions.fit_top.");
    }
    std::vector<std::pair<std::string, Model>> out{};
    out.reserve(static_cast<std::size_t>(k));
    for (std::int64_t i = 0; i < k; ++i) {
      const std::int64_t trial_index = report.ranked_indices[static_cast<std::size_t>(i)];
      out.emplace_back(report.trials[static_cast<std::size_t>(trial_index)].name,
                       fitted[static_cast<std::size_t>(i)]);
    }
    return out;
  }
};

class Search final {
 public:
  // Direct LabeledBatch entry point -- see selection::compare's own
  // LabeledBatch overload for why this exists (GPU-resident Series data has
  // nowhere to go through DataFrame, which is deliberately CPU-only).
  [[nodiscard]] SearchResult run(const std::function<Model(const GridPoint&)>& make_model,
                                 const LabeledBatch& batch,
                                 const TrainOptions& train_options = {},
                                 const AssessOptions& assess_options = {},
                                 MetricKind objective_metric = MetricKind::Accuracy,
                                 TuneObjectivePolicy objective_policy = TuneObjectivePolicy::Auto,
                                 selection::FitOptions fit_options = {}) const {
    const SplitOptions split = default_split_for_(make_model);
    return run(make_model, batch, split, train_options, assess_options, objective_metric, objective_policy, fit_options);
  }

  [[nodiscard]] SearchResult run(const std::function<Model(const GridPoint&)>& make_model,
                                 const LabeledBatch& batch,
                                 const SplitOptions& split,
                                 const TrainOptions& train_options = {},
                                 const AssessOptions& assess_options = {},
                                 MetricKind objective_metric = MetricKind::Accuracy,
                                 TuneObjectivePolicy objective_policy = TuneObjectivePolicy::Auto,
                                 selection::FitOptions fit_options = {}) const;

  [[nodiscard]] SearchResult run(const std::function<Model(const GridPoint&)>& make_model,
                                 const pluss::table::DataFrame& x,
                                 const Series& y,
                                 const TrainOptions& train_options = {},
                                 const AssessOptions& assess_options = {},
                                 MetricKind objective_metric = MetricKind::Accuracy,
                                 TuneObjectivePolicy objective_policy = TuneObjectivePolicy::Auto,
                                 selection::FitOptions fit_options = {}) const {
    const SplitOptions split = default_split_for_(make_model);
    return run(make_model, x, y, split, train_options, assess_options, objective_metric, objective_policy, fit_options);
  }

  [[nodiscard]] SearchResult run(const std::function<Model(const GridPoint&)>& make_model,
                                 const pluss::table::DataFrame& x,
                                 const Series& y,
                                 const SplitOptions& split,
                                 const TrainOptions& train_options = {},
                                 const AssessOptions& assess_options = {},
                                 MetricKind objective_metric = MetricKind::Accuracy,
                                 TuneObjectivePolicy objective_policy = TuneObjectivePolicy::Auto,
                                 selection::FitOptions fit_options = {}) const {
    const auto names = cross_validate_default_feature_names_(x, "Search::run(x,y)");
    const LabeledBatch batch = cross_validate_labeled_from_xy_(x, y, names, "Search::run(x,y)");
    return run(make_model, batch, split, train_options, assess_options, objective_metric, objective_policy, fit_options);
  }

  [[nodiscard]] SearchResult run(const std::function<Model(const GridPoint&)>& make_model,
                                 const pluss::table::DataFrame& x,
                                 const pluss::table::Thread& y,
                                 const TrainOptions& train_options = {},
                                 const AssessOptions& assess_options = {},
                                 MetricKind objective_metric = MetricKind::Accuracy,
                                 TuneObjectivePolicy objective_policy = TuneObjectivePolicy::Auto,
                                 selection::FitOptions fit_options = {}) const {
    return run(make_model, x, as_stratify_series(y), train_options, assess_options,
               objective_metric, objective_policy, fit_options);
  }

  [[nodiscard]] SearchResult run(const std::function<Model(const GridPoint&)>& make_model,
                                 const pluss::table::DataFrame& x,
                                 const pluss::table::Thread& y,
                                 const SplitOptions& split,
                                 const TrainOptions& train_options = {},
                                 const AssessOptions& assess_options = {},
                                 MetricKind objective_metric = MetricKind::Accuracy,
                                 TuneObjectivePolicy objective_policy = TuneObjectivePolicy::Auto,
                                 selection::FitOptions fit_options = {}) const {
    return run(make_model, x, as_stratify_series(y), split, train_options, assess_options,
               objective_metric, objective_policy, fit_options);
  }

  // (frame, DataFrameBatchOptions) sibling -- see confusion_table's twin in
  // detail/model_core.h for why this exists alongside the (x, y) form.
  [[nodiscard]] SearchResult run(const std::function<Model(const GridPoint&)>& make_model,
                                 const pluss::table::DataFrame& frame,
                                 const DataFrameBatchOptions& data_options,
                                 const TrainOptions& train_options = {},
                                 const AssessOptions& assess_options = {},
                                 MetricKind objective_metric = MetricKind::Accuracy,
                                 TuneObjectivePolicy objective_policy = TuneObjectivePolicy::Auto,
                                 selection::FitOptions fit_options = {}) const {
    const SplitOptions split = default_split_for_(make_model);
    return run(make_model, frame, data_options, split, train_options, assess_options, objective_metric, objective_policy, fit_options);
  }

  [[nodiscard]] SearchResult run(const std::function<Model(const GridPoint&)>& make_model,
                                 const pluss::table::DataFrame& frame,
                                 const DataFrameBatchOptions& data_options,
                                 const SplitOptions& split,
                                 const TrainOptions& train_options = {},
                                 const AssessOptions& assess_options = {},
                                 MetricKind objective_metric = MetricKind::Accuracy,
                                 TuneObjectivePolicy objective_policy = TuneObjectivePolicy::Auto,
                                 selection::FitOptions fit_options = {}) const {
    const LabeledBatch batch = resolve(labeled_batch_from_dataframe(frame, data_options),
                                       "Search::run(frame,data_options).labeled_batch_from_dataframe");
    return run(make_model, batch, split, train_options, assess_options, objective_metric, objective_policy, fit_options);
  }

 private:
  friend Search search::grid(std::vector<GridAxis> axes, SplitOptions split);
  friend Search search::grid(std::vector<GridAxis> axes, CrossValidationOptions cv);
  friend Search search::random(std::vector<GridAxis> axes, std::int64_t trials, RandomState random, SplitOptions split);
  friend Search search::random(std::vector<GridAxis> axes, std::int64_t trials, CrossValidationOptions cv);
  friend Search search::random(std::vector<GridAxis> axes,
                               std::int64_t trials,
                               RandomState random,
                               CrossValidationOptions cv);
  friend Search search::build(std::vector<GridAxis> axes, TuneOptions options);

  Search(std::vector<GridAxis> axes, SearchKind kind, std::int64_t trials, RandomState random, SplitOptions split)
      : axes_(std::move(axes)), kind_(kind), trials_(trials), random_(random), split_(std::move(split)) {}

  Search(std::vector<GridAxis> axes, SearchKind kind, std::int64_t trials, RandomState random, CrossValidationOptions cv)
      : axes_(std::move(axes)),
        kind_(kind),
        trials_(trials),
        random_(random),
        split_(make_cross_validation_split_options(cv)),
        cv_(std::move(cv)) {}

  [[nodiscard]] SplitOptions default_split_for_(const std::function<Model(const GridPoint&)>& make_model) const;

  std::vector<GridAxis> axes_{};
  SearchKind kind_ = SearchKind::Grid;
  std::int64_t trials_ = 0;
  RandomState random_{};
  SplitOptions split_{};
  std::optional<CrossValidationOptions> cv_{};
};

[[nodiscard]] inline SplitOptions Search::default_split_for_(const std::function<Model(const GridPoint&)>& make_model) const {
  if (!cv_.has_value()) {
    return split_;
  }
  const std::int64_t total = selection::grid_size(axes_);
  if (total <= 0) {
    ::pluss::throw_error("Search::run: search space is empty (an axis has zero values).");
  }
  const TaskKind task = make_model(selection::grid_point_at(0, axes_)).task_kind();
  return detail::effective_workflow_split_options_(*cv_, task);
}

[[nodiscard]] inline SearchResult Search::run(const std::function<Model(const GridPoint&)>& make_model,
                                              const LabeledBatch& batch,
                                              const SplitOptions& split_options,
                                              const TrainOptions& train_options,
                                              const AssessOptions& assess_options_in,
                                              MetricKind objective_metric,
                                              TuneObjectivePolicy objective_policy,
                                              selection::FitOptions fit_options) const {
  const std::int64_t total = selection::grid_size(axes_);
  if (total <= 0) {
    ::pluss::throw_error("Search::run: search space is empty (an axis has zero values).");
  }
  if (fit_options.fit_top < 0) {
    ::pluss::throw_error("Search::run: FitOptions.fit_top must be non-negative.");
  }

  std::vector<std::int64_t> indices{};
  if (kind_ == SearchKind::Random) {
    if (trials_ <= 0) {
      ::pluss::throw_error("Search::run: SearchKind::Random requires trials > 0 (see search::random).");
    }
    indices = selection::grid_sample_indices_(total, trials_, random_);
  } else if (kind_ == SearchKind::None || kind_ == SearchKind::Grid) {
    indices.resize(static_cast<std::size_t>(total));
    std::iota(indices.begin(), indices.end(), std::int64_t{0});
  } else {
    ::pluss::throw_error(
        "Search::run: only SearchKind::Grid (or None) and SearchKind::Random are supported here -- "
        "call selection::probe_specs directly with a custom sampler for Bayesian/SuccessiveHalving.");
  }

  std::vector<GridPoint> points{};
  points.reserve(indices.size());
  for (const auto index : indices) points.push_back(selection::grid_point_at(index, axes_));

  AssessOptions assess_options = assess_options_in;
  if (assess_options.metrics.empty()) {
    assess_options.metrics = {objective_metric};
  } else if (std::find(assess_options.metrics.begin(), assess_options.metrics.end(), objective_metric) == assess_options.metrics.end()) {
    assess_options.metrics.push_back(objective_metric);
  }

  const bool minimize = selection::minimize_metric_(objective_metric, objective_policy);

  // A Search trial's Model doesn't exist until make_model(point) actually
  // runs -- unlike selection::compare_batch_, which reads task_kind() off
  // an already-built candidates.front(). One throwaway probe call here
  // reads it the same way, cheaply: building an untrained Model's
  // architecture (no .learn() involved) is not the expensive part of any
  // of this.
  const TaskKind task = make_model(points.front()).task_kind();

  const SplitPlan plan = detail::workflow_split_plan_(batch, split_options, task, "Search::run");
  const std::vector<FoldBatchPair> fold_batches = detail::materialize_fold_batches_(batch, plan, "Search::run");

  struct GridCell_ {
    std::size_t point_index;
    std::size_t fold_index;
  };
  const std::size_t point_n = points.size();
  const std::size_t fold_n = fold_batches.size();
  std::vector<GridCell_> cells{};
  cells.reserve(point_n * fold_n);
  for (std::size_t pi = 0; pi < point_n; ++pi) {
    for (std::size_t fi = 0; fi < fold_n; ++fi) {
      cells.push_back(GridCell_{pi, fi});
    }
  }

  const std::int64_t metric_count = static_cast<std::int64_t>(assess_options.metrics.size());
  const auto evaluate_cell = [&](const GridCell_& cell) -> FoldAssessResult {
    // Option A (see this file's header comment): every cell rebuilds its
    // own full Model from scratch, unconditionally -- no cleverness, no
    // special-casing any one point.
    Model trial_model = make_model(points[cell.point_index]);
    const auto& fold = plan.folds[cell.fold_index];
    const FoldBatchPair& fold_batch = fold_batches[cell.fold_index];
    const Model trained = trial_model.learn(fold_batch.train, train_options);
    const Series fold_metrics =
        trained.assess_series(fold_batch.test, assess_options).to(ScalarType::Float64).reshape({metric_count});
    return FoldAssessResult{
        .metrics = fold_metrics,
        .train_indices = fold.train_indices,
        .test_indices = fold.test_indices,
    };
  };

  pluss::aux::transmit_options topts{};
  topts.grain_size = 1;
  if (split_options.max_parallel == 1 || cells.size() <= 1) {
    topts.exec = pluss::aux::transmit_exec::seq;
  } else {
    topts.exec = pluss::aux::transmit_exec::par;
    topts.workers = split_options.max_parallel > 0 ? static_cast<std::size_t>(split_options.max_parallel) : 0;
  }
  const std::vector<FoldAssessResult> cell_results = pluss::aux::transmit(evaluate_cell, cells, topts);

  SearchResult result{};
  result.report.objective_metric = objective_metric;
  result.report.objective_policy = objective_policy;
  result.report.trials.reserve(point_n);

  std::int64_t best_index = -1;
  double best_objective = std::numeric_limits<double>::quiet_NaN();
  for (std::size_t pi = 0; pi < point_n; ++pi) {
    CrossAssessReport cross_report{};
    cross_report.metrics = assess_options.metrics;
    cross_report.folds.resize(fold_n);
    for (std::size_t fi = 0; fi < fold_n; ++fi) {
      cross_report.folds[fi] = cell_results[pi * fold_n + fi];
    }
    CrossValidationReport cv_report = detail::cross_validation_report_from_folds_(std::move(cross_report));
    const double objective = selection::scalar_metric_(cv_report.mean, objective_metric);
    result.report.trials.push_back(selection::Trial{
        .name = points[pi].label(),
        .objective = objective,
        .cv = std::move(cv_report),
    });

    if (best_index < 0) {
      best_index = static_cast<std::int64_t>(pi);
      best_objective = objective;
      continue;
    }
    const bool better = minimize ? (objective < best_objective) : (objective > best_objective);
    if (better) {
      best_index = static_cast<std::int64_t>(pi);
      best_objective = objective;
    }
  }

  result.report.best_index = best_index;
  result.report.best_name = result.report.trials[static_cast<std::size_t>(best_index)].name;
  selection::assign_objective_ranks_(result.report);
  if (result.report.ranked_indices.empty()) {
    ::pluss::throw_error("Search::run: no finite objective trial was available to fit.");
  }

  if (fit_options.fit_top > 0) {
    const std::int64_t fit_count =
        std::min<std::int64_t>(fit_options.fit_top, static_cast<std::int64_t>(result.report.ranked_indices.size()));
    result.fitted.reserve(static_cast<std::size_t>(fit_count));
    for (std::int64_t i = 0; i < fit_count; ++i) {
      const std::int64_t point_index = result.report.ranked_indices[static_cast<std::size_t>(i)];
      Model fitted_model = make_model(points[static_cast<std::size_t>(point_index)]).learn(batch, train_options);
      result.fitted.push_back(std::move(fitted_model));
    }
    // ranked_indices.front() == best_index (assign_objective_ranks_ sorts
    // with the same minimize/maximize comparator used to pick best_index
    // above, both breaking ties by keeping the earliest point), so
    // result.fitted[0] is already a trained copy of the winning point --
    // clone it (dump+reload trained state, not a retrain) instead of
    // training the winner a second time.
    result.best_model = result.fitted.front().clone();
    return result;
  }

  const GridPoint& best_point = points[static_cast<std::size_t>(best_index)];
  result.best_model = make_model(best_point).learn(batch, train_options);
  return result;
}

namespace search {

[[nodiscard]] inline Search grid(std::vector<GridAxis> axes, SplitOptions split) {
  return Search(std::move(axes), SearchKind::Grid, 0, RandomState{}, std::move(split));
}

[[nodiscard]] inline Search grid(std::vector<GridAxis> axes, CrossValidationOptions cv) {
  return Search(std::move(axes), SearchKind::Grid, 0, RandomState{}, std::move(cv));
}

[[nodiscard]] inline Search random(std::vector<GridAxis> axes,
                                   std::int64_t trials,
                                   RandomState random,
                                   SplitOptions split) {
  return Search(std::move(axes), SearchKind::Random, trials, random, std::move(split));
}

[[nodiscard]] inline Search random(std::vector<GridAxis> axes,
                                   std::int64_t trials,
                                   CrossValidationOptions cv) {
  return Search(std::move(axes), SearchKind::Random, trials,
                RandomState{.seed = cv.random_state, .deterministic = cv.deterministic},
                std::move(cv));
}

[[nodiscard]] inline Search random(std::vector<GridAxis> axes,
                                   std::int64_t trials,
                                   RandomState random,
                                   CrossValidationOptions cv) {
  return Search(std::move(axes), SearchKind::Random, trials, random, std::move(cv));
}

[[nodiscard]] inline Search build(std::vector<GridAxis> axes, TuneOptions options) {
  SplitOptions split = options.split;
  if (split.max_parallel <= 0 && options.max_parallel > 0) {
    split.max_parallel = options.max_parallel;
  }
  if (options.search == SearchKind::Random) {
    return random(std::move(axes), options.trials, options.random, std::move(split));
  }
  return Search(std::move(axes), options.search, options.trials, options.random, std::move(split));
}

} // namespace search

} // namespace pluss::ai::lymph
