#pragma once

#include "pluss/ai/lymph/model.h"
#include "pluss/ai/lymph/payload.h"
#include "pluss/ai/lymph/policy.h"
#include "pluss/ai/lymph/time.h"

#include <cstdint>
#include <fstream>
#include <functional>
#include <initializer_list>
#include <sstream>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

// workflow.h -- Workflow composition object.
//
// Role: the trainable/runnable ML composition object -- currently Model +
// Policy, with phase-1 widening so it behaves like a single end-to-end object
// over the same core actions Model supports. Rich routed branches, time
// safety, inverse contracts, and custom stage/model surfaces land in later
// Workflow-rival phases; production deployment hardening belongs to the
// future Procedure family, not this type.
//
// Implemented surface:
//   - class Workflow -- cheap-copy handle-shaped value, mirroring Model's
//     normal user ergonomics (copy now, detach in Model on mutation).
//   - learn_/learn/map/learn_map/infer/infer_scores/assess/save, raw-frame
//     paths applying Policy before the held Model sees data, plus batch paths
//     for callers already working at Series/FeatureBatch/LabeledBatch level.
//   - save_policy(path, Policy) / load_policy(path) -- sidecar persistence
//     helpers kept here because they are Workflow-only concerns.
//   - namespace lymph::workflow { Workflow make(Model, Policy); Workflow
//     restore(FlowBundle, Policy); Workflow load(std::string path); }
//
// Depends on: model.h (Model), policy.h (Policy), NodeJ's io::save/
// io::load/FlowBundle. The sidecar file is intentionally outside
// FlowBundle's own schema.
//
// Consumed by: eventually validate/search/spotcheck/reporting families once
// those phases learn to run Workflow directly. Workflow itself must not absorb
// those families' responsibilities.

namespace pluss::ai::lymph {

namespace workflow {

struct WorkflowBranchInfo {
  std::string name{};
  bool named = false;
  std::int64_t stage_count = 0;
};

enum class WorkflowSchedule {
  Deterministic,
  Adaptive,
};

enum class WorkflowCache {
  Off,
  Stage,
};

struct WorkflowOptions {
  std::int64_t workers = 1;
  WorkflowSchedule schedule = WorkflowSchedule::Deterministic;
  WorkflowCache cache = WorkflowCache::Off;
};

struct WorkflowStepSpec {
  std::string name{};
  Stage stage{};
};

struct WorkflowBranchSpec {
  std::string name{};
  std::vector<std::string> columns{};
  std::vector<WorkflowStepSpec> steps{};
};

class WorkflowBranch;
class WorkflowRoute;
class Workflow;

} // namespace workflow

using WorkflowBranchInfo = workflow::WorkflowBranchInfo;
using WorkflowSchedule = workflow::WorkflowSchedule;
using WorkflowCache = workflow::WorkflowCache;
using WorkflowOptions = workflow::WorkflowOptions;
using WorkflowStepSpec = workflow::WorkflowStepSpec;
using WorkflowBranchSpec = workflow::WorkflowBranchSpec;
using WorkflowBranch = workflow::WorkflowBranch;
using WorkflowRoute = workflow::WorkflowRoute;
using Workflow = workflow::Workflow;

namespace detail {

[[nodiscard]] inline std::string workflow_policy_sidecar_path_(const std::string& bundle_path) {
  return bundle_path + ".policy";
}

inline void require_workflow_xy_policy_safe_(const Policy& policy, const char* where) {
  if (policy.filter_nulls) {
    ::pluss::throw_error(std::string(where) +
                         ": Policy can drop rows; use the frame + DataFrameBatchOptions overload so features and labels stay aligned.");
  }
}

inline void require_frame_payload_weight_column_(const Payload& payload, const char* where) {
  if (payload.weights().defined() && !payload.has_weight_feature()) {
    ::pluss::throw_error(std::string(where) +
                         ": DataFrame-backed Payload with Series weights must use weights(\"column\") so policy/filtering keeps rows aligned.");
  }
}

inline void write_policy_name_list_(std::ofstream& out, const std::vector<std::string>& names) {
  out << names.size() << "\n";
  for (const std::string& name : names) out << name << "\n";
}

[[nodiscard]] inline std::vector<std::string> read_policy_name_list_(std::ifstream& in, const char* where) {
  std::size_t count = 0;
  if (!(in >> count)) {
    ::pluss::throw_error(std::string(where) + ": malformed workflow policy sidecar (name count).");
  }
  in.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  std::vector<std::string> out{};
  out.reserve(count);
  for (std::size_t i = 0; i < count; ++i) {
    std::string line;
    if (!std::getline(in, line)) {
      ::pluss::throw_error(std::string(where) + ": malformed workflow policy sidecar (expected " +
                           std::to_string(count) + " names).");
    }
    out.push_back(std::move(line));
  }
  return out;
}

} // namespace detail

inline void save_policy(const std::string& bundle_path, const Policy& policy) {
  const std::string sidecar = detail::workflow_policy_sidecar_path_(bundle_path);
  std::ofstream out(sidecar, std::ios::trunc);
  if (!out) {
    ::pluss::throw_error("lymph::save_policy: could not open \"" + sidecar + "\" for writing.");
  }
  out << "LYMPH_POLICY_V2\n";
  detail::write_policy_name_list_(out, policy.drop_columns);
  detail::write_policy_name_list_(out, policy.require_columns);
  detail::write_policy_name_list_(out, policy.forbid_columns);
  detail::write_policy_name_list_(out, policy.reject_missing);
  detail::write_policy_name_list_(out, policy.allow_missing);
  out << (policy.filter_nulls ? 1 : 0) << "\n";
  out << (policy.drop_only_if_all_null ? 1 : 0) << "\n";
  detail::write_policy_name_list_(out, policy.null_filter_columns);
}

[[nodiscard]] inline Policy load_policy(const std::string& bundle_path) {
  const std::string sidecar = detail::workflow_policy_sidecar_path_(bundle_path);
  std::ifstream in(sidecar);
  if (!in) {
    return Policy{};
  }
  Policy policy{};
  std::string first;
  if (!std::getline(in, first)) {
    return policy;
  }
  if (first == "LYMPH_POLICY_V2") {
    policy.drop_columns = detail::read_policy_name_list_(in, "lymph::load_policy");
    policy.require_columns = detail::read_policy_name_list_(in, "lymph::load_policy");
    policy.forbid_columns = detail::read_policy_name_list_(in, "lymph::load_policy");
    policy.reject_missing = detail::read_policy_name_list_(in, "lymph::load_policy");
    policy.allow_missing = detail::read_policy_name_list_(in, "lymph::load_policy");
  } else {
    std::istringstream first_in(first);
    std::size_t count = 0;
    if (!(first_in >> count)) {
      ::pluss::throw_error("lymph::load_policy: malformed workflow policy sidecar (version/name count).");
    }
    policy.drop_columns.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
      std::string line;
      if (!std::getline(in, line)) {
        ::pluss::throw_error("lymph::load_policy: malformed workflow policy sidecar (legacy drop column list).");
      }
      policy.drop_columns.push_back(std::move(line));
    }
  }
  int filter_nulls = 0;
  int drop_only_if_all_null = 0;
  if (!(in >> filter_nulls) || !(in >> drop_only_if_all_null)) {
    ::pluss::throw_error("lymph::load_policy: malformed workflow policy sidecar (flags).");
  }
  in.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  policy.filter_nulls = filter_nulls != 0;
  policy.drop_only_if_all_null = drop_only_if_all_null != 0;
  policy.null_filter_columns = detail::read_policy_name_list_(in, "lymph::load_policy");
  return policy;
}

namespace workflow {
[[nodiscard]] Workflow make(Policy policy);
[[nodiscard]] Workflow make(Model model, Policy policy = {});
[[nodiscard]] Workflow make(Policy policy,
                            std::vector<WorkflowBranchSpec> branches,
                            WorkflowOptions options = {});
[[nodiscard]] Workflow make(Policy policy,
                            std::initializer_list<WorkflowBranchSpec> branches,
                            WorkflowOptions options = {});
[[nodiscard]] Workflow make(Policy policy,
                            std::vector<WorkflowBranchSpec> branches,
                            Model model,
                            WorkflowOptions options = {});
[[nodiscard]] Workflow make(Policy policy,
                            std::initializer_list<WorkflowBranchSpec> branches,
                            Model model,
                            WorkflowOptions options = {});
[[nodiscard]] Workflow make(std::vector<WorkflowBranchSpec> branches,
                            Model model,
                            WorkflowOptions options = {});
[[nodiscard]] Workflow make(std::initializer_list<WorkflowBranchSpec> branches,
                            Model model,
                            WorkflowOptions options = {});
[[nodiscard]] WorkflowBranchSpec branch(std::string name,
                                        std::vector<std::string> columns,
                                        std::vector<WorkflowStepSpec> steps);
[[nodiscard]] WorkflowBranchSpec branch(std::string name,
                                        std::initializer_list<std::string> columns,
                                        std::initializer_list<WorkflowStepSpec> steps);
[[nodiscard]] Workflow restore(const io::FlowBundle& bundle, Policy policy = {});
[[nodiscard]] Workflow load(const std::string& path);
} // namespace workflow

class workflow::Workflow final {
 public:
  Workflow(Workflow&&) noexcept = default;
  Workflow& operator=(Workflow&&) noexcept = default;
  Workflow(const Workflow&) = default;
  Workflow& operator=(const Workflow&) = default;

  [[nodiscard]] bool trained() const noexcept { return model_.trained(); }
  [[nodiscard]] bool has_model() const noexcept { return model_.has_model(); }
  [[nodiscard]] bool has_model_spec() const noexcept { return model_.has_model_spec(); }
  [[nodiscard]] bool has_opaque_transforms() const noexcept { return model_.has_opaque_transforms(); }
  [[nodiscard]] std::int64_t stage_count() const noexcept { return model_.stage_count(); }
  [[nodiscard]] TaskKind task_kind() const noexcept { return model_.task_kind(); }
  [[nodiscard]] const std::optional<ModelSpec>& model_spec() const noexcept { return model_.model_spec(); }
  [[nodiscard]] const std::vector<TransformSpec>& transform_specs() const noexcept { return model_.transform_specs(); }
  [[nodiscard]] FlowGraphInfo graph_info() const { return model_.graph_info(); }
  [[nodiscard]] std::vector<std::string> nested_param_keys() const { return model_.nested_param_keys(); }
  [[nodiscard]] double nested_param(std::string_view path) const { return model_.nested_param(path); }
  [[nodiscard]] std::string flow_summary() const { return model_.flow_summary(); }
  [[nodiscard]] std::string save_blueprint() const { return model_.save_blueprint(); }
  [[nodiscard]] ModelPrep prep() noexcept { return model_.prepare(); }
  [[nodiscard]] const std::vector<WorkflowBranchInfo>& branches() const noexcept { return branches_; }
  [[nodiscard]] std::int64_t branch_count() const noexcept {
    return static_cast<std::int64_t>(branches_.size());
  }
  [[nodiscard]] bool joined() const noexcept { return joined_; }
  [[nodiscard]] const WorkflowOptions& options() const noexcept { return options_; }
  [[nodiscard]] std::int64_t workers() const noexcept { return options_.workers; }
  [[nodiscard]] bool time_aware() const noexcept { return time_options_.has_value(); }
  [[nodiscard]] const std::optional<TimeOptions>& time_options() const noexcept { return time_options_; }

  Workflow& workers(std::int64_t value) {
    if (value < 1) {
      ::pluss::throw_error("lymph::Workflow::workers: worker count must be at least 1.");
    }
    options_.workers = value;
    return *this;
  }

  Workflow& join() noexcept {
    joined_ = true;
    return *this;
  }

  Workflow& time(TimeOptions options) {
    validate_time_options(options, "lymph::Workflow::time");
    time_options_ = std::move(options);
    return *this;
  }

  [[nodiscard]] Workflow with(Model model) const {
    Workflow out(std::move(model), policy_);
    out.branches_ = branches_;
    out.steps_ = steps_;
    out.joined_ = joined_;
    out.options_ = options_;
    out.time_options_ = time_options_;
    for (const WorkflowStepRecord& step : out.steps_) {
      out.model_.append(step.stage, step.router);
    }
    return out;
  }

  [[nodiscard]] WorkflowBranch branch(std::string name);

  [[nodiscard]] WorkflowRoute on(ColumnRouter router);

  [[nodiscard]] WorkflowRoute on(std::vector<std::string> names);

  [[nodiscard]] WorkflowRoute on(std::initializer_list<std::string> names);

  [[nodiscard]] WorkflowRoute on(std::string name, ColumnRouter router);

  [[nodiscard]] WorkflowRoute on(std::string name, std::vector<std::string> names);

  [[nodiscard]] WorkflowRoute on(std::string name, std::initializer_list<std::string> names);

  [[nodiscard]] WorkflowRoute on(
      std::function<bool(std::string_view feature_name, std::string_view dtype)> predicate);

  [[nodiscard]] WorkflowRoute on(
      std::string name,
      std::function<bool(std::string_view feature_name, std::string_view dtype)> predicate);

  Workflow& add(const CustomStage& stage) {
    model_.add(stage);
    return *this;
  }

  Workflow& learn_(const pluss::table::DataFrame& raw_frame,
                   const DataFrameBatchOptions& data_options,
                   const TrainOptions& options = {}) {
    model_.learn_(policy_.apply(raw_frame), data_options, options);
    return *this;
  }

  Workflow& learn_(const LabeledBatch& batch, const TrainOptions& options = {}) {
    model_.learn_(batch, options);
    return *this;
  }

  Workflow& learn_(const pluss::table::DataFrame& x,
                   const Series& y,
                   const TrainOptions& options = {}) {
    detail::require_workflow_xy_policy_safe_(policy_, "lymph::Workflow::learn_(x,y)");
    model_.learn_(policy_.apply(x), y, options);
    return *this;
  }

  Workflow& learn_(const pluss::table::DataFrame& x,
                   const pluss::table::Thread& y,
                   const TrainOptions& options = {}) {
    detail::require_workflow_xy_policy_safe_(policy_, "lymph::Workflow::learn_(x,y)");
    model_.learn_(policy_.apply(x), y, options);
    return *this;
  }

  [[nodiscard]] Workflow learn(const pluss::table::DataFrame& raw_frame,
                               const DataFrameBatchOptions& data_options,
                               const TrainOptions& options = {}) const {
    Workflow out(*this);
    out.learn_(raw_frame, data_options, options);
    return out;
  }

  [[nodiscard]] Workflow learn(const LabeledBatch& batch, const TrainOptions& options = {}) const {
    Workflow out(*this);
    out.learn_(batch, options);
    return out;
  }

  [[nodiscard]] Workflow learn(const pluss::table::DataFrame& x,
                               const Series& y,
                               const TrainOptions& options = {}) const {
    Workflow out(*this);
    out.learn_(x, y, options);
    return out;
  }

  [[nodiscard]] Workflow learn(const pluss::table::DataFrame& x,
                               const pluss::table::Thread& y,
                               const TrainOptions& options = {}) const {
    Workflow out(*this);
    out.learn_(x, y, options);
    return out;
  }

  Workflow& learn_(const Payload& payload, const TrainOptions& options = {}) {
    if (payload.has_frame()) {
      const pluss::table::DataFrame frame = policy_.apply(payload.frame());
      if (payload.has_label_feature()) {
        detail::require_frame_payload_weight_column_(payload, "lymph::Workflow::learn_(payload)");
        model_.learn_(frame, payload.data(), options);
        return *this;
      }
      if (payload.has_label()) {
        detail::require_workflow_xy_policy_safe_(policy_, "lymph::Workflow::learn_(payload)");
        model_.learn_(frame, payload.label(), options);
        return *this;
      }
    }
    model_.learn_(payload.labeled_batch(), options);
    return *this;
  }

  [[nodiscard]] Workflow learn(const Payload& payload, const TrainOptions& options = {}) const {
    Workflow out(*this);
    out.learn_(payload, options);
    return out;
  }

  [[nodiscard]] Series map(const FeatureBatch& batch, const InferOptions& options = {}) const {
    return model_.map(batch, options);
  }

  [[nodiscard]] Series map(const pluss::table::DataFrame& raw_frame, const InferOptions& options = {}) const {
    return model_.map(policy_.apply(raw_frame), options);
  }

  [[nodiscard]] Series map(const pluss::table::DataFrame& raw_frame,
                           const DataFrameBatchOptions& data_options,
                           const InferOptions& options = {}) const {
    return model_.map(policy_.apply(raw_frame), data_options, options);
  }

  [[nodiscard]] Series map(const Payload& payload, const InferOptions& options = {}) const {
    if (payload.has_frame()) {
      return model_.map(policy_.apply(payload.frame()), payload.data(), options);
    }
    return model_.map(payload.feature_batch(), options);
  }

  [[nodiscard]] Series learn_map(const FeatureBatch& batch,
                                 const TrainOptions& learn_options = {},
                                 const InferOptions& infer_options = {}) {
    return model_.learn_map(batch, learn_options, infer_options);
  }

  [[nodiscard]] Series learn_map(const pluss::table::DataFrame& raw_frame,
                                 const DataFrameBatchOptions& data_options,
                                 const TrainOptions& learn_options = {},
                                 const InferOptions& infer_options = {}) {
    return model_.learn_map(policy_.apply(raw_frame), data_options, learn_options, infer_options);
  }

  [[nodiscard]] Series learn_map(const pluss::table::DataFrame& raw_frame,
                                 const TrainOptions& learn_options = {},
                                 const InferOptions& infer_options = {}) {
    return model_.learn_map(policy_.apply(raw_frame), learn_options, infer_options);
  }

  [[nodiscard]] Series learn_map(const Payload& payload,
                                 const TrainOptions& learn_options = {},
                                 const InferOptions& infer_options = {}) {
    if (payload.has_frame()) {
      return model_.learn_map(policy_.apply(payload.frame()), payload.data(), learn_options, infer_options);
    }
    return model_.learn_map(payload.feature_batch(), learn_options, infer_options);
  }

  [[nodiscard]] Series infer(const FeatureBatch& batch, const InferOptions& options = {}) const {
    return model_.infer(batch, options);
  }

  [[nodiscard]] Series infer(const pluss::table::DataFrame& raw_frame, const InferOptions& options = {}) const {
    return model_.infer(policy_.apply(raw_frame), options);
  }

  [[nodiscard]] Series infer(const pluss::table::DataFrame& raw_frame,
                             const DataFrameBatchOptions& data_options,
                             const InferOptions& options = {}) const {
    return model_.infer(policy_.apply(raw_frame), data_options, options);
  }

  [[nodiscard]] Series infer(const Payload& payload, const InferOptions& options = {}) const {
    if (payload.has_frame()) {
      return model_.infer(policy_.apply(payload.frame()), payload.data(), options);
    }
    return model_.infer(payload.feature_batch(), options);
  }

  [[nodiscard]] Series infer_scores(const FeatureBatch& batch, const InferOptions& options = {}) const {
    return model_.infer_scores(batch, options);
  }

  [[nodiscard]] Series infer_scores(const pluss::table::DataFrame& raw_frame,
                                    const InferOptions& options = {}) const {
    return model_.infer_scores(policy_.apply(raw_frame), options);
  }

  [[nodiscard]] Series infer_scores(const pluss::table::DataFrame& raw_frame,
                                    const DataFrameBatchOptions& data_options,
                                    const InferOptions& options = {}) const {
    return model_.infer_scores(policy_.apply(raw_frame), data_options, options);
  }

  [[nodiscard]] Series infer_scores(const Payload& payload, const InferOptions& options = {}) const {
    if (payload.has_frame()) {
      return model_.infer_scores(policy_.apply(payload.frame()), payload.data(), options);
    }
    return model_.infer_scores(payload.feature_batch(), options);
  }

  [[nodiscard]] MetricReport assess(const LabeledBatch& batch, const AssessOptions& options = {}) const {
    return model_.assess(batch, options);
  }

  [[nodiscard]] MetricReport assess(const pluss::table::DataFrame& raw_frame,
                                    const DataFrameBatchOptions& data_options,
                                    const AssessOptions& options = {}) const {
    return model_.assess(policy_.apply(raw_frame), data_options, options);
  }

  [[nodiscard]] MetricReport assess(const pluss::table::DataFrame& x,
                                    const Series& y,
                                    const AssessOptions& options = {}) const {
    detail::require_workflow_xy_policy_safe_(policy_, "lymph::Workflow::assess(x,y)");
    return model_.assess(policy_.apply(x), y, options);
  }

  [[nodiscard]] MetricReport assess(const pluss::table::DataFrame& x,
                                    const pluss::table::Thread& y,
                                    const AssessOptions& options = {}) const {
    detail::require_workflow_xy_policy_safe_(policy_, "lymph::Workflow::assess(x,y)");
    return model_.assess(policy_.apply(x), y, options);
  }

  [[nodiscard]] MetricReport assess(const Payload& payload, const AssessOptions& options = {}) const {
    if (payload.has_frame()) {
      const pluss::table::DataFrame frame = policy_.apply(payload.frame());
      if (payload.has_label_feature()) {
        detail::require_frame_payload_weight_column_(payload, "lymph::Workflow::assess(payload)");
        return model_.assess(frame, payload.data(), options);
      }
      if (payload.has_label()) {
        detail::require_workflow_xy_policy_safe_(policy_, "lymph::Workflow::assess(payload)");
        return model_.assess(frame, payload.label(), options);
      }
    }
    return model_.assess(payload.labeled_batch(), options);
  }

  void save(const std::string& path,
            const pluss::table::DataFrame& raw_train_frame,
            const DataFrameBatchOptions& data_options,
            const TrainOptions& train_options = {},
            io::FlowBundleMetadata metadata = {}) const {
    model_.save(path, policy_.apply(raw_train_frame), data_options, train_options, std::move(metadata));
    save_policy(path, policy_);
  }

  void save(const std::string& path) const {
    model_.save(path);
    save_policy(path, policy_);
  }

  void save(const std::string& path,
            io::FlowBundleMetadata metadata) const {
    model_.save(path, std::move(metadata));
    save_policy(path, policy_);
  }

  void save(const std::string& path,
            const Payload& payload,
            const TrainOptions& train_options = {},
            io::FlowBundleMetadata metadata = {}) const;

  [[nodiscard]] const Model& model() const noexcept { return model_; }
  [[nodiscard]] Model& model() noexcept { return model_; }
  [[nodiscard]] const Policy& policy() const noexcept { return policy_; }

 private:
  struct WorkflowStepRecord {
    WorkflowStepRecord(std::int64_t in_branch_index, Stage in_stage, ColumnRouter in_router)
        : branch_index(in_branch_index), stage(std::move(in_stage)), router(std::move(in_router)) {}

    std::int64_t branch_index;
    Stage stage;
    ColumnRouter router;
  };

  [[nodiscard]] WorkflowRoute open_branch_(std::string name, bool named, ColumnRouter router);

  Model& append_step_(std::int64_t branch_index, Stage stage, const ColumnRouter& router) {
    if (branch_index < 0 || branch_index >= static_cast<std::int64_t>(branches_.size())) {
      ::pluss::throw_error("lymph::Workflow: invalid branch index while appending a stage.");
    }
    Stage stored_stage = stage;
    ColumnRouter stored_router = router;
    model_.append(std::move(stage), router);
    steps_.push_back(WorkflowStepRecord{branch_index, std::move(stored_stage), std::move(stored_router)});
    ++branches_[static_cast<std::size_t>(branch_index)].stage_count;
    return model_;
  }

  friend class WorkflowBranch;
  friend Workflow workflow::make(Model model, Policy policy);
  friend Workflow workflow::make(Policy policy);
  friend Workflow workflow::restore(const io::FlowBundle& bundle, Policy policy);
  friend class WorkflowRoute;

  Workflow(Model model, Policy policy)
      : model_(std::move(model)), policy_(std::move(policy)) {}

  Model model_;
  Policy policy_;
  std::vector<WorkflowBranchInfo> branches_{};
  std::vector<WorkflowStepRecord> steps_{};
  bool joined_ = false;
  WorkflowOptions options_{};
  std::optional<TimeOptions> time_options_{};
};

class workflow::WorkflowBranch final {
 public:
  WorkflowBranch(Workflow& workflow, std::string name)
      : workflow_(&workflow), name_(std::move(name)) {
    if (name_.empty()) {
      ::pluss::throw_error("lymph::WorkflowBranch: branch name cannot be empty.");
    }
  }

  [[nodiscard]] WorkflowRoute route(ColumnRouter router);

  [[nodiscard]] WorkflowRoute route(std::vector<std::string> names);

  [[nodiscard]] WorkflowRoute route(std::initializer_list<std::string> names);

  [[nodiscard]] WorkflowRoute route(
      std::function<bool(std::string_view feature_name, std::string_view dtype)> predicate);

 private:
  Workflow* workflow_ = nullptr;
  std::string name_;
};

class workflow::WorkflowRoute final {
 public:
  WorkflowRoute(Workflow& workflow, ColumnRouter router, std::int64_t branch_index)
      : workflow_(&workflow), router_(std::move(router)), branch_index_(branch_index) {}

  WorkflowRoute& add(Stage stage) {
    if (workflow_ == nullptr) {
      ::pluss::throw_error("lymph::WorkflowRoute: invalid empty route accessor.");
    }
    workflow_->append_step_(branch_index_, std::move(stage), router_);
    return *this;
  }

 private:
  Workflow* workflow_ = nullptr;
  ColumnRouter router_;
  std::int64_t branch_index_ = -1;
};

[[nodiscard]] inline WorkflowRoute Workflow::open_branch_(std::string name, bool named, ColumnRouter router) {
  if (named && name.empty()) {
    ::pluss::throw_error("lymph::Workflow::on: named workflow branch cannot be empty.");
  }
  const std::int64_t branch_index = static_cast<std::int64_t>(branches_.size());
  branches_.push_back(WorkflowBranchInfo{std::move(name), named, 0});
  return WorkflowRoute(*this, std::move(router), branch_index);
}

[[nodiscard]] inline WorkflowBranch Workflow::branch(std::string name) {
  return WorkflowBranch(*this, std::move(name));
}

[[nodiscard]] inline WorkflowRoute Workflow::on(ColumnRouter router) {
  return open_branch_({}, false, std::move(router));
}

[[nodiscard]] inline WorkflowRoute Workflow::on(std::vector<std::string> names) {
  return on(columnrouter::names(std::move(names)));
}

[[nodiscard]] inline WorkflowRoute Workflow::on(std::initializer_list<std::string> names) {
  return on(std::vector<std::string>(names.begin(), names.end()));
}

[[nodiscard]] inline WorkflowRoute Workflow::on(std::string name, ColumnRouter router) {
  return open_branch_(std::move(name), true, std::move(router));
}

[[nodiscard]] inline WorkflowRoute Workflow::on(std::string name, std::vector<std::string> names) {
  return on(std::move(name), columnrouter::names(std::move(names)));
}

[[nodiscard]] inline WorkflowRoute Workflow::on(std::string name, std::initializer_list<std::string> names) {
  return on(std::move(name), std::vector<std::string>(names.begin(), names.end()));
}

[[nodiscard]] inline WorkflowRoute Workflow::on(
    std::function<bool(std::string_view feature_name, std::string_view dtype)> predicate) {
  return on(columnrouter::where(std::move(predicate)));
}

[[nodiscard]] inline WorkflowRoute Workflow::on(
    std::string name,
    std::function<bool(std::string_view feature_name, std::string_view dtype)> predicate) {
  return on(std::move(name), columnrouter::where(std::move(predicate)));
}

[[nodiscard]] inline WorkflowRoute WorkflowBranch::route(ColumnRouter router) {
  if (workflow_ == nullptr) {
    ::pluss::throw_error("lymph::WorkflowBranch: invalid empty branch accessor.");
  }
  return workflow_->open_branch_(name_, true, std::move(router));
}

[[nodiscard]] inline WorkflowRoute WorkflowBranch::route(std::vector<std::string> names) {
  return route(columnrouter::names(std::move(names)));
}

[[nodiscard]] inline WorkflowRoute WorkflowBranch::route(std::initializer_list<std::string> names) {
  return route(std::vector<std::string>(names.begin(), names.end()));
}

[[nodiscard]] inline WorkflowRoute WorkflowBranch::route(
    std::function<bool(std::string_view feature_name, std::string_view dtype)> predicate) {
  return route(columnrouter::where(std::move(predicate)));
}

namespace detail {

struct WorkflowLabeledInput {
  std::optional<pluss::table::DataFrame> frame{};
  DataFrameBatchOptions data_options{};
  std::optional<LabeledBatch> batch{};
};

[[nodiscard]] inline WorkflowLabeledInput workflow_labeled_input_(const Workflow& workflow,
                                                                  const Payload& payload,
                                                                  const char* where) {
  if (payload.has_frame()) {
    if (payload.has_external_weights() && !payload.has_weight_feature()) {
      ::pluss::throw_error(std::string(where) +
                           ": DataFrame-backed Payload with Series weights must use weights(\"column\") so Policy row filtering stays aligned.");
    }
    pluss::table::DataFrame frame = workflow.policy().apply(payload.frame());
    if (payload.has_label_feature()) {
      return WorkflowLabeledInput{
          .frame = std::move(frame),
          .data_options = payload.data(),
          .batch = std::nullopt,
      };
    }
    if (payload.has_label()) {
      require_workflow_xy_policy_safe_(workflow.policy(), where);
      return WorkflowLabeledInput{
          .frame = std::nullopt,
          .data_options = payload.data(),
          .batch = payload.with_frame(std::move(frame)).labeled_batch(),
      };
    }
  }
  return WorkflowLabeledInput{
      .frame = std::nullopt,
      .data_options = payload.data(),
      .batch = payload.labeled_batch(),
  };
}

} // namespace detail

inline void Workflow::save(const std::string& path,
                           const Payload& payload,
                           const TrainOptions& train_options,
                           io::FlowBundleMetadata metadata) const {
  const detail::WorkflowLabeledInput input = detail::workflow_labeled_input_(*
      this, payload, "lymph::Workflow::save(payload)");
  if (input.frame.has_value()) {
    model_.save(path, *input.frame, input.data_options, train_options, std::move(metadata));
  } else {
    model_.save(path, *input.batch, train_options, std::move(metadata));
  }
  save_policy(path, policy_);
}

namespace workflow {

[[nodiscard]] inline Workflow make(Policy policy) {
  return Workflow(Model(::pluss::ai::nodej::Workflow{}), std::move(policy));
}

[[nodiscard]] inline Workflow make(Model model, Policy policy) {
  return Workflow(std::move(model), std::move(policy));
}

namespace detail {

inline void apply_workflow_options_(Workflow& workflow, const WorkflowOptions& options) {
  workflow.workers(options.workers);
}

inline void apply_workflow_branch_(Workflow& workflow, const WorkflowBranchSpec& branch) {
  if (branch.name.empty()) {
    ::pluss::throw_error("lymph::workflow::make: workflow branch name cannot be empty.");
  }
  if (branch.columns.empty()) {
    ::pluss::throw_error("lymph::workflow::make: workflow branch \"" + branch.name + "\" has no columns.");
  }
  WorkflowRoute route = workflow.on(branch.name, branch.columns);
  for (const WorkflowStepSpec& step : branch.steps) {
    if (step.name.empty()) {
      ::pluss::throw_error("lymph::workflow::make: workflow branch \"" + branch.name +
                           "\" contains an unnamed step.");
    }
    route.add(step.stage);
  }
}

inline void apply_workflow_branches_(Workflow& workflow, const std::vector<WorkflowBranchSpec>& branches) {
  for (const WorkflowBranchSpec& branch : branches) {
    apply_workflow_branch_(workflow, branch);
  }
  if (!branches.empty()) {
    workflow.join();
  }
}

} // namespace detail

[[nodiscard]] inline WorkflowBranchSpec branch(std::string name,
                                               std::vector<std::string> columns,
                                               std::vector<WorkflowStepSpec> steps) {
  return WorkflowBranchSpec{
      .name = std::move(name),
      .columns = std::move(columns),
      .steps = std::move(steps),
  };
}

[[nodiscard]] inline WorkflowBranchSpec branch(std::string name,
                                               std::initializer_list<std::string> columns,
                                               std::initializer_list<WorkflowStepSpec> steps) {
  return branch(
      std::move(name),
      std::vector<std::string>(columns.begin(), columns.end()),
      std::vector<WorkflowStepSpec>(steps.begin(), steps.end()));
}

[[nodiscard]] inline Workflow make(Policy policy,
                                   std::vector<WorkflowBranchSpec> branches,
                                   WorkflowOptions options) {
  Workflow workflow = workflow::make(std::move(policy));
  detail::apply_workflow_options_(workflow, options);
  detail::apply_workflow_branches_(workflow, branches);
  return workflow;
}

[[nodiscard]] inline Workflow make(Policy policy,
                                   std::initializer_list<WorkflowBranchSpec> branches,
                                   WorkflowOptions options) {
  return make(
      std::move(policy),
      std::vector<WorkflowBranchSpec>(branches.begin(), branches.end()),
      options);
}

[[nodiscard]] inline Workflow make(Policy policy,
                                   std::vector<WorkflowBranchSpec> branches,
                                   Model model,
                                   WorkflowOptions options) {
  Workflow workflow = make(std::move(model), std::move(policy));
  detail::apply_workflow_options_(workflow, options);
  detail::apply_workflow_branches_(workflow, branches);
  return workflow;
}

[[nodiscard]] inline Workflow make(Policy policy,
                                   std::initializer_list<WorkflowBranchSpec> branches,
                                   Model model,
                                   WorkflowOptions options) {
  return make(
      std::move(policy),
      std::vector<WorkflowBranchSpec>(branches.begin(), branches.end()),
      std::move(model),
      options);
}

[[nodiscard]] inline Workflow make(std::vector<WorkflowBranchSpec> branches,
                                   Model model,
                                   WorkflowOptions options) {
  return make(Policy{}, std::move(branches), std::move(model), options);
}

[[nodiscard]] inline Workflow make(std::initializer_list<WorkflowBranchSpec> branches,
                                   Model model,
                                   WorkflowOptions options) {
  return make(
      std::vector<WorkflowBranchSpec>(branches.begin(), branches.end()),
      std::move(model),
      options);
}

[[nodiscard]] inline Workflow restore(const io::FlowBundle& bundle, Policy policy) {
  return Workflow(Model::restore(bundle), std::move(policy));
}

[[nodiscard]] inline Workflow load(const std::string& path) {
  return restore(resolve(io::load(path), "lymph::workflow::load"), load_policy(path));
}

} // namespace workflow

} // namespace pluss::ai::lymph
