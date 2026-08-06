#pragma once

#include "pluss/ai/lymph/workflow_model.h"

#include <fstream>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace pluss::ai::lymph {

// Declarative, replayable raw-frame cleanup: which columns never reach the
// model at all (ID/leakage columns) and which columns must be non-null for a
// row to survive. Both steps run on the raw DataFrame, before any
// FeatureBatch/LabeledBatch exists -- exactly where a hand-written
// `.drop(...).dropnull()` call used to live in caller code (e.g.
// loan_mod_lymp.cpp), except now the choice is data (savable, replayable by
// PreparedModel below) instead of source a caller without it can't reproduce.
struct FramePrepPolicy {
  std::vector<std::string> drop_columns{};
  bool filter_nulls = false;
  // Empty means "every remaining column" -- DataFrame::dropnull's own
  // default when no subset is given.
  std::vector<std::string> null_filter_columns{};
  // false (default) drops a row if ANY of null_filter_columns is null,
  // matching DataFrame::dropnull's own `all=false` default. true drops a row
  // only when ALL of them are null.
  bool drop_only_if_all_null = false;

  [[nodiscard]] bool empty() const noexcept { return drop_columns.empty() && !filter_nulls; }
};

[[nodiscard]] inline pluss::table::DataFrame apply_frame_prep(const pluss::table::DataFrame& frame,
                                                               const FramePrepPolicy& policy) {
  pluss::table::DataFrame out = frame;
  if (!policy.drop_columns.empty()) {
    out = out.drop(policy.drop_columns);
  }
  if (policy.filter_nulls) {
    out = out.dropnull(policy.null_filter_columns, policy.drop_only_if_all_null);
  }
  return out;
}

namespace detail {

[[nodiscard]] inline std::string frame_prep_sidecar_path_(const std::string& bundle_path) {
  return bundle_path + ".frameprep";
}

inline void write_name_list_(std::ofstream& out, const std::vector<std::string>& names) {
  out << names.size() << "\n";
  for (const auto& name : names) out << name << "\n";
}

[[nodiscard]] inline std::vector<std::string> read_name_list_(std::ifstream& in, const char* where) {
  std::size_t count = 0;
  if (!(in >> count)) {
    ::pluss::throw_error(std::string(where) + ": malformed frameprep sidecar (name count).");
  }
  in.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  std::vector<std::string> out{};
  out.reserve(count);
  for (std::size_t i = 0; i < count; ++i) {
    std::string line;
    if (!std::getline(in, line)) {
      ::pluss::throw_error(std::string(where) + ": malformed frameprep sidecar (expected " +
                           std::to_string(count) + " names).");
    }
    out.push_back(std::move(line));
  }
  return out;
}

} // namespace detail

// FlowBundle's binary schema (nodej/io.h) has no free-form metadata slot to
// piggyback arbitrary policy data on without a schema bump touching every
// existing saved bundle, so FramePrepPolicy gets its own small plain-text
// sidecar next to the bundle path instead -- simple, human-inspectable, and
// entirely independent of the model bundle's own format.
inline void save_frame_prep_policy(const std::string& bundle_path, const FramePrepPolicy& policy) {
  const std::string sidecar = detail::frame_prep_sidecar_path_(bundle_path);
  std::ofstream out(sidecar, std::ios::trunc);
  if (!out) {
    ::pluss::throw_error("save_frame_prep_policy: could not open \"" + sidecar + "\" for writing.");
  }
  detail::write_name_list_(out, policy.drop_columns);
  out << (policy.filter_nulls ? 1 : 0) << "\n";
  out << (policy.drop_only_if_all_null ? 1 : 0) << "\n";
  detail::write_name_list_(out, policy.null_filter_columns);
}

[[nodiscard]] inline FramePrepPolicy load_frame_prep_policy(const std::string& bundle_path) {
  const std::string sidecar = detail::frame_prep_sidecar_path_(bundle_path);
  std::ifstream in(sidecar);
  if (!in) {
    // No sidecar on disk -- a plain WorkflowModel bundle (or a PreparedModel
    // saved with an empty policy) never wrote one; treat as a legitimate
    // no-op policy rather than an error.
    return FramePrepPolicy{};
  }
  FramePrepPolicy policy{};
  policy.drop_columns = detail::read_name_list_(in, "load_frame_prep_policy");
  int filter_nulls = 0;
  int drop_only_if_all_null = 0;
  if (!(in >> filter_nulls) || !(in >> drop_only_if_all_null)) {
    ::pluss::throw_error("load_frame_prep_policy: malformed frameprep sidecar (flags).");
  }
  in.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  policy.filter_nulls = filter_nulls != 0;
  policy.drop_only_if_all_null = drop_only_if_all_null != 0;
  policy.null_filter_columns = detail::read_name_list_(in, "load_frame_prep_policy");
  return policy;
}

// The "single object, raw frame in, prediction out" production wrapper: owns
// a WorkflowModel plus the FramePrepPolicy that must run before that model
// ever sees a DataFrame, so a caller holding only the original raw schema
// (ID/leakage columns and nulls still present) can learn_/infer/assess/save
// straight against it, the same way WorkflowModel's own .target_transform(...)
// rescales a label instead of a cleaned frame. Deliberately raw-frame-only: once a
// caller already has separated x/y Series, the point of this class -- undoing
// column drops and null rows on the *combined* raw frame -- no longer
// applies, so there is no (x, y) overload here, unlike WorkflowModel's own.
class PreparedModel final {
 public:
  PreparedModel(WorkflowModel model, FramePrepPolicy policy)
      : model_(std::move(model)), policy_(std::move(policy)) {}

  PreparedModel& learn_(const pluss::table::DataFrame& raw_frame,
                        const DataFrameBatchOptions& data_options,
                        const TrainOptions& options = {}) {
    model_.learn_(apply_frame_prep(raw_frame, policy_), data_options, options);
    return *this;
  }

  [[nodiscard]] Series infer(const pluss::table::DataFrame& raw_frame, const InferOptions& options = {}) const {
    return model_.infer(apply_frame_prep(raw_frame, policy_), options);
  }

  [[nodiscard]] Series infer(const pluss::table::DataFrame& raw_frame,
                             const DataFrameBatchOptions& data_options,
                             const InferOptions& options = {}) const {
    return model_.infer(apply_frame_prep(raw_frame, policy_), data_options, options);
  }

  [[nodiscard]] MetricReport assess(const pluss::table::DataFrame& raw_frame,
                                    const DataFrameBatchOptions& data_options,
                                    const AssessOptions& options = {}) const {
    return model_.assess(apply_frame_prep(raw_frame, policy_), data_options, options);
  }

  void save(const std::string& path,
           const pluss::table::DataFrame& raw_train_frame,
           const DataFrameBatchOptions& data_options,
           const TrainOptions& train_options = {},
           io::FlowBundleMetadata metadata = {}) const {
    model_.save(path, apply_frame_prep(raw_train_frame, policy_), data_options, train_options, std::move(metadata));
    save_frame_prep_policy(path, policy_);
  }

  [[nodiscard]] static PreparedModel restore(const io::FlowBundle& bundle, FramePrepPolicy policy = {}) {
    return PreparedModel(WorkflowModel::restore(bundle), std::move(policy));
  }

  [[nodiscard]] static PreparedModel load(const std::string& path) {
    return PreparedModel(WorkflowModel::load(path), load_frame_prep_policy(path));
  }

  [[nodiscard]] const WorkflowModel& model() const noexcept { return model_; }
  [[nodiscard]] WorkflowModel& model() noexcept { return model_; }
  [[nodiscard]] const FramePrepPolicy& policy() const noexcept { return policy_; }

 private:
  WorkflowModel model_;
  FramePrepPolicy policy_;
};

} // namespace pluss::ai::lymph
