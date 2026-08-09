#pragma once

#include "pluss/ai/lymph/model.h"
#include "pluss/ai/lymph/policy.h"

#include <fstream>
#include <limits>
#include <string>
#include <utility>
#include <vector>

// workflow.h -- deployable raw-frame artifact.
//
// Role: the actual "stack everything, raw data in, prediction out"
// deployable object -- Model + Policy, save()/load() round-tripping both
// together via a small sidecar next to the model bundle. This is the object
// the very first question in this whole redesign thread ("the final stage
// for deployment is the pipeline that stacks everything") was asking about;
// the public name is Workflow per the redesign, not Pipeline.
//
// Implemented surface:
//   - class Workflow -- learn_/infer/assess/save, all raw-frame shaped,
//     each running Policy::apply(frame) before the held Model sees data.
//   - save_policy(path, Policy) / load_policy(path) -- sidecar persistence
//     helpers kept here because they are Workflow-only concerns.
//   - namespace lymph::workflow { Workflow from(Model, Policy); Workflow
//     restore(FlowBundle, Policy); Workflow load(std::string path); }
//
// Depends on: model.h (Model), policy.h (Policy), NodeJ's io::save/
// io::load/FlowBundle. The sidecar file is intentionally outside
// FlowBundle's own schema.
//
// Consumed by: nothing -- this is the end of the lifecycle.

namespace pluss::ai::lymph {

namespace detail {

[[nodiscard]] inline std::string workflow_policy_sidecar_path_(const std::string& bundle_path) {
  return bundle_path + ".policy";
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
  detail::write_policy_name_list_(out, policy.drop_columns);
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
  policy.drop_columns = detail::read_policy_name_list_(in, "lymph::load_policy");
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

class Workflow;

namespace workflow {
[[nodiscard]] Workflow from(Model model, Policy policy = {});
[[nodiscard]] Workflow restore(const io::FlowBundle& bundle, Policy policy = {});
[[nodiscard]] Workflow load(const std::string& path);
} // namespace workflow

class Workflow final {
 public:
  Workflow(Workflow&&) noexcept = default;
  Workflow& operator=(Workflow&&) noexcept = default;
  Workflow(const Workflow&) = delete;
  Workflow& operator=(const Workflow&) = delete;

  Workflow& learn_(const pluss::table::DataFrame& raw_frame,
                   const DataFrameBatchOptions& data_options,
                   const TrainOptions& options = {}) {
    model_.learn_(policy_.apply(raw_frame), data_options, options);
    return *this;
  }

  [[nodiscard]] Series infer(const pluss::table::DataFrame& raw_frame, const InferOptions& options = {}) const {
    return model_.infer(policy_.apply(raw_frame), options);
  }

  [[nodiscard]] Series infer(const pluss::table::DataFrame& raw_frame,
                             const DataFrameBatchOptions& data_options,
                             const InferOptions& options = {}) const {
    return model_.infer(policy_.apply(raw_frame), data_options, options);
  }

  [[nodiscard]] MetricReport assess(const pluss::table::DataFrame& raw_frame,
                                    const DataFrameBatchOptions& data_options,
                                    const AssessOptions& options = {}) const {
    return model_.assess(policy_.apply(raw_frame), data_options, options);
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

  [[nodiscard]] const Model& model() const noexcept { return model_; }
  [[nodiscard]] Model& model() noexcept { return model_; }
  [[nodiscard]] const Policy& policy() const noexcept { return policy_; }

 private:
  friend Workflow workflow::from(Model model, Policy policy);
  friend Workflow workflow::restore(const io::FlowBundle& bundle, Policy policy);

  Workflow(Model model, Policy policy)
      : model_(std::move(model)), policy_(std::move(policy)) {}

  Model model_;
  Policy policy_;
};

namespace workflow {

[[nodiscard]] inline Workflow from(Model model, Policy policy) {
  return Workflow(std::move(model), std::move(policy));
}

[[nodiscard]] inline Workflow restore(const io::FlowBundle& bundle, Policy policy) {
  return Workflow(Model::restore(bundle), std::move(policy));
}

[[nodiscard]] inline Workflow load(const std::string& path) {
  return restore(resolve(io::load(path), "lymph::workflow::load"), load_policy(path));
}

} // namespace workflow

} // namespace pluss::ai::lymph
