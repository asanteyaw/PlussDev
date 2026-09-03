#pragma once

#include "pluss/ai/lymph/common.h"
#include "pluss/ai/lymph/model.h"
#include "pluss/ai/lymph/selection.h"
#include "pluss/msharp/msharp.h"

#include <initializer_list>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

// builder.h -- Builder (LYMPH_REDESIGN_PLAN.md; Nexus-inspired model
// coordinator, requested alongside the Comparison->Benchmark rename).
//
// A named collection of in-memory Models, mirroring the role
// pluss::table::Nexus plays for DataFrames (PlussMic/src/table/Nexus.h) as
// closely as Model's own constraints allow -- .names()/.has()/.get() mirror
// Nexus::keys()/.has()/.get()/.frame(), .list(n)/.report(n) mirror the
// same "cheap listing vs. rich DataFrame" split Nexus draws between
// .keys()/.names() and .summary()/.profile() (named .report(), not
// .describe(), since DataFrame already has its own unrelated .describe()
// for summary statistics -- same name, different meaning, worth avoiding).
//
// Builder can now be populated Nexus-style from named Model pairs because
// Model itself is a cheap intrusive-handle copy with copy-on-write detachment
// before mutators. Copying a registry into Builder shares model state until
// either side mutates; no std::move ceremony is required for ordinary use.
// Builder still offers exactly one mutating .inject(...) operation. A
// value-returning inject sibling would be API bloat here because the
// coordinator is intentionally a mutable registry, not a data object whose
// functional updates are the common path.
//
// Duplicate names: matches Nexus::inject(key, frame)'s real behavior
// (NexusInternals.inc's append_named_frame -- PLUSS_CHECK's a duplicate key
// is an error, not a silent overwrite) rather than "last write wins" --
// re-injecting an existing name is almost always a caller mistake (a typo
// colliding with an existing entry) worth surfacing immediately, not a
// deliberate in-place swap; eject the old one first if a real replacement
// is intended.
//
// Construction still goes through the lowercase `models::builder::`
// subfamily per the redesign's construction-vs-method rule
// (LYMPH_REDESIGN_PLAN.md 2.0): `models::builder::build()` for an empty
// Builder, `models::builder::build({{"name", model}, {"inline", models::...()}})` for
// Nexus-like construction-time population, or the map/vector overloads for
// preassembled registries. Builder entries are strictly named; there is no
// unnamed injection or implicit model_0/model_1 counter in this surface.
namespace pluss::ai::lymph {

// TaskKind has no name() helper elsewhere in lymph (metrics.h's
// metric_kind_name_ is the closest precedent, for MetricKind) -- small and
// local to this file's describe() table; promote elsewhere if another file
// ends up needing it too.
[[nodiscard]] inline std::string task_kind_name_(TaskKind kind) {
  switch (kind) {
    case TaskKind::Classification: return "classification";
    case TaskKind::Regression: return "regression";
    case TaskKind::Transform: return "transform";
  }
  return "task";
}

namespace models::builder {

class Builder final {
 public:
  using Entry = std::pair<std::string, Model>;

  Builder() = default;
  Builder(Builder&&) noexcept = default;
  Builder& operator=(Builder&&) noexcept = default;
  Builder(const Builder&) = delete;
  Builder& operator=(const Builder&) = delete;

  // Registers `model` under `name`. Returns *this so calls chain:
  //   builder.inject("logreg", models::logistic_regression())
  //          .inject("tree", models::decision_tree_classifier());
  // A freshly-built prvalue needs no std::move at the call site; an
  // already-named Model lvalue is copied cheaply and detached only if one
  // side later mutates. Throws on an empty name or a name already registered
  // (see the duplicate-names note above).
  Builder& inject(std::string name, Model model) {
    if (name.empty()) {
      ::pluss::throw_error("lymph::Builder::inject: name must not be empty.");
    }
    if (has(name)) {
      ::pluss::throw_error("lymph::Builder::inject: \"" + name + "\" is already registered; eject it first to replace it.");
    }
    items_.emplace_back(std::move(name), std::move(model));
    return *this;
  }

  // Registers several named models without requiring a long chain of
  // single-entry inject calls. Validate the complete batch before changing
  // the registry, so an invalid later entry cannot leave an earlier entry
  // partially registered.
  Builder& inject(pluss::msharp::RangeView::Of<Entry> entries) {
    std::vector<std::string_view> pending_names{};
    pending_names.reserve(entries.size());

    for (const auto& [name, model] : entries) {
      (void)model;
      if (name.empty()) {
        ::pluss::throw_error("lymph::Builder::inject: name must not be empty.");
      }
      if (has(name)) {
        ::pluss::throw_error("lymph::Builder::inject: \"" + name + "\" is already registered; eject it first to replace it.");
      }
      for (const std::string_view pending : pending_names) {
        if (pending == name) {
          ::pluss::throw_error("lymph::Builder::inject: \"" + name + "\" appears more than once in the supplied entries.");
        }
      }
      pending_names.push_back(name);
    }

    items_.reserve(items_.size() + entries.size());
    for (const auto& [name, model] : entries) {
      items_.emplace_back(name, model);
    }
    return *this;
  }

  Builder& inject(std::initializer_list<Entry> entries) {
    return inject(pluss::msharp::RangeView::Of<Entry>(entries));
  }

  // Bulk inject from a preassembled name->Model map. The container's keys
  // are already distinct, so the intra-batch duplicate scan the RangeView
  // overload runs is unnecessary here; only the empty-name and
  // already-registered checks apply. The whole map is still validated before
  // a single entry is added, so a collision on any key leaves the registry
  // exactly as it was. Models are copied cheaply (copy-on-write handles) and
  // detach only if one side later mutates.
  Builder& inject(const std::map<std::string, Model>& entries) {
    return inject_assoc_(entries);
  }

  Builder& inject(const std::unordered_map<std::string, Model>& entries) {
    return inject_assoc_(entries);
  }

  // Removes and returns the model registered under `name`; the Builder no
  // longer holds it afterward.
  [[nodiscard]] Model eject(std::string_view name) {
    for (std::size_t i = 0; i < items_.size(); ++i) {
      if (items_[i].first == name) {
        Model out = std::move(items_[i].second);
        items_.erase(items_.begin() + static_cast<std::ptrdiff_t>(i));
        return out;
      }
    }
    ::pluss::throw_error("lymph::Builder::eject: no model registered under \"" + std::string(name) + "\".");
  }

  [[nodiscard]] bool has(std::string_view name) const noexcept {
    for (const auto& item : items_) {
      if (item.first == name) return true;
    }
    return false;
  }

  [[nodiscard]] const Model& get(std::string_view name) const {
    for (const auto& item : items_) {
      if (item.first == name) return item.second;
    }
    ::pluss::throw_error("lymph::Builder::get: no model registered under \"" + std::string(name) + "\".");
  }

  [[nodiscard]] Model& get(std::string_view name) {
    for (auto& item : items_) {
      if (item.first == name) return item.second;
    }
    ::pluss::throw_error("lymph::Builder::get: no model registered under \"" + std::string(name) + "\".");
  }

  [[nodiscard]] std::int64_t size() const noexcept { return static_cast<std::int64_t>(items_.size()); }
  [[nodiscard]] bool empty() const noexcept { return items_.empty(); }

  // Every registered name, in insertion order -- matches Nexus::keys()/
  // .names().
  [[nodiscard]] std::vector<std::string> names() const {
    std::vector<std::string> out{};
    out.reserve(items_.size());
    for (const auto& item : items_) out.push_back(item.first);
    return out;
  }

  // Preview of the first `n` names (n < 0, or n >= size(), returns all of
  // them) -- the plain-list sibling of report() below; the split the user
  // asked for over a single DataFrame-returning .list().
  [[nodiscard]] std::vector<std::string> list(std::int64_t n = -1) const {
    const std::int64_t count = (n < 0 || n > size()) ? size() : n;
    std::vector<std::string> out{};
    out.reserve(static_cast<std::size_t>(count));
    for (std::int64_t i = 0; i < count; ++i) out.push_back(items_[static_cast<std::size_t>(i)].first);
    return out;
  }

  // Rich, tabular sibling of list()/names() -- one row per held model:
  // name, task_kind, trained, stage_count. Matches Nexus::summary()/
  // .profile()'s own convention (a DataFrame, .show()/<<-streamable, not a
  // hand-rolled report) rather than inventing a second reporting format.
  [[nodiscard]] pluss::table::DataFrame report(std::int64_t n = -1) const;

  // Clones every held model (Model::clone() -- dump+reload trained
  // state, not a retrain) into a fresh selection::Candidate, leaving this
  // Builder's own copies untouched. Exposed on its own, not only used
  // internally by benchmark() below, so a caller can still reach for
  // selection::compare(...) directly with the candidates if
  // benchmark()'s canned overload set doesn't fit.
  [[nodiscard]] std::vector<selection::Candidate> candidates() const {
    std::vector<selection::Candidate> out{};
    out.reserve(items_.size());
    for (const auto& item : items_) {
      out.emplace_back(item.first, item.second.clone());
    }
    return out;
  }

  // Spot-checks every held model via selection::compare(...) -- the engine
  // behind what the redesign calls Benchmark (selection.h) -- without the
  // caller ever listing candidates by hand. Non-destructive: the comparison
  // runs on clones (candidates() above), so this Builder still holds every
  // model it started with regardless of how the benchmark comes out.
  // Direct LabeledBatch entry point -- see selection::compare's own
  // LabeledBatch overload (selection.h) for why this exists: DataFrame is
  // deliberately CPU-only, but LabeledBatch is nothing but raw Series ({x,
  // y, sample_weights} -- nodej/dataset.h), so training/spot-checking on
  // GPU-resident data (a Series built with
  // `Config{}.with_platform(pluss::pGPU)`) goes through here, no DataFrame
  // detour and no separate "which platform" argument required -- the
  // platform already lives on the Series you hand in.
  [[nodiscard]] selection::CompareResult benchmark(const LabeledBatch& batch,
                                                    selection::CrossValOptions cv = {},
                                                    TrainOptions train_options = {},
                                                    AssessOptions assess_options = {},
                                                    MetricKind objective_metric = MetricKind::Accuracy,
                                                    TuneObjectivePolicy objective_policy = TuneObjectivePolicy::Auto,
                                                    selection::FitOptions fit_options = {}) const {
    return remember_(selection::compare(candidates(), batch, cv, train_options, assess_options,
                                        objective_metric, objective_policy, fit_options));
  }

  [[nodiscard]] selection::CompareResult benchmark(const pluss::table::DataFrame& x,
                                                    const Series& y,
                                                    selection::CrossValOptions cv = {},
                                                    TrainOptions train_options = {},
                                                    AssessOptions assess_options = {},
                                                    MetricKind objective_metric = MetricKind::Accuracy,
                                                    TuneObjectivePolicy objective_policy = TuneObjectivePolicy::Auto,
                                                    selection::FitOptions fit_options = {}) const {
    return remember_(selection::compare(candidates(), x, y, cv, train_options, assess_options,
                                        objective_metric, objective_policy, fit_options));
  }

  // (frame, DataFrameBatchOptions) sibling -- see confusion_table's twin in
  // detail/model_core.h for why this exists alongside the (x, y) form (a raw
  // Note column, e.g. via DataNotePolicy::Factorize, has nowhere to go in
  // the (x, y) form).
  [[nodiscard]] selection::CompareResult benchmark(const pluss::table::DataFrame& frame,
                                                    const DataFrameBatchOptions& data_options,
                                                    selection::CrossValOptions cv = {},
                                                    TrainOptions train_options = {},
                                                    AssessOptions assess_options = {},
                                                    MetricKind objective_metric = MetricKind::Accuracy,
                                                    TuneObjectivePolicy objective_policy = TuneObjectivePolicy::Auto,
                                                    selection::FitOptions fit_options = {}) const {
    return remember_(selection::compare(candidates(), frame, data_options, cv, train_options, assess_options,
                                        objective_metric, objective_policy, fit_options));
  }

  [[nodiscard]] bool benchmarked() const noexcept {
    return last_result_.has_value();
  }

  [[nodiscard]] Model best() const {
    return require_last_result_().best();
  }

  [[nodiscard]] Model worst() const {
    return require_last_result_().worst();
  }

 private:
  // Shared body for the std::map / std::unordered_map inject overloads.
  // Two passes: validate every entry, then insert every entry, so the
  // registry is never left half-updated by a mid-batch collision.
  template <class AssocMap>
  Builder& inject_assoc_(const AssocMap& entries) {
    for (const auto& [name, model] : entries) {
      (void)model;
      if (name.empty()) {
        ::pluss::throw_error("lymph::Builder::inject: name must not be empty.");
      }
      if (has(name)) {
        ::pluss::throw_error("lymph::Builder::inject: \"" + name + "\" is already registered; eject it first to replace it.");
      }
    }
    items_.reserve(items_.size() + entries.size());
    for (const auto& [name, model] : entries) {
      items_.emplace_back(name, model);
    }
    return *this;
  }

  [[nodiscard]] const selection::CompareResult& require_last_result_() const {
    if (!last_result_.has_value()) {
      ::pluss::throw_error("lymph::Builder: benchmark() must be run before best() or worst().");
    }
    return *last_result_;
  }

  [[nodiscard]] selection::CompareResult remember_(selection::CompareResult result) const {
    last_result_ = result;
    return result;
  }

  std::vector<std::pair<std::string, Model>> items_{};
  mutable std::optional<selection::CompareResult> last_result_{};
};
} // namespace models::builder

namespace models {
using Builder = builder::Builder;
} // namespace models

namespace models::builder {

[[nodiscard]] inline pluss::table::DataFrame Builder::report(std::int64_t n) const {
  const std::int64_t count = (n < 0 || n > size()) ? size() : n;
  std::vector<std::string> names_col{};
  std::vector<std::string> task_kinds{};
  std::vector<std::string> trained{};  // "true"/"false" -- make_note has no bool overload
  std::vector<std::int64_t> stage_counts{};
  names_col.reserve(static_cast<std::size_t>(count));
  task_kinds.reserve(static_cast<std::size_t>(count));
  trained.reserve(static_cast<std::size_t>(count));
  stage_counts.reserve(static_cast<std::size_t>(count));

  for (std::int64_t i = 0; i < count; ++i) {
    const auto& item = items_[static_cast<std::size_t>(i)];
    names_col.push_back(item.first);
    task_kinds.push_back(task_kind_name_(item.second.task_kind()));
    trained.push_back(item.second.trained() ? "true" : "false");
    stage_counts.push_back(item.second.stage_count());
  }

  const Config i64_cfg = Config{}.with_dtype(ScalarType::Int64).with_platform(pluss::pCPU);
  const auto row_n = static_cast<std::int64_t>(count);

  std::vector<std::pair<std::string, pluss::table::Thread>> columns{};
  columns.reserve(4);
  columns.emplace_back("name", pluss::table::Thread(pluss::make_note(names_col)));
  columns.emplace_back("task_kind", pluss::table::Thread(pluss::make_note(task_kinds)));
  columns.emplace_back("trained", pluss::table::Thread(pluss::make_note(trained)));
  columns.emplace_back("stage_count", pluss::table::Thread(pluss::make_series(stage_counts, {row_n}, i64_cfg)));

  return pluss::table::DataFrame(std::move(columns)).title("Builder");
}

[[nodiscard]] inline Builder build() { return Builder(); }

[[nodiscard]] inline Builder build(std::initializer_list<std::pair<std::string, Model>> entries) {
  Builder out;
  for (const auto& entry : entries) {
    out.inject(entry.first, entry.second);
  }
  return out;
}

[[nodiscard]] inline Builder build(const std::map<std::string, Model>& entries) {
  Builder out;
  for (const auto& [name, model] : entries) {
    out.inject(name, model);
  }
  return out;
}

[[nodiscard]] inline Builder build(const std::vector<std::pair<std::string, Model>>& entries) {
  Builder out;
  for (const auto& entry : entries) {
    out.inject(entry.first, entry.second);
  }
  return out;
}

[[nodiscard]] inline Builder build(std::map<std::string, Model>&& entries) {
  Builder out;
  for (auto& [name, model] : entries) out.inject(std::move(name), std::move(model));
  return out;
}

[[nodiscard]] inline Builder build(std::vector<std::pair<std::string, Model>>&& entries) {
  Builder out;
  for (auto& entry : entries) out.inject(std::move(entry.first), std::move(entry.second));
  return out;
}

} // namespace models::builder

namespace models {
using builder::build;
} // namespace models

} // namespace pluss::ai::lymph
