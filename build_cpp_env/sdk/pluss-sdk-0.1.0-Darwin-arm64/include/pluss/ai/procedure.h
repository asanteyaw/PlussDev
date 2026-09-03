#pragma once

#include "pluss/io.h"
#include "pluss/ai/ice.h"
#include "pluss/ai/lymph/workflow.h"
#include "pluss/ai/nodeb/workflow.h"

#include <algorithm>
#include <chrono>
#include <concepts>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <future>
#include <functional>
#include <initializer_list>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>
#include <type_traits>

namespace pluss::ai {

enum class ProcedureSchedule : std::uint8_t {
  Deterministic,
  Adaptive,
};

enum class ProcedureCache : std::uint8_t {
  Off,
  ReadyArtifacts,
};

enum class ProcedureMode : std::uint8_t {
  Infer,
  Learn,
  Assess,
};

enum class StepCapability : std::uint16_t {
  None = 0,
  Admit = 1U << 0U,
  Map = 1U << 1U,
  Learn = 1U << 2U,
  Infer = 1U << 3U,
  Assess = 1U << 4U,
  Gate = 1U << 5U,
  Join = 1U << 6U,
  Sink = 1U << 7U,
  Watch = 1U << 8U,
};

[[nodiscard]] constexpr std::uint16_t capability_bit(StepCapability capability) noexcept {
  return static_cast<std::uint16_t>(capability);
}

struct ProcedureOptions {
  std::int64_t workers = 1;
  ProcedureSchedule schedule = ProcedureSchedule::Deterministic;
  ProcedureCache cache = ProcedureCache::Off;
  bool fail_fast = true;
  bool validate_on_make = true;
};

struct RunOptions {
  ProcedureMode mode = ProcedureMode::Infer;
  bool audit = true;
  bool allow_temporal_leakage = false;
};

struct StepOptions {
  std::vector<StepCapability> capabilities{StepCapability::Map};
  std::vector<std::string> inputs{};
  std::vector<std::string> outputs{};
  std::vector<std::string> require_columns{};
  std::string time_feature{};
  bool persistable = false;
  bool trained = false;
  bool can_change_row_count = false;
  bool requires_time = false;
  bool learns_from_future = false;
  std::string component{};
};

struct StepPersistSpec {
  using Saver = std::function<void(std::string_view)>;

  std::string key{};
  std::int64_t schema_version = 1;
  std::int64_t component_schema_version = 0;
  bool reloadable = false;
  bool frozen = false;
  bool custom = false;
  std::vector<std::pair<std::string, std::string>> fields{};
  Saver save{};
};

struct SinkSpec {
  std::string target{};
};

struct ArtifactTrace {
  std::string name{};
  std::string type{};
  std::int64_t rows = -1;
  std::int64_t columns = -1;
  std::int64_t elements = -1;
  bool defined = true;
};

struct StepTrace {
  std::string step{};
  std::string component{};
  std::vector<ArtifactTrace> inputs{};
  ArtifactTrace output{};
  std::int64_t elapsed_us = 0;
  bool row_count_changed = false;
};

class Artifact final {
 public:
  using Value = std::variant<std::monostate,
                             ::pluss::Series,
                             pluss::table::DataFrame,
                             pluss::Note,
                             pluss::table::Thread,
                             lymph::FeatureBatch,
                             lymph::LabeledBatch,
                             lymph::workflow::Payload,
                             lymph::models::Model,
                             lymph::workflow::Workflow,
                             lymph::MetricReport,
                             ice::Batch,
                             ice::AssessReport,
                             nodeb::Doc,
                             std::vector<nodeb::Doc>,
                             nodeb::Workflow,
                             nodeb::WorkflowReport,
                             std::string,
                             std::vector<std::string>,
                             double,
                             std::int64_t,
                             bool>;

  Artifact() = default;
  explicit Artifact(std::string name) : name_(std::move(name)) {}
  Artifact(std::string name, Value value) : name_(std::move(name)), value_(std::move(value)) {}

  [[nodiscard]] const std::string& name() const noexcept { return name_; }
  [[nodiscard]] bool named() const noexcept { return !name_.empty(); }
  [[nodiscard]] bool empty() const noexcept { return std::holds_alternative<std::monostate>(value_); }
  [[nodiscard]] const Value& value() const noexcept { return value_; }
  [[nodiscard]] Value& value() noexcept { return value_; }

  template <class T>
  [[nodiscard]] bool is() const noexcept {
    return std::holds_alternative<T>(value_);
  }

  template <class T>
  [[nodiscard]] const T& get() const {
    if (!std::holds_alternative<T>(value_)) {
      ::pluss::throw_error("pluss::ai::Artifact::get: artifact \"" + name_ + "\" does not hold the requested type.");
    }
    return std::get<T>(value_);
  }

 private:
  std::string name_{};
  Value value_{};
};

class Step final {
 public:
  using Runner = std::function<Artifact(const std::vector<Artifact>&, const RunOptions&)>;

  Step() = default;
  Step(std::string name, StepOptions options)
      : name_(std::move(name)), options_(std::move(options)) {}
  Step(std::string name, StepOptions options, Runner runner)
      : name_(std::move(name)), options_(std::move(options)), runner_(std::move(runner)) {}
  Step(std::string name, StepOptions options, StepPersistSpec persist, Runner runner)
      : name_(std::move(name)),
        options_(std::move(options)),
        runner_(std::move(runner)),
        persist_(std::move(persist)) {}

  [[nodiscard]] const std::string& name() const noexcept { return name_; }
  [[nodiscard]] const StepOptions& options() const noexcept { return options_; }
  [[nodiscard]] StepOptions& options() noexcept { return options_; }
  [[nodiscard]] const std::vector<std::string>& inputs() const noexcept { return options_.inputs; }
  [[nodiscard]] const std::vector<std::string>& outputs() const noexcept { return options_.outputs; }
  [[nodiscard]] bool persistable() const noexcept { return options_.persistable; }
  [[nodiscard]] bool trained() const noexcept { return options_.trained; }
  [[nodiscard]] bool can_change_row_count() const noexcept { return options_.can_change_row_count; }
  [[nodiscard]] const std::string& component() const noexcept { return options_.component; }
  [[nodiscard]] bool runnable() const noexcept { return static_cast<bool>(runner_); }
  [[nodiscard]] const StepPersistSpec& persist() const noexcept { return persist_; }
  [[nodiscard]] bool reloadable() const noexcept { return persist_.reloadable; }
  [[nodiscard]] bool frozen() const noexcept { return persist_.frozen; }
  [[nodiscard]] bool custom() const noexcept { return persist_.custom; }
  [[nodiscard]] const std::string& persist_key() const noexcept { return persist_.key; }
  [[nodiscard]] std::int64_t persist_schema_version() const noexcept { return persist_.schema_version; }
  [[nodiscard]] std::int64_t component_schema_version() const noexcept { return persist_.component_schema_version; }
  [[nodiscard]] const std::vector<std::pair<std::string, std::string>>& persist_fields() const noexcept {
    return persist_.fields;
  }
  [[nodiscard]] bool can_save_component() const noexcept { return static_cast<bool>(persist_.save); }

  void save_component(std::string_view path) const {
    if (persist_.save) {
      persist_.save(path);
    }
  }

  [[nodiscard]] std::uint16_t capability_mask() const noexcept {
    std::uint16_t mask = 0;
    for (const StepCapability capability : options_.capabilities) {
      mask = static_cast<std::uint16_t>(mask | capability_bit(capability));
    }
    return mask;
  }

  [[nodiscard]] bool has(StepCapability capability) const noexcept {
    return (capability_mask() & capability_bit(capability)) != 0;
  }

  [[nodiscard]] Artifact run(const std::vector<Artifact>& inputs, const RunOptions& options) const {
    if (runner_) return runner_(inputs, options);
    if (has(StepCapability::Sink)) {
      if (inputs.empty()) return Artifact(name_);
      return Artifact(name_, inputs.front().value());
    }
    if (inputs.size() == 1) {
      return Artifact(default_output_name_(), inputs.front().value());
    }
    return Artifact(default_output_name_());
  }

 private:
  [[nodiscard]] std::string default_output_name_() const {
    if (options_.outputs.size() == 1) return options_.outputs.front();
    return name_;
  }

  std::string name_{};
  StepOptions options_{};
  Runner runner_{};
  StepPersistSpec persist_{};
};

struct Link {
  std::string from{};
  std::string to{};
  std::string from_output{};
  std::string to_input{};
};

class ProcedureResult final {
 public:
  ProcedureResult() = default;
  ProcedureResult(std::vector<Artifact> artifacts,
                  std::vector<Artifact> metrics,
                  std::vector<std::string> trace,
                  std::vector<StepTrace> audit,
                  std::vector<std::string> warnings,
                  std::string failed_step)
      : artifacts_(std::move(artifacts)),
        metrics_(std::move(metrics)),
        trace_(std::move(trace)),
        audit_(std::move(audit)),
        warnings_(std::move(warnings)),
        failed_step_(std::move(failed_step)) {}

  [[nodiscard]] bool ok() const noexcept { return failed_step_.empty(); }
  [[nodiscard]] const std::string& failed_step() const noexcept { return failed_step_; }
  [[nodiscard]] const std::vector<std::string>& trace() const noexcept { return trace_; }
  [[nodiscard]] const std::vector<StepTrace>& audit() const noexcept { return audit_; }
  [[nodiscard]] const std::vector<std::string>& warnings() const noexcept { return warnings_; }
  [[nodiscard]] const std::vector<Artifact>& artifacts() const noexcept { return artifacts_; }
  [[nodiscard]] const std::vector<Artifact>& metrics() const noexcept { return metrics_; }

  template <class T>
  [[nodiscard]] const T& get(std::string_view name) const {
    for (const Artifact& artifact : artifacts_) {
      if (artifact.name() == name) return artifact.get<T>();
    }
    ::pluss::throw_error("pluss::ai::ProcedureResult::get: no artifact named \"" + std::string(name) + "\".");
  }

 private:
  std::vector<Artifact> artifacts_{};
  std::vector<Artifact> metrics_{};
  std::vector<std::string> trace_{};
  std::vector<StepTrace> audit_{};
  std::vector<std::string> warnings_{};
  std::string failed_step_{};
};

class Procedure final {
 public:
  Procedure() = default;
  Procedure(std::vector<Step> steps, std::vector<Link> links, ProcedureOptions options = {})
      : steps_(std::move(steps)), links_(std::move(links)), options_(options) {
    if (options_.validate_on_make) check();
  }

  [[nodiscard]] const std::vector<Step>& steps() const noexcept { return steps_; }
  [[nodiscard]] const std::vector<Link>& links() const noexcept { return links_; }
  [[nodiscard]] const ProcedureOptions& options() const noexcept { return options_; }
  [[nodiscard]] std::int64_t step_count() const noexcept { return static_cast<std::int64_t>(steps_.size()); }
  [[nodiscard]] std::int64_t link_count() const noexcept { return static_cast<std::int64_t>(links_.size()); }
  [[nodiscard]] bool empty() const noexcept { return steps_.empty(); }
  void save(std::string_view path) const;

  [[nodiscard]] std::vector<std::string> names() const {
    std::vector<std::string> out{};
    out.reserve(steps_.size());
    for (const Step& step : steps_) out.push_back(step.name());
    return out;
  }

  [[nodiscard]] bool has(std::string_view name) const noexcept {
    for (const Step& step : steps_) {
      if (step.name() == name) return true;
    }
    return false;
  }

  [[nodiscard]] const Step& get(std::string_view name) const {
    for (const Step& step : steps_) {
      if (step.name() == name) return step;
    }
    ::pluss::throw_error("pluss::ai::Procedure::get: no step named \"" + std::string(name) + "\".");
  }

  [[nodiscard]] std::optional<std::string> check_message() const {
    if (options_.workers < 1) {
      return "pluss::ai::Procedure::check: worker count must be at least 1.";
    }
    if (steps_.empty()) {
      return "pluss::ai::Procedure::check: procedure must contain at least one step.";
    }

    std::unordered_set<std::string> names_seen{};
    names_seen.reserve(steps_.size());
    for (const Step& step : steps_) {
      if (step.name().empty()) {
        return "pluss::ai::Procedure::check: step names must not be empty.";
      }
      if (!names_seen.insert(step.name()).second) {
        return "pluss::ai::Procedure::check: duplicate step name \"" + step.name() + "\".";
      }
      if (step.capability_mask() == 0) {
        return "pluss::ai::Procedure::check: step \"" + step.name() +
               "\" must declare at least one capability.";
      }
      if (const std::optional<std::string> issue = detail_unique_names_issue_(step.inputs(), "input", step.name())) {
        return issue;
      }
      if (const std::optional<std::string> issue = detail_unique_names_issue_(step.outputs(), "output", step.name())) {
        return issue;
      }
    }

    std::unordered_map<std::string, std::int64_t> indegree{};
    std::unordered_map<std::string, std::vector<std::string>> adjacency{};
    indegree.reserve(steps_.size());
    adjacency.reserve(steps_.size());
    for (const Step& step : steps_) {
      indegree.emplace(step.name(), 0);
      adjacency.emplace(step.name(), std::vector<std::string>{});
    }

    std::unordered_set<std::string> links_seen{};
    links_seen.reserve(links_.size());
    for (const Link& link : links_) {
      if (link.from.empty() || link.to.empty()) {
        return "pluss::ai::Procedure::check: links must declare non-empty endpoints.";
      }
      if (!names_seen.contains(link.from)) {
        return "pluss::ai::Procedure::check: link source step \"" + link.from + "\" does not exist.";
      }
      if (!names_seen.contains(link.to)) {
        return "pluss::ai::Procedure::check: link target step \"" + link.to + "\" does not exist.";
      }
      const Step* source = find_step_(link.from);
      const Step* target = find_step_(link.to);
      if (source != nullptr && !link.from_output.empty() && !source->outputs().empty() &&
          !detail_contains_name_(source->outputs(), link.from_output)) {
        return "pluss::ai::Procedure::check: link source output \"" + link.from_output +
               "\" is not declared by step \"" + link.from + "\".";
      }
      if (target != nullptr && !link.to_input.empty() && !target->inputs().empty() &&
          !detail_contains_name_(target->inputs(), link.to_input)) {
        return "pluss::ai::Procedure::check: link target input \"" + link.to_input +
               "\" is not declared by step \"" + link.to + "\".";
      }
      const std::string key = link.from + "\n" + link.from_output + "\n" + link.to + "\n" + link.to_input;
      if (!links_seen.insert(key).second) {
        return "pluss::ai::Procedure::check: duplicate link from \"" + link.from +
               "\" to \"" + link.to + "\".";
      }
      adjacency[link.from].push_back(link.to);
      ++indegree[link.to];
    }

    std::vector<std::string> ready{};
    ready.reserve(steps_.size());
    for (const auto& [name, degree] : indegree) {
      if (degree == 0) ready.push_back(name);
    }

    std::int64_t visited = 0;
    while (!ready.empty()) {
      std::string current = std::move(ready.back());
      ready.pop_back();
      ++visited;
      for (const std::string& next : adjacency[current]) {
        auto found = indegree.find(next);
        if (found != indegree.end() && --found->second == 0) ready.push_back(next);
      }
    }

    if (visited != static_cast<std::int64_t>(steps_.size())) {
      return "pluss::ai::Procedure::check: procedure graph contains a cycle.";
    }
    return std::nullopt;
  }

  void check() const {
    if (const std::optional<std::string> issue = check_message()) {
      ::pluss::throw_error(*issue);
    }
  }

  [[nodiscard]] ProcedureResult run(std::vector<Artifact> inputs = {}, RunOptions options = {}) const {
    check();
    if (options_.workers <= 1) return run_serial_(std::move(inputs), options);
    return run_parallel_(std::move(inputs), options);
  }

  [[nodiscard]] ProcedureResult run(std::initializer_list<Artifact> inputs, RunOptions options = {}) const {
    return run(std::vector<Artifact>(inputs.begin(), inputs.end()), options);
  }

 private:
  [[nodiscard]] ProcedureResult run_serial_(std::vector<Artifact> inputs, RunOptions options) const {
    std::unordered_map<std::string, Artifact> artifacts{};
    std::vector<std::string> artifact_order{};

    for (Artifact& artifact : inputs) {
      put_artifact_(artifacts, artifact_order, std::move(artifact));
    }

    std::vector<std::string> trace{};
    std::vector<StepTrace> audit{};
    std::vector<std::string> warnings{};
    const std::vector<std::string> order = topological_order_();
    for (const std::string& step_name : order) {
      const Step& step = get(step_name);
      try {
        std::vector<Artifact> step_inputs = resolve_step_inputs_(step, artifacts);
        enforce_step_contract_(step, step_inputs, options);
        const auto start = std::chrono::steady_clock::now();
        Artifact output = step.run(step_inputs, options);
        const auto stop = std::chrono::steady_clock::now();
        if (!output.named()) output = Artifact(default_step_output_name_(step), output.value());
        StepTrace record = make_step_trace_(step, step_inputs, output, start, stop);
        enforce_output_contract_(step, record);
        put_artifact_(artifacts, artifact_order, std::move(output));
        trace.push_back("ran:" + step.name());
        if (options.audit) audit.push_back(std::move(record));
      } catch (const std::exception& e) {
        if (options_.fail_fast) throw;
        warnings.push_back(e.what());
        return make_result_(artifacts, artifact_order, std::move(trace), std::move(audit), std::move(warnings), step.name());
      }
    }

    return make_result_(artifacts, artifact_order, std::move(trace), std::move(audit), std::move(warnings), {});
  }

  [[nodiscard]] ProcedureResult run_parallel_(std::vector<Artifact> inputs, RunOptions options) const {
    std::unordered_map<std::string, Artifact> artifacts{};
    std::vector<std::string> artifact_order{};

    for (Artifact& artifact : inputs) {
      put_artifact_(artifacts, artifact_order, std::move(artifact));
    }

    std::vector<std::string> trace{};
    std::vector<StepTrace> audit{};
    std::vector<std::string> warnings{};
    const std::vector<std::vector<std::string>> waves = execution_waves_();
    const std::size_t worker_limit = static_cast<std::size_t>(std::max<std::int64_t>(1, options_.workers));
    for (const std::vector<std::string>& wave : waves) {
      for (std::size_t offset = 0; offset < wave.size(); offset += worker_limit) {
        const std::size_t end = std::min(wave.size(), offset + worker_limit);
        std::vector<std::future<Artifact>> futures{};
        futures.reserve(end - offset);
        std::vector<std::string> step_names{};
        step_names.reserve(end - offset);
        std::vector<std::vector<Artifact>> launched_inputs{};
        launched_inputs.reserve(end - offset);
        for (std::size_t i = offset; i < end; ++i) {
          const Step& step = get(wave[i]);
          const Step* step_ptr = &step;
          std::vector<Artifact> step_inputs = resolve_step_inputs_(step, artifacts);
          try {
            enforce_step_contract_(step, step_inputs, options);
          } catch (const std::exception& e) {
            if (options_.fail_fast) throw;
            warnings.push_back(e.what());
            return make_result_(artifacts, artifact_order, std::move(trace), std::move(audit), std::move(warnings), step.name());
          }
          step_names.push_back(step.name());
          launched_inputs.push_back(step_inputs);
          futures.push_back(std::async(std::launch::async,
                                       [this, step_ptr, step_inputs = std::move(step_inputs), options]() mutable {
                                         Artifact output = step_ptr->run(step_inputs, options);
                                         if (!output.named()) {
                                           output = Artifact(default_step_output_name_(*step_ptr), output.value());
                                         }
                                         return output;
                                       }));
        }
        for (std::size_t i = 0; i < futures.size(); ++i) {
          Artifact output;
          try {
            output = futures[i].get();
          } catch (const std::exception& e) {
            if (options_.fail_fast) throw;
            warnings.push_back(e.what());
            return make_result_(artifacts, artifact_order, std::move(trace), std::move(audit), std::move(warnings),
                                step_names[i]);
          }
          const Step& step = get(step_names[i]);
          const auto stamp = std::chrono::steady_clock::now();
          StepTrace record = make_step_trace_(step, launched_inputs[i], output, stamp, stamp);
          try {
            enforce_output_contract_(step, record);
          } catch (const std::exception& e) {
            if (options_.fail_fast) throw;
            warnings.push_back(e.what());
            return make_result_(artifacts, artifact_order, std::move(trace), std::move(audit), std::move(warnings),
                                step_names[i]);
          }
          put_artifact_(artifacts, artifact_order, std::move(output));
          trace.push_back("ran:" + step_names[i]);
          if (options.audit) audit.push_back(std::move(record));
        }
      }
    }

    return make_result_(artifacts, artifact_order, std::move(trace), std::move(audit), std::move(warnings), {});
  }

  [[nodiscard]] static ProcedureResult make_result_(const std::unordered_map<std::string, Artifact>& artifacts,
                                                    const std::vector<std::string>& artifact_order,
                                                    std::vector<std::string> trace,
                                                    std::vector<StepTrace> audit,
                                                    std::vector<std::string> warnings,
                                                    std::string failed_step) {
    std::vector<Artifact> ordered{};
    ordered.reserve(artifact_order.size());
    for (const std::string& name : artifact_order) {
      ordered.push_back(artifacts.at(name));
    }
    return ProcedureResult(std::move(ordered), {}, std::move(trace), std::move(audit), std::move(warnings),
                           std::move(failed_step));
  }

  [[nodiscard]] static std::string artifact_type_(const Artifact& artifact) {
    if (artifact.is<::pluss::Series>()) return "Series";
    if (artifact.is<pluss::table::DataFrame>()) return "DataFrame";
    if (artifact.is<pluss::Note>()) return "Note";
    if (artifact.is<pluss::table::Thread>()) return "Thread";
    if (artifact.is<lymph::FeatureBatch>()) return "lymph::FeatureBatch";
    if (artifact.is<lymph::LabeledBatch>()) return "lymph::LabeledBatch";
    if (artifact.is<lymph::workflow::Payload>()) return "lymph::workflow::Payload";
    if (artifact.is<lymph::models::Model>()) return "lymph::models::Model";
    if (artifact.is<lymph::workflow::Workflow>()) return "lymph::workflow::Workflow";
    if (artifact.is<lymph::MetricReport>()) return "lymph::MetricReport";
    if (artifact.is<ice::Batch>()) return "ice::Batch";
    if (artifact.is<ice::AssessReport>()) return "ice::AssessReport";
    if (artifact.is<nodeb::Doc>()) return "nodeb::Doc";
    if (artifact.is<std::vector<nodeb::Doc>>()) return "std::vector<nodeb::Doc>";
    if (artifact.is<nodeb::Workflow>()) return "nodeb::Workflow";
    if (artifact.is<nodeb::WorkflowReport>()) return "nodeb::WorkflowReport";
    if (artifact.is<std::string>()) return "std::string";
    if (artifact.is<std::vector<std::string>>()) return "std::vector<std::string>";
    if (artifact.is<double>()) return "double";
    if (artifact.is<std::int64_t>()) return "int64";
    if (artifact.is<bool>()) return "bool";
    return "empty";
  }

  [[nodiscard]] static ArtifactTrace artifact_trace_(const Artifact& artifact) {
    ArtifactTrace out{};
    out.name = artifact.name();
    out.type = artifact_type_(artifact);
    out.defined = !artifact.empty();
    if (artifact.is<::pluss::Series>()) {
      const Series& x = artifact.get<::pluss::Series>();
      out.defined = x.defined();
      out.elements = x.defined() ? x.numel() : 0;
      if (x.defined() && x.dim() > 0) out.rows = x.sizes().front();
      if (x.defined() && x.dim() > 1) out.columns = x.sizes().back();
    } else if (artifact.is<pluss::table::DataFrame>()) {
      const pluss::table::DataFrame& frame = artifact.get<pluss::table::DataFrame>();
      out.defined = frame.defined();
      out.rows = frame.defined() ? frame.ncases() : 0;
      out.columns = frame.defined() ? frame.nfeatures() : 0;
      out.elements = out.rows >= 0 && out.columns >= 0 ? out.rows * out.columns : -1;
    } else if (artifact.is<pluss::table::Thread>()) {
      const pluss::table::Thread& thread = artifact.get<pluss::table::Thread>();
      out.rows = thread.ncases();
      out.elements = thread.size();
    } else if (artifact.is<lymph::FeatureBatch>()) {
      const lymph::FeatureBatch& batch = artifact.get<lymph::FeatureBatch>();
      out.defined = batch.defined();
      out.rows = batch.sample_count();
      out.columns = batch.feature_count();
      out.elements = batch.x.defined() ? batch.x.numel() : 0;
    } else if (artifact.is<lymph::LabeledBatch>()) {
      const lymph::LabeledBatch& batch = artifact.get<lymph::LabeledBatch>();
      out.defined = batch.features.defined();
      out.rows = batch.features.sample_count();
      out.columns = batch.features.feature_count();
      out.elements = batch.features.x.defined() ? batch.features.x.numel() : 0;
    } else if (artifact.is<lymph::workflow::Payload>()) {
      const lymph::workflow::Payload& payload = artifact.get<lymph::workflow::Payload>();
      if (payload.has_frame()) {
        out.rows = payload.frame().ncases();
        out.columns = payload.frame().nfeatures();
        out.elements = out.rows * out.columns;
      } else if (payload.has_features()) {
        out.rows = payload.features().sample_count();
        out.columns = payload.features().feature_count();
        out.elements = payload.features().x.defined() ? payload.features().x.numel() : 0;
      } else if (payload.has_labeled()) {
        out.rows = payload.labeled().features.sample_count();
        out.columns = payload.labeled().features.feature_count();
        out.elements = payload.labeled().features.x.defined() ? payload.labeled().features.x.numel() : 0;
      }
    } else if (artifact.is<ice::Batch>()) {
      const ice::Batch& batch = artifact.get<ice::Batch>();
      if (batch.has_inputs()) {
        out.rows = batch.inputs().defined() && batch.inputs().dim() > 0 ? batch.inputs().sizes().front() : 0;
        out.elements = batch.inputs().defined() ? batch.inputs().numel() : 0;
      }
    } else if (artifact.is<std::vector<nodeb::Doc>>()) {
      out.rows = static_cast<std::int64_t>(artifact.get<std::vector<nodeb::Doc>>().size());
      out.elements = out.rows;
    } else if (artifact.is<std::vector<std::string>>()) {
      out.rows = static_cast<std::int64_t>(artifact.get<std::vector<std::string>>().size());
      out.elements = out.rows;
    } else if (!artifact.empty()) {
      out.elements = 1;
    }
    return out;
  }

  [[nodiscard]] static StepTrace make_step_trace_(const Step& step,
                                                  const std::vector<Artifact>& inputs,
                                                  const Artifact& output,
                                                  std::chrono::steady_clock::time_point start,
                                                  std::chrono::steady_clock::time_point stop) {
    StepTrace out{};
    out.step = step.name();
    out.component = step.component();
    out.inputs.reserve(inputs.size());
    for (const Artifact& input : inputs) out.inputs.push_back(artifact_trace_(input));
    out.output = artifact_trace_(output);
    out.elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count();
    for (const ArtifactTrace& input : out.inputs) {
      if (input.rows >= 0 && out.output.rows >= 0 && input.rows != out.output.rows) {
        out.row_count_changed = true;
        break;
      }
    }
    return out;
  }

  static void enforce_output_contract_(const Step& step, const StepTrace& record) {
    if (record.row_count_changed && !step.can_change_row_count()) {
      ::pluss::throw_error("pluss::ai::Procedure::run: step \"" + step.name() +
                           "\" changed row count without declaring can_change_row_count.");
    }
  }

  static void enforce_step_contract_(const Step& step,
                                     const std::vector<Artifact>& inputs,
                                     const RunOptions& options) {
    if (step.options().learns_from_future &&
        options.mode != ProcedureMode::Infer &&
        !options.allow_temporal_leakage) {
      ::pluss::throw_error("pluss::ai::Procedure::run: step \"" + step.name() +
                           "\" declares future-data access; set RunOptions::allow_temporal_leakage only for audited backtests.");
    }
    if (!step.options().require_columns.empty()) {
      enforce_required_columns_(step, inputs);
    }
    if (step.options().requires_time || !step.options().time_feature.empty()) {
      enforce_time_contract_(step, inputs);
    }
  }

  static void enforce_required_columns_(const Step& step, const std::vector<Artifact>& inputs) {
    bool saw_schema_carrier = false;
    for (const Artifact& input : inputs) {
      if (input.is<pluss::table::DataFrame>()) {
        saw_schema_carrier = true;
        require_columns_in_frame_(step.name(), input.get<pluss::table::DataFrame>(), step.options().require_columns);
      } else if (input.is<lymph::workflow::Payload>()) {
        const lymph::workflow::Payload& payload = input.get<lymph::workflow::Payload>();
        if (payload.has_frame()) {
          saw_schema_carrier = true;
          require_columns_in_frame_(step.name(), payload.frame(), step.options().require_columns);
        }
      }
    }
    if (!saw_schema_carrier) {
      ::pluss::throw_error("pluss::ai::Procedure::run: step \"" + step.name() +
                           "\" declares required columns but received no DataFrame/Payload frame input.");
    }
  }

  static void require_columns_in_frame_(const std::string& step_name,
                                        const pluss::table::DataFrame& frame,
                                        const std::vector<std::string>& columns) {
    if (!frame.defined()) {
      ::pluss::throw_error("pluss::ai::Procedure::run: step \"" + step_name +
                           "\" requires columns but received an undefined DataFrame.");
    }
    for (const std::string& column : columns) {
      if (column.empty()) {
        ::pluss::throw_error("pluss::ai::Procedure::run: step \"" + step_name +
                             "\" declares an empty required column name.");
      }
      if (!frame.get(column).has_value()) {
        ::pluss::throw_error("pluss::ai::Procedure::run: step \"" + step_name +
                             "\" requires missing column \"" + column + "\".");
      }
    }
  }

  static void enforce_time_contract_(const Step& step, const std::vector<Artifact>& inputs) {
    bool has_time = false;
    bool saw_schema_carrier = false;
    for (const Artifact& input : inputs) {
      if (input.is<lymph::workflow::Payload>()) {
        const lymph::workflow::Payload& payload = input.get<lymph::workflow::Payload>();
        if (payload.has_time()) has_time = true;
        if (payload.has_frame()) {
          saw_schema_carrier = true;
          if (!step.options().time_feature.empty() && payload.frame().get(step.options().time_feature).has_value()) {
            has_time = true;
          }
        }
      } else if (input.is<pluss::table::DataFrame>()) {
        saw_schema_carrier = true;
        if (!step.options().time_feature.empty() &&
            input.get<pluss::table::DataFrame>().get(step.options().time_feature).has_value()) {
          has_time = true;
        }
      }
    }
    if (!has_time) {
      std::string detail = step.options().time_feature.empty()
                               ? "a Payload with a time role"
                               : "time feature \"" + step.options().time_feature + "\"";
      if (!saw_schema_carrier && !step.options().time_feature.empty()) {
        detail += " on a DataFrame/Payload frame";
      }
      ::pluss::throw_error("pluss::ai::Procedure::run: step \"" + step.name() + "\" requires " + detail + ".");
    }
  }

  static void put_artifact_(std::unordered_map<std::string, Artifact>& artifacts,
                            std::vector<std::string>& artifact_order,
                            Artifact artifact) {
    if (!artifact.named()) {
      ::pluss::throw_error("pluss::ai::Procedure::run: artifacts must be named.");
    }
    const std::string name = artifact.name();
    if (!artifacts.contains(name)) artifact_order.push_back(name);
    artifacts[name] = std::move(artifact);
  }

  [[nodiscard]] static std::optional<std::string> detail_unique_names_issue_(
      const std::vector<std::string>& names,
      const char* role,
      const std::string& step_name) {
    std::unordered_set<std::string> seen{};
    seen.reserve(names.size());
    for (const std::string& name : names) {
      if (name.empty()) {
        return "pluss::ai::Procedure::check: step \"" + step_name +
               "\" has an empty " + std::string(role) + " name.";
      }
      if (!seen.insert(name).second) {
        return "pluss::ai::Procedure::check: step \"" + step_name +
               "\" has duplicate " + std::string(role) + " name \"" + name + "\".";
      }
    }
    return std::nullopt;
  }

  [[nodiscard]] static bool detail_contains_name_(const std::vector<std::string>& names,
                                                  std::string_view name) noexcept {
    return std::find_if(names.begin(), names.end(), [name](const std::string& value) {
             return value == name;
           }) != names.end();
  }

  [[nodiscard]] std::vector<std::string> topological_order_() const {
    std::unordered_map<std::string, std::int64_t> indegree{};
    std::unordered_map<std::string, std::vector<std::string>> adjacency{};
    indegree.reserve(steps_.size());
    adjacency.reserve(steps_.size());
    for (const Step& step : steps_) {
      indegree.emplace(step.name(), 0);
      adjacency.emplace(step.name(), std::vector<std::string>{});
    }
    for (const Link& link : links_) {
      adjacency[link.from].push_back(link.to);
      ++indegree[link.to];
    }

    std::vector<std::string> ready{};
    ready.reserve(steps_.size());
    for (const Step& step : steps_) {
      if (indegree[step.name()] == 0) ready.push_back(step.name());
    }

    std::vector<std::string> order{};
    order.reserve(steps_.size());
    for (std::size_t i = 0; i < ready.size(); ++i) {
      const std::string current = ready[i];
      order.push_back(current);
      for (const std::string& next : adjacency[current]) {
        auto found = indegree.find(next);
        if (found != indegree.end() && --found->second == 0) ready.push_back(next);
      }
    }
    return order;
  }

  [[nodiscard]] std::vector<std::vector<std::string>> execution_waves_() const {
    std::unordered_map<std::string, std::int64_t> indegree{};
    std::unordered_map<std::string, std::vector<std::string>> adjacency{};
    indegree.reserve(steps_.size());
    adjacency.reserve(steps_.size());
    for (const Step& step : steps_) {
      indegree.emplace(step.name(), 0);
      adjacency.emplace(step.name(), std::vector<std::string>{});
    }
    for (const Link& link : links_) {
      adjacency[link.from].push_back(link.to);
      ++indegree[link.to];
    }

    std::unordered_set<std::string> emitted{};
    emitted.reserve(steps_.size());
    std::vector<std::vector<std::string>> waves{};
    waves.reserve(steps_.size());
    while (emitted.size() < steps_.size()) {
      std::vector<std::string> wave{};
      wave.reserve(steps_.size() - emitted.size());
      for (const Step& step : steps_) {
        if (!emitted.contains(step.name()) && indegree[step.name()] == 0) {
          wave.push_back(step.name());
        }
      }
      if (wave.empty()) {
        ::pluss::throw_error("pluss::ai::Procedure::run: procedure graph contains a cycle.");
      }
      for (const std::string& name : wave) emitted.insert(name);
      for (const std::string& name : wave) {
        for (const std::string& next : adjacency[name]) {
          auto found = indegree.find(next);
          if (found != indegree.end()) --found->second;
        }
      }
      waves.push_back(std::move(wave));
    }
    return waves;
  }

  [[nodiscard]] static std::string default_step_output_name_(const Step& step) {
    if (step.outputs().size() == 1) return step.outputs().front();
    return step.name();
  }

  [[nodiscard]] const Step* find_step_(std::string_view name) const noexcept {
    for (const Step& step : steps_) {
      if (step.name() == name) return &step;
    }
    return nullptr;
  }

  [[nodiscard]] std::string link_source_artifact_name_(const Link& link) const {
    if (!link.from_output.empty()) return link.from_output;
    const Step* source = find_step_(link.from);
    if (source != nullptr && source->outputs().size() == 1) return source->outputs().front();
    return link.from;
  }

  [[nodiscard]] std::vector<Artifact> resolve_step_inputs_(
      const Step& step,
      const std::unordered_map<std::string, Artifact>& artifacts) const {
    std::vector<Artifact> out{};
    for (const Link& link : links_) {
      if (link.to != step.name()) continue;
      const std::string artifact_name = link_source_artifact_name_(link);
      const auto found = artifacts.find(artifact_name);
      if (found == artifacts.end()) {
        ::pluss::throw_error("pluss::ai::Procedure::run: step \"" + step.name() +
                             "\" requires missing artifact \"" + artifact_name + "\".");
      }
      out.push_back(found->second);
    }
    if (!out.empty()) return out;
    for (const std::string& input_name : step.inputs()) {
      const auto found = artifacts.find(input_name);
      if (found == artifacts.end()) {
        ::pluss::throw_error("pluss::ai::Procedure::run: step \"" + step.name() +
                             "\" requires missing input artifact \"" + input_name + "\".");
      }
      out.push_back(found->second);
    }
    return out;
  }

  std::vector<Step> steps_{};
  std::vector<Link> links_{};
  ProcedureOptions options_{};
};

namespace procedure {

using StepLoader =
    std::function<Step(std::string,
                       StepOptions,
                       std::string_view,
                       const std::vector<std::pair<std::string, std::string>>&)>;
using IceFactory = std::function<std::shared_ptr<ice::Model>()>;
using CompatibilityGate =
    std::function<std::optional<std::string>(std::int64_t,
                                             std::int64_t,
                                             const std::vector<std::pair<std::string, std::string>>&)>;

struct CustomContract {
  std::string key{};
  std::int64_t persist_schema_version = 1;
  std::int64_t component_schema_version = 1;
  bool frozen = false;
  StepLoader load{};
  CompatibilityGate compatible{};
};

struct CustomPersist {
  std::string key{};
  bool reloadable = true;
  bool frozen = false;
  std::vector<std::pair<std::string, std::string>> fields{};
  StepPersistSpec::Saver save{};
};

[[nodiscard]] inline std::unordered_map<std::string, CustomContract>& custom_contracts_();
[[nodiscard]] inline std::unordered_map<std::string, StepLoader>& step_loaders_();

namespace detail {

inline constexpr std::int64_t kCurrentProcedureSchemaVersion = 1;
inline constexpr std::int64_t kMinLoadableProcedureSchemaVersion = 1;
inline constexpr std::string_view kProcedureProducer = "pluss::ai::Procedure";
inline constexpr std::string_view kProcedureProducerVersion = "unversioned";

template <class Fn>
concept SeriesStepCallable = requires(Fn& fn, const ::pluss::Series& x) {
  fn(x);
};

template <class Fn>
concept DataFrameStepCallable = requires(Fn& fn, const pluss::table::DataFrame& frame) {
  fn(frame);
};

template <class Module>
concept SeriesForwardModule = requires(Module& module, const ::pluss::Series& x) {
  { module.forward(x) } -> std::convertible_to<::pluss::Series>;
};

[[nodiscard]] inline std::string sanitize_step_name_(std::string_view name) {
  std::string out{};
  out.reserve(name.size());
  for (const unsigned char ch : name) {
    if ((ch >= '0' && ch <= '9') ||
        (ch >= 'A' && ch <= 'Z') ||
        (ch >= 'a' && ch <= 'z')) {
      out.push_back(static_cast<char>(ch));
    } else {
      out.push_back('_');
    }
  }
  if (out.empty()) return "step";
  return out;
}

inline void write_line_(std::ostream& out, std::string_view key, std::string_view value) {
  out << key << "=" << value << "\n";
}

[[nodiscard]] inline std::pair<std::string, std::string> split_line_(const std::string& line, const char* where) {
  const std::size_t split = line.find('=');
  if (split == std::string::npos) {
    ::pluss::throw_error(std::string(where) + ": malformed manifest line \"" + line + "\".");
  }
  return {line.substr(0, split), line.substr(split + 1)};
}

[[nodiscard]] inline std::int64_t parse_i64_(std::string_view text, const char* where) {
  try {
    return std::stoll(std::string(text));
  } catch (...) {
    ::pluss::throw_error(std::string(where) + ": expected integer value, got \"" + std::string(text) + "\".");
  }
}

[[nodiscard]] inline bool parse_bool_(std::string_view text, const char* where) {
  if (text == "1" || text == "true") return true;
  if (text == "0" || text == "false") return false;
  ::pluss::throw_error(std::string(where) + ": expected boolean value, got \"" + std::string(text) + "\".");
}

[[nodiscard]] inline std::string bool_text_(bool value) {
  return value ? "1" : "0";
}

[[nodiscard]] inline std::string f64_text_(double value) {
  std::ostringstream out;
  out.precision(17);
  out << value;
  return out.str();
}

[[nodiscard]] inline double parse_f64_(std::string_view text, const char* where) {
  try {
    return std::stod(std::string(text));
  } catch (...) {
    ::pluss::throw_error(std::string(where) + ": expected floating-point value, got \"" + std::string(text) + "\".");
  }
}

template <class Enum>
[[nodiscard]] inline std::string enum_text_(Enum value) {
  return std::to_string(static_cast<std::int64_t>(value));
}

template <class Enum>
[[nodiscard]] inline Enum parse_enum_(std::string_view text, const char* where) {
  return static_cast<Enum>(parse_i64_(text, where));
}

[[nodiscard]] inline std::string join_capabilities_(const std::vector<StepCapability>& capabilities) {
  std::ostringstream out;
  for (std::size_t i = 0; i < capabilities.size(); ++i) {
    if (i != 0) out << ",";
    out << static_cast<std::uint16_t>(capabilities[i]);
  }
  return out.str();
}

[[nodiscard]] inline std::vector<StepCapability> split_capabilities_(std::string_view text) {
  std::vector<StepCapability> out{};
  std::string current{};
  for (const char ch : text) {
    if (ch == ',') {
      if (!current.empty()) {
        out.push_back(static_cast<StepCapability>(parse_i64_(current, "pluss::ai::procedure::load(capabilities)")));
        current.clear();
      }
      continue;
    }
    current.push_back(ch);
  }
  if (!current.empty()) {
    out.push_back(static_cast<StepCapability>(parse_i64_(current, "pluss::ai::procedure::load(capabilities)")));
  }
  return out;
}

[[nodiscard]] inline std::string join_metric_kinds_(const std::vector<lymph::MetricKind>& metrics) {
  std::ostringstream out;
  for (std::size_t i = 0; i < metrics.size(); ++i) {
    if (i != 0) out << ",";
    out << static_cast<std::int64_t>(metrics[i]);
  }
  return out.str();
}

[[nodiscard]] inline std::vector<lymph::MetricKind> split_metric_kinds_(std::string_view text) {
  std::vector<lymph::MetricKind> out{};
  std::string current{};
  for (const char ch : text) {
    if (ch == ',') {
      if (!current.empty()) {
        out.push_back(parse_enum_<lymph::MetricKind>(current, "pluss::ai::procedure::load(metrics)"));
        current.clear();
      }
      continue;
    }
    current.push_back(ch);
  }
  if (!current.empty()) {
    out.push_back(parse_enum_<lymph::MetricKind>(current, "pluss::ai::procedure::load(metrics)"));
  }
  return out;
}

[[nodiscard]] inline std::vector<std::filesystem::path> artifact_members_(const std::filesystem::path& base) {
  std::vector<std::filesystem::path> out{};
  const std::filesystem::path parent = base.parent_path();
  if (!std::filesystem::exists(parent)) return out;
  const std::string prefix = base.filename().string();
  for (const auto& entry : std::filesystem::directory_iterator(parent)) {
    if (!entry.is_regular_file()) continue;
    const std::string name = entry.path().filename().string();
    if (name.rfind(prefix, 0) == 0) out.push_back(entry.path());
  }
  std::sort(out.begin(), out.end(), [](const std::filesystem::path& lhs, const std::filesystem::path& rhs) {
    return lhs.string() < rhs.string();
  });
  return out;
}

[[nodiscard]] inline std::uint64_t fnv1a_file_bytes_(const std::vector<std::filesystem::path>& files) {
  constexpr std::uint64_t kOffset = 1469598103934665603ULL;
  constexpr std::uint64_t kPrime = 1099511628211ULL;
  std::uint64_t hash = kOffset;
  for (const auto& path : files) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
      ::pluss::throw_error("pluss::ai::Procedure persistence: could not open \"" + path.string() + "\" for checksum.");
    }
    char buffer[4096];
    while (in.good()) {
      in.read(buffer, sizeof(buffer));
      const std::streamsize count = in.gcount();
      for (std::streamsize i = 0; i < count; ++i) {
        hash ^= static_cast<unsigned char>(buffer[i]);
        hash *= kPrime;
      }
    }
  }
  return hash;
}

[[nodiscard]] inline std::string artifact_checksum_(const std::filesystem::path& base) {
  const std::vector<std::filesystem::path> members = artifact_members_(base);
  if (members.empty()) return {};
  std::ostringstream out;
  out << std::hex << fnv1a_file_bytes_(members);
  return out.str();
}

[[nodiscard]] inline std::vector<std::pair<std::string, std::string>> find_fields_(
    const std::vector<std::pair<std::string, std::string>>& fields,
    std::string_view key) {
  std::vector<std::pair<std::string, std::string>> out{};
  for (const auto& field : fields) {
    if (field.first == key) out.push_back(field);
  }
  return out;
}

[[nodiscard]] inline std::string field_or_(const std::vector<std::pair<std::string, std::string>>& fields,
                                           std::string_view key,
                                           std::string default_value = {}) {
  for (const auto& field : fields) {
    if (field.first == key) return field.second;
  }
  return default_value;
}

inline void push_field_(std::vector<std::pair<std::string, std::string>>& fields,
                        std::string key,
                        std::string value) {
  fields.emplace_back(std::move(key), std::move(value));
}

[[nodiscard]] inline std::filesystem::path sidecar_path_(std::string_view base, std::string_view suffix) {
  return std::filesystem::path(std::string(base) + std::string(suffix));
}

inline void sync_series_sidecar_(std::string_view base, std::string_view suffix, const Series& value) {
  const std::filesystem::path path = sidecar_path_(base, suffix);
  if (value.defined()) {
    ::pluss::io::save(value, path.string());
    return;
  }
  std::error_code ec;
  std::filesystem::remove(path, ec);
}

[[nodiscard]] inline Series load_series_sidecar_(const std::vector<std::pair<std::string, std::string>>& fields,
                                                 std::string_view present_key,
                                                 std::string_view base,
                                                 std::string_view suffix) {
  if (!parse_bool_(field_or_(fields, present_key, "0"), "pluss::ai::procedure::load(series_sidecar)")) {
    return Series::undefined();
  }
  return ::pluss::io::load<Series>(sidecar_path_(base, suffix).string());
}

[[nodiscard]] inline std::vector<std::pair<std::string, std::string>> lymph_train_fields_(
    const lymph::TrainOptions& options) {
  std::vector<std::pair<std::string, std::string>> fields{};
  fields.reserve(26);
  push_field_(fields, "train.layout", enum_text_(options.layout));
  push_field_(fields, "train.missing", enum_text_(options.missing));
  push_field_(fields, "train.unknown_categorical", enum_text_(options.unknown_categorical));
  push_field_(fields, "train.max_iterations", std::to_string(options.max_iterations));
  push_field_(fields, "train.tolerance", f64_text_(options.tolerance));
  push_field_(fields, "train.warm_start", bool_text_(options.warm_start));
  push_field_(fields, "train.track_diagnostics", bool_text_(options.track_diagnostics));
  push_field_(fields, "train.random.seed", std::to_string(options.random.seed));
  push_field_(fields, "train.random.deterministic", bool_text_(options.random.deterministic));
  push_field_(fields, "train.sample_weights", bool_text_(options.sample_weights.defined()));
  push_field_(fields, "train.class_weight", enum_text_(options.class_weight));
  push_field_(fields, "train.class_weights", bool_text_(options.class_weights.defined()));
  push_field_(fields, "train.calibration", enum_text_(options.calibration));
  push_field_(fields, "train.calibration_iterations", std::to_string(options.calibration_iterations));
  push_field_(fields, "train.calibration_tolerance", f64_text_(options.calibration_tolerance));
  push_field_(fields, "train.parameter_lower_bounds", bool_text_(options.parameter_lower_bounds.defined()));
  push_field_(fields, "train.parameter_upper_bounds", bool_text_(options.parameter_upper_bounds.defined()));
  push_field_(fields, "train.balance.kind", enum_text_(options.balance.kind));
  push_field_(fields, "train.balance.ratio", f64_text_(options.balance.ratio));
  push_field_(fields, "train.balance.counts", bool_text_(options.balance.counts.defined()));
  push_field_(fields, "train.balance.growth", enum_text_(options.balance.growth));
  push_field_(fields, "train.balance.neighbors", std::to_string(options.balance.neighbors));
  push_field_(fields, "train.balance.clean", enum_text_(options.balance.clean));
  push_field_(fields, "train.balance.clean_neighbors", std::to_string(options.balance.clean_neighbors));
  push_field_(fields, "train.balance.clean_vote", enum_text_(options.balance.clean_vote));
  push_field_(fields, "train.balance.protect_small_classes", bool_text_(options.balance.protect_small_classes));
  return fields;
}

[[nodiscard]] inline std::vector<std::pair<std::string, std::string>> lymph_infer_fields_(
    const lymph::InferOptions& options) {
  return {
      {"infer.layout", enum_text_(options.layout)},
      {"infer.missing", enum_text_(options.missing)},
      {"infer.unknown_categorical", enum_text_(options.unknown_categorical)},
      {"infer.positive_threshold", f64_text_(options.positive_threshold)},
  };
}

[[nodiscard]] inline std::vector<std::pair<std::string, std::string>> lymph_assess_fields_(
    const lymph::AssessOptions& options) {
  return {
      {"assess.layout", enum_text_(options.layout)},
      {"assess.metrics", join_metric_kinds_(options.metrics)},
      {"assess.sample_weights", bool_text_(options.sample_weights.defined())},
      {"assess.positive_threshold", f64_text_(options.positive_threshold)},
      {"assess.regression_quantile", f64_text_(options.regression_quantile)},
  };
}

[[nodiscard]] inline lymph::TrainOptions load_lymph_train_options_(
    const std::vector<std::pair<std::string, std::string>>& fields,
    std::string_view base) {
  lymph::TrainOptions options{};
  options.layout = parse_enum_<lymph::ObservationLayout>(
      field_or_(fields, "train.layout", enum_text_(lymph::ObservationLayout::Rows)),
      "pluss::ai::procedure::load(train.layout)");
  options.missing = parse_enum_<lymph::MissingPolicy>(
      field_or_(fields, "train.missing", enum_text_(lymph::MissingPolicy::Reject)),
      "pluss::ai::procedure::load(train.missing)");
  options.unknown_categorical = parse_enum_<lymph::UnknownCategoryPolicy>(
      field_or_(fields, "train.unknown_categorical", enum_text_(lymph::UnknownCategoryPolicy::Error)),
      "pluss::ai::procedure::load(train.unknown_categorical)");
  options.max_iterations = parse_i64_(
      field_or_(fields, "train.max_iterations", "1000"),
      "pluss::ai::procedure::load(train.max_iterations)");
  options.tolerance = parse_f64_(
      field_or_(fields, "train.tolerance", "1e-06"),
      "pluss::ai::procedure::load(train.tolerance)");
  options.warm_start = parse_bool_(
      field_or_(fields, "train.warm_start", "0"),
      "pluss::ai::procedure::load(train.warm_start)");
  options.track_diagnostics = parse_bool_(
      field_or_(fields, "train.track_diagnostics", "1"),
      "pluss::ai::procedure::load(train.track_diagnostics)");
  options.random.seed = static_cast<std::uint64_t>(parse_i64_(
      field_or_(fields, "train.random.seed", "0"),
      "pluss::ai::procedure::load(train.random.seed)"));
  options.random.deterministic = parse_bool_(
      field_or_(fields, "train.random.deterministic", "1"),
      "pluss::ai::procedure::load(train.random.deterministic)");
  options.sample_weights = load_series_sidecar_(fields, "train.sample_weights", base, ".train_sample_weights.bin");
  options.class_weight = parse_enum_<lymph::ClassWeightMode>(
      field_or_(fields, "train.class_weight", enum_text_(lymph::ClassWeightMode::None)),
      "pluss::ai::procedure::load(train.class_weight)");
  options.class_weights = load_series_sidecar_(fields, "train.class_weights", base, ".train_class_weights.bin");
  options.calibration = parse_enum_<lymph::ScoreCalibration>(
      field_or_(fields, "train.calibration", enum_text_(lymph::ScoreCalibration::None)),
      "pluss::ai::procedure::load(train.calibration)");
  options.calibration_iterations = parse_i64_(
      field_or_(fields, "train.calibration_iterations", "400"),
      "pluss::ai::procedure::load(train.calibration_iterations)");
  options.calibration_tolerance = parse_f64_(
      field_or_(fields, "train.calibration_tolerance", "1e-06"),
      "pluss::ai::procedure::load(train.calibration_tolerance)");
  options.parameter_lower_bounds =
      load_series_sidecar_(fields, "train.parameter_lower_bounds", base, ".train_parameter_lower_bounds.bin");
  options.parameter_upper_bounds =
      load_series_sidecar_(fields, "train.parameter_upper_bounds", base, ".train_parameter_upper_bounds.bin");
  options.balance.kind = parse_enum_<lymph::BalanceTargetKind>(
      field_or_(fields, "train.balance.kind", enum_text_(lymph::BalanceTargetKind::Keep)),
      "pluss::ai::procedure::load(train.balance.kind)");
  options.balance.ratio = parse_f64_(
      field_or_(fields, "train.balance.ratio", "1"),
      "pluss::ai::procedure::load(train.balance.ratio)");
  options.balance.counts = load_series_sidecar_(fields, "train.balance.counts", base, ".train_balance_counts.bin");
  options.balance.growth = parse_enum_<lymph::BalanceGrowthKind>(
      field_or_(fields, "train.balance.growth", enum_text_(lymph::BalanceGrowthKind::ReuseRows)),
      "pluss::ai::procedure::load(train.balance.growth)");
  options.balance.neighbors = parse_i64_(
      field_or_(fields, "train.balance.neighbors", "5"),
      "pluss::ai::procedure::load(train.balance.neighbors)");
  options.balance.clean = parse_enum_<lymph::BalanceCleanKind>(
      field_or_(fields, "train.balance.clean", enum_text_(lymph::BalanceCleanKind::None)),
      "pluss::ai::procedure::load(train.balance.clean)");
  options.balance.clean_neighbors = parse_i64_(
      field_or_(fields, "train.balance.clean_neighbors", "3"),
      "pluss::ai::procedure::load(train.balance.clean_neighbors)");
  options.balance.clean_vote = parse_enum_<lymph::BalanceCleanVote>(
      field_or_(fields, "train.balance.clean_vote", enum_text_(lymph::BalanceCleanVote::AnyDisagreement)),
      "pluss::ai::procedure::load(train.balance.clean_vote)");
  options.balance.protect_small_classes = parse_bool_(
      field_or_(fields, "train.balance.protect_small_classes", "1"),
      "pluss::ai::procedure::load(train.balance.protect_small_classes)");
  return options;
}

[[nodiscard]] inline lymph::InferOptions load_lymph_infer_options_(
    const std::vector<std::pair<std::string, std::string>>& fields) {
  lymph::InferOptions options{};
  options.layout = parse_enum_<lymph::ObservationLayout>(
      field_or_(fields, "infer.layout", enum_text_(lymph::ObservationLayout::Rows)),
      "pluss::ai::procedure::load(infer.layout)");
  options.missing = parse_enum_<lymph::MissingPolicy>(
      field_or_(fields, "infer.missing", enum_text_(lymph::MissingPolicy::Reject)),
      "pluss::ai::procedure::load(infer.missing)");
  options.unknown_categorical = parse_enum_<lymph::UnknownCategoryPolicy>(
      field_or_(fields, "infer.unknown_categorical", enum_text_(lymph::UnknownCategoryPolicy::Error)),
      "pluss::ai::procedure::load(infer.unknown_categorical)");
  options.positive_threshold = parse_f64_(
      field_or_(fields, "infer.positive_threshold", "0.5"),
      "pluss::ai::procedure::load(infer.positive_threshold)");
  return options;
}

[[nodiscard]] inline lymph::AssessOptions load_lymph_assess_options_(
    const std::vector<std::pair<std::string, std::string>>& fields,
    std::string_view base) {
  lymph::AssessOptions options{};
  options.layout = parse_enum_<lymph::ObservationLayout>(
      field_or_(fields, "assess.layout", enum_text_(lymph::ObservationLayout::Rows)),
      "pluss::ai::procedure::load(assess.layout)");
  options.metrics = split_metric_kinds_(field_or_(fields, "assess.metrics", ""));
  options.sample_weights = load_series_sidecar_(fields, "assess.sample_weights", base, ".assess_sample_weights.bin");
  options.positive_threshold = parse_f64_(
      field_or_(fields, "assess.positive_threshold", "0.5"),
      "pluss::ai::procedure::load(assess.positive_threshold)");
  options.regression_quantile = parse_f64_(
      field_or_(fields, "assess.regression_quantile", "0.5"),
      "pluss::ai::procedure::load(assess.regression_quantile)");
  return options;
}

[[nodiscard]] inline std::vector<std::pair<std::string, std::string>> ice_train_fields_(
    const ice::TrainOptions& options) {
  std::vector<std::pair<std::string, std::string>> fields{};
  fields.reserve(42);
  push_field_(fields, "train.batch_size", std::to_string(options.batch_size));
  push_field_(fields, "train.learning_rate", f64_text_(options.learning_rate));
  push_field_(fields, "train.optimizer", enum_text_(options.optimizer));
  push_field_(fields, "train.loss", enum_text_(options.loss));
  push_field_(fields, "train.shuffle", bool_text_(options.shuffle));
  push_field_(fields, "train.seed", std::to_string(options.seed));
  push_field_(fields, "train.standardize", bool_text_(options.standardize));
  push_field_(fields, "train.validation.split_ratio", f64_text_(options.validation.split_ratio));
  push_field_(fields, "train.validation.every_n_epochs", std::to_string(options.validation.every_n_epochs));
  push_field_(fields, "train.validation.start_from_epoch", std::to_string(options.validation.start_from_epoch));
  push_field_(fields, "train.validation.monitor", options.validation.monitor);
  push_field_(fields, "train.validation.direction", enum_text_(options.validation.direction));
  push_field_(fields, "train.arch.dropout", f64_text_(options.architecture_regularization.dropout));
  push_field_(fields, "train.objective.weight_decay", f64_text_(options.objective_regularization.weight_decay));
  push_field_(fields, "train.objective.decoupled_weight_decay", bool_text_(options.objective_regularization.decoupled_weight_decay));
  push_field_(fields, "train.objective.l1_penalty", f64_text_(options.objective_regularization.l1_penalty));
  push_field_(fields, "train.objective.l2_penalty", f64_text_(options.objective_regularization.l2_penalty));
  push_field_(fields, "train.objective.label_smoothing", f64_text_(options.objective_regularization.label_smoothing));
  push_field_(fields, "train.loop.early_stopping", bool_text_(options.loop_regularization.early_stopping));
  push_field_(fields, "train.loop.patience", std::to_string(options.loop_regularization.patience));
  push_field_(fields, "train.loop.min_delta", f64_text_(options.loop_regularization.min_delta));
  push_field_(fields, "train.loop.restore_best_weights", bool_text_(options.loop_regularization.restore_best_weights));
  push_field_(fields, "train.checkpoint.enabled", bool_text_(options.checkpoint.enabled));
  push_field_(fields, "train.checkpoint.save_last", bool_text_(options.checkpoint.save_last));
  push_field_(fields, "train.checkpoint.save_best", bool_text_(options.checkpoint.save_best));
  push_field_(fields, "train.checkpoint.save_weights_only", bool_text_(options.checkpoint.save_weights_only));
  push_field_(fields, "train.checkpoint.save_on_exception", bool_text_(options.checkpoint.save_on_exception));
  push_field_(fields, "train.checkpoint.every_n_epochs", std::to_string(options.checkpoint.every_n_epochs));
  push_field_(fields, "train.checkpoint.dirpath", options.checkpoint.dirpath);
  push_field_(fields, "train.checkpoint.filename", options.checkpoint.filename);
  push_field_(fields, "train.checkpoint.resume_from", options.checkpoint.resume_from);
  push_field_(fields, "train.execution.grad_clip.mode", enum_text_(options.execution.grad_clip.mode));
  push_field_(fields, "train.execution.grad_clip.threshold", f64_text_(options.execution.grad_clip.threshold));
  push_field_(fields, "train.execution.max_epochs", std::to_string(options.execution.max_epochs));
  push_field_(fields, "train.execution.accumulation_steps", std::to_string(options.execution.accumulation_steps));
  push_field_(fields, "train.execution.fast_dev_batches", std::to_string(options.execution.fast_dev_batches));
  push_field_(fields, "train.execution.max_train_batches", std::to_string(options.execution.max_train_batches));
  push_field_(fields, "train.execution.max_valid_batches", std::to_string(options.execution.max_valid_batches));
  push_field_(fields, "train.execution.fail_on_non_finite", bool_text_(options.execution.fail_on_non_finite));
  push_field_(fields, "train.execution.deterministic", bool_text_(options.execution.deterministic));
  push_field_(fields, "train.execution.precision", enum_text_(options.execution.precision));
  push_field_(fields, "train.execution.platform", enum_text_(options.execution.platform));
  return fields;
}

[[nodiscard]] inline ice::TrainOptions load_ice_train_options_(
    const std::vector<std::pair<std::string, std::string>>& fields) {
  ice::TrainOptions options{};
  options.batch_size = parse_i64_(field_or_(fields, "train.batch_size", "32"),
                                  "pluss::ai::procedure::load(train.batch_size)");
  options.learning_rate = parse_f64_(field_or_(fields, "train.learning_rate", "0.001"),
                                     "pluss::ai::procedure::load(train.learning_rate)");
  options.optimizer = parse_enum_<ice::OptimizerKind>(
      field_or_(fields, "train.optimizer", enum_text_(ice::OptimizerKind::Adam)),
      "pluss::ai::procedure::load(train.optimizer)");
  options.loss = parse_enum_<ice::LossKind>(
      field_or_(fields, "train.loss", enum_text_(ice::LossKind::MSE)),
      "pluss::ai::procedure::load(train.loss)");
  options.shuffle = parse_bool_(field_or_(fields, "train.shuffle", "1"),
                                "pluss::ai::procedure::load(train.shuffle)");
  options.seed = static_cast<std::uint64_t>(parse_i64_(field_or_(fields, "train.seed", "0"),
                                                       "pluss::ai::procedure::load(train.seed)"));
  options.standardize = parse_bool_(field_or_(fields, "train.standardize", "0"),
                                    "pluss::ai::procedure::load(train.standardize)");
  options.validation.split_ratio = parse_f64_(
      field_or_(fields, "train.validation.split_ratio", "0.1"),
      "pluss::ai::procedure::load(train.validation.split_ratio)");
  options.validation.every_n_epochs = parse_i64_(
      field_or_(fields, "train.validation.every_n_epochs", "1"),
      "pluss::ai::procedure::load(train.validation.every_n_epochs)");
  options.validation.start_from_epoch = parse_i64_(
      field_or_(fields, "train.validation.start_from_epoch", "0"),
      "pluss::ai::procedure::load(train.validation.start_from_epoch)");
  options.validation.monitor = field_or_(fields, "train.validation.monitor", "val_loss");
  options.validation.direction = parse_enum_<ice::MonitorDirection>(
      field_or_(fields, "train.validation.direction", enum_text_(ice::MonitorDirection::Auto)),
      "pluss::ai::procedure::load(train.validation.direction)");
  options.architecture_regularization.dropout = parse_f64_(
      field_or_(fields, "train.arch.dropout", "0"),
      "pluss::ai::procedure::load(train.arch.dropout)");
  options.objective_regularization.weight_decay = parse_f64_(
      field_or_(fields, "train.objective.weight_decay", "0"),
      "pluss::ai::procedure::load(train.objective.weight_decay)");
  options.objective_regularization.decoupled_weight_decay = parse_bool_(
      field_or_(fields, "train.objective.decoupled_weight_decay", "0"),
      "pluss::ai::procedure::load(train.objective.decoupled_weight_decay)");
  options.objective_regularization.l1_penalty = parse_f64_(
      field_or_(fields, "train.objective.l1_penalty", "0"),
      "pluss::ai::procedure::load(train.objective.l1_penalty)");
  options.objective_regularization.l2_penalty = parse_f64_(
      field_or_(fields, "train.objective.l2_penalty", "0"),
      "pluss::ai::procedure::load(train.objective.l2_penalty)");
  options.objective_regularization.label_smoothing = parse_f64_(
      field_or_(fields, "train.objective.label_smoothing", "0"),
      "pluss::ai::procedure::load(train.objective.label_smoothing)");
  options.loop_regularization.early_stopping = parse_bool_(
      field_or_(fields, "train.loop.early_stopping", "1"),
      "pluss::ai::procedure::load(train.loop.early_stopping)");
  options.loop_regularization.patience = parse_i64_(
      field_or_(fields, "train.loop.patience", "10"),
      "pluss::ai::procedure::load(train.loop.patience)");
  options.loop_regularization.min_delta = parse_f64_(
      field_or_(fields, "train.loop.min_delta", "1e-06"),
      "pluss::ai::procedure::load(train.loop.min_delta)");
  options.loop_regularization.restore_best_weights = parse_bool_(
      field_or_(fields, "train.loop.restore_best_weights", "0"),
      "pluss::ai::procedure::load(train.loop.restore_best_weights)");
  options.checkpoint.enabled = parse_bool_(field_or_(fields, "train.checkpoint.enabled", "0"),
                                           "pluss::ai::procedure::load(train.checkpoint.enabled)");
  options.checkpoint.save_last = parse_bool_(field_or_(fields, "train.checkpoint.save_last", "0"),
                                             "pluss::ai::procedure::load(train.checkpoint.save_last)");
  options.checkpoint.save_best = parse_bool_(field_or_(fields, "train.checkpoint.save_best", "0"),
                                             "pluss::ai::procedure::load(train.checkpoint.save_best)");
  options.checkpoint.save_weights_only = parse_bool_(
      field_or_(fields, "train.checkpoint.save_weights_only", "0"),
      "pluss::ai::procedure::load(train.checkpoint.save_weights_only)");
  options.checkpoint.save_on_exception = parse_bool_(
      field_or_(fields, "train.checkpoint.save_on_exception", "0"),
      "pluss::ai::procedure::load(train.checkpoint.save_on_exception)");
  options.checkpoint.every_n_epochs = parse_i64_(
      field_or_(fields, "train.checkpoint.every_n_epochs", "1"),
      "pluss::ai::procedure::load(train.checkpoint.every_n_epochs)");
  options.checkpoint.dirpath = field_or_(fields, "train.checkpoint.dirpath", {});
  options.checkpoint.filename = field_or_(fields, "train.checkpoint.filename", {});
  options.checkpoint.resume_from = field_or_(fields, "train.checkpoint.resume_from", {});
  options.execution.grad_clip.mode = parse_enum_<pluss::nn::optim::GradientClipMode>(
      field_or_(fields, "train.execution.grad_clip.mode", enum_text_(pluss::nn::optim::GradientClipMode::ByNorm)),
      "pluss::ai::procedure::load(train.execution.grad_clip.mode)");
  options.execution.grad_clip.threshold = parse_f64_(
      field_or_(fields, "train.execution.grad_clip.threshold", "1"),
      "pluss::ai::procedure::load(train.execution.grad_clip.threshold)");
  options.execution.max_epochs = parse_i64_(field_or_(fields, "train.execution.max_epochs", "60"),
                                            "pluss::ai::procedure::load(train.execution.max_epochs)");
  options.execution.accumulation_steps = parse_i64_(
      field_or_(fields, "train.execution.accumulation_steps", "1"),
      "pluss::ai::procedure::load(train.execution.accumulation_steps)");
  options.execution.fast_dev_batches = parse_i64_(
      field_or_(fields, "train.execution.fast_dev_batches", "0"),
      "pluss::ai::procedure::load(train.execution.fast_dev_batches)");
  options.execution.max_train_batches = parse_i64_(
      field_or_(fields, "train.execution.max_train_batches", "0"),
      "pluss::ai::procedure::load(train.execution.max_train_batches)");
  options.execution.max_valid_batches = parse_i64_(
      field_or_(fields, "train.execution.max_valid_batches", "0"),
      "pluss::ai::procedure::load(train.execution.max_valid_batches)");
  options.execution.fail_on_non_finite = parse_bool_(
      field_or_(fields, "train.execution.fail_on_non_finite", "1"),
      "pluss::ai::procedure::load(train.execution.fail_on_non_finite)");
  options.execution.deterministic = parse_bool_(
      field_or_(fields, "train.execution.deterministic", "0"),
      "pluss::ai::procedure::load(train.execution.deterministic)");
  options.execution.precision = parse_enum_<ScalarType>(
      field_or_(fields, "train.execution.precision", enum_text_(ScalarType::Float32)),
      "pluss::ai::procedure::load(train.execution.precision)");
  options.execution.platform = parse_enum_<ice::TrainPlatform>(
      field_or_(fields, "train.execution.platform", enum_text_(ice::TrainPlatform::CPU)),
      "pluss::ai::procedure::load(train.execution.platform)");
  return options;
}

[[nodiscard]] inline Step::Runner make_ice_runner_(std::shared_ptr<ice::Model> state,
                                                   ice::LossKind assess_loss,
                                                   ice::TrainOptions train_options,
                                                   std::string output) {
  return [state = std::move(state),
          assess_loss,
          train_options = std::move(train_options),
          output = std::move(output)](
             const std::vector<Artifact>& inputs, const RunOptions& run_options) mutable -> Artifact {
    if (inputs.size() != 1) {
      ::pluss::throw_error("pluss::ai::procedure::step(ice::Model): expected exactly one input artifact.");
    }
    const Artifact& input = inputs.front();
    if (run_options.mode == ProcedureMode::Learn) {
      if (input.is<lymph::LabeledBatch>()) {
        const lymph::LabeledBatch& batch = input.get<lymph::LabeledBatch>();
        state->learn(batch.features.x, batch.y, train_options);
        return Artifact(output, Artifact::Value(true));
      }
      if (input.is<ice::Batch>()) {
        const ice::Batch& batch = input.get<ice::Batch>();
        if (!batch.has_inputs() || !batch.has_targets()) {
          ::pluss::throw_error("pluss::ai::procedure::step(ice::Model): Ice Batch learn input needs inputs and targets.");
        }
        state->learn(batch.inputs(), batch.targets(), train_options);
        return Artifact(output, Artifact::Value(true));
      }
      ::pluss::throw_error("pluss::ai::procedure::step(ice::Model): Learn mode expects LabeledBatch or ice::Batch input.");
    }
    if (run_options.mode == ProcedureMode::Assess) {
      if (input.is<lymph::LabeledBatch>()) {
        const lymph::LabeledBatch& batch = input.get<lymph::LabeledBatch>();
        return Artifact(output, Artifact::Value(state->assess(batch.features.x, batch.y, assess_loss)));
      }
      if (input.is<ice::Batch>()) {
        const ice::Batch& batch = input.get<ice::Batch>();
        if (!batch.has_inputs() || !batch.has_targets()) {
          ::pluss::throw_error("pluss::ai::procedure::step(ice::Model): Ice Batch assess input needs inputs and targets.");
        }
        return Artifact(output, Artifact::Value(state->assess(batch.inputs(), batch.targets(), assess_loss)));
      }
      ::pluss::throw_error("pluss::ai::procedure::step(ice::Model): Assess mode expects LabeledBatch or ice::Batch input.");
    }
    if (input.is<::pluss::Series>()) {
      return Artifact(output, Artifact::Value(state->infer(input.get<::pluss::Series>())));
    }
    if (input.is<lymph::FeatureBatch>()) {
      return Artifact(output, Artifact::Value(state->infer(input.get<lymph::FeatureBatch>().x)));
    }
    if (input.is<ice::Batch>()) {
      const ice::Batch& batch = input.get<ice::Batch>();
      if (!batch.has_inputs()) {
        ::pluss::throw_error("pluss::ai::procedure::step(ice::Model): Ice Batch infer input needs inputs.");
      }
      return Artifact(output, Artifact::Value(state->infer(batch.inputs())));
    }
    ::pluss::throw_error("pluss::ai::procedure::step(ice::Model): Infer mode expects Series, FeatureBatch, or ice::Batch input.");
  };
}

template <class Fn>
requires (!SeriesForwardModule<Fn> &&
          (SeriesStepCallable<Fn> || DataFrameStepCallable<Fn>))
[[nodiscard]] inline Step::Runner make_callable_runner_(Fn fn, std::string output) {
  return [fn = std::move(fn), output = std::move(output)](
             const std::vector<Artifact>& inputs, const RunOptions&) mutable -> Artifact {
    if (inputs.size() != 1) {
      ::pluss::throw_error("pluss::ai::procedure::step: callable steps require exactly one input artifact.");
    }
    const Artifact& input = inputs.front();
    if constexpr (SeriesStepCallable<Fn>) {
      if (input.is<::pluss::Series>()) {
        using Return = decltype(fn(input.get<::pluss::Series>()));
        if constexpr (std::is_same_v<std::decay_t<Return>, Artifact>) {
          Artifact artifact = fn(input.get<::pluss::Series>());
          if (!artifact.named()) return Artifact(output, artifact.value());
          return artifact;
        } else {
          return Artifact(output, Artifact::Value(fn(input.get<::pluss::Series>())));
        }
      }
    }
    if constexpr (DataFrameStepCallable<Fn>) {
      if (input.is<pluss::table::DataFrame>()) {
        using Return = decltype(fn(input.get<pluss::table::DataFrame>()));
        if constexpr (std::is_same_v<std::decay_t<Return>, Artifact>) {
          Artifact artifact = fn(input.get<pluss::table::DataFrame>());
          if (!artifact.named()) return Artifact(output, artifact.value());
          return artifact;
        } else {
          return Artifact(output, Artifact::Value(fn(input.get<pluss::table::DataFrame>())));
        }
      }
    }
    ::pluss::throw_error("pluss::ai::procedure::step: input artifact type is incompatible with callable step.");
  };
}

[[nodiscard]] inline Step make_ice_step_(std::string name,
                                         std::string input_name,
                                         std::string output_name,
                                         std::shared_ptr<ice::Model> state,
                                         std::string restore_key,
                                         ice::LossKind assess_loss,
                                         ice::TrainOptions train_options) {
  if (input_name.empty() || output_name.empty()) {
    ::pluss::throw_error("pluss::ai::procedure::step: input and output names must not be empty.");
  }
  if (!state) {
    ::pluss::throw_error("pluss::ai::procedure::step(ice::Model): state must not be null.");
  }
  StepOptions options{};
  options.capabilities = {StepCapability::Learn, StepCapability::Infer, StepCapability::Assess};
  options.inputs = {std::move(input_name)};
  options.outputs = {output_name};
  options.persistable = !restore_key.empty();
  options.trained = state->ready();
  options.component = "ice::Model";

  StepPersistSpec persist{};
  if (!restore_key.empty()) {
    persist.key = "ice::Model";
    persist.schema_version = 1;
    persist.component_schema_version = 1;
    persist.reloadable = true;
    persist.fields = ice_train_fields_(train_options);
    push_field_(persist.fields, "ice.restore", restore_key);
    push_field_(persist.fields, "ice.assess_loss", enum_text_(assess_loss));
    persist.save = [state](std::string_view path) {
      state->save(path);
    };
  }

  Step::Runner runner = make_ice_runner_(std::move(state), assess_loss, std::move(train_options), std::move(output_name));
  if (options.persistable) {
    if (name.empty()) {
      ::pluss::throw_error("pluss::ai::procedure::step: name must not be empty.");
    }
    if (!runner) {
      ::pluss::throw_error("pluss::ai::procedure::step: runner must be callable.");
    }
    if (persist.key.empty()) {
      ::pluss::throw_error("pluss::ai::procedure::step: persistable steps need a persistence key.");
    }
    return Step(std::move(name), std::move(options), std::move(persist), std::move(runner));
  }
  if (name.empty()) {
    ::pluss::throw_error("pluss::ai::procedure::step: name must not be empty.");
  }
  if (!runner) {
    ::pluss::throw_error("pluss::ai::procedure::step: runner must be callable.");
  }
  return Step(std::move(name), std::move(options), std::move(runner));
}

inline void validate_loaded_contract_(const Step& loaded,
                                      const StepOptions& manifest_options,
                                      std::string_view persist_key,
                                      std::int64_t persist_schema,
                                      std::int64_t component_schema,
                                      bool reloadable,
                                      bool frozen,
                                      bool custom) {
  if (loaded.component() != manifest_options.component) {
    ::pluss::throw_error("pluss::ai::procedure::load: component mismatch for step \"" + loaded.name() + "\".");
  }
  if (loaded.persistable() != manifest_options.persistable) {
    ::pluss::throw_error("pluss::ai::procedure::load: persistable contract mismatch for step \"" + loaded.name() + "\".");
  }
  if (loaded.persist_key() != persist_key) {
    ::pluss::throw_error("pluss::ai::procedure::load: persistence key mismatch for step \"" + loaded.name() + "\".");
  }
  if (loaded.persist_schema_version() != persist_schema) {
    ::pluss::throw_error("pluss::ai::procedure::load: persistence schema mismatch for step \"" + loaded.name() + "\".");
  }
  if (loaded.component_schema_version() != component_schema) {
    ::pluss::throw_error("pluss::ai::procedure::load: component schema mismatch for step \"" + loaded.name() + "\".");
  }
  if (loaded.reloadable() != reloadable) {
    ::pluss::throw_error("pluss::ai::procedure::load: reloadable contract mismatch for step \"" + loaded.name() + "\".");
  }
  if (loaded.frozen() != frozen) {
    ::pluss::throw_error("pluss::ai::procedure::load: frozen contract mismatch for step \"" + loaded.name() + "\".");
  }
  if (loaded.custom() != custom) {
    ::pluss::throw_error("pluss::ai::procedure::load: custom contract mismatch for step \"" + loaded.name() + "\".");
  }
}

} // namespace detail

[[nodiscard]] inline Step step(std::string name, StepOptions options = {}) {
  if (name.empty()) {
    ::pluss::throw_error("pluss::ai::procedure::step: name must not be empty.");
  }
  return Step(std::move(name), std::move(options));
}

[[nodiscard]] inline Step step(std::string name, StepOptions options, Step::Runner runner) {
  if (name.empty()) {
    ::pluss::throw_error("pluss::ai::procedure::step: name must not be empty.");
  }
  if (!runner) {
    ::pluss::throw_error("pluss::ai::procedure::step: runner must be callable.");
  }
  return Step(std::move(name), std::move(options), std::move(runner));
}

[[nodiscard]] inline Step step(std::string name, StepOptions options, StepPersistSpec persist, Step::Runner runner) {
  if (name.empty()) {
    ::pluss::throw_error("pluss::ai::procedure::step: name must not be empty.");
  }
  if (!runner) {
    ::pluss::throw_error("pluss::ai::procedure::step: runner must be callable.");
  }
  if (options.persistable && persist.key.empty()) {
    ::pluss::throw_error("pluss::ai::procedure::step: persistable steps need a persistence key.");
  }
  return Step(std::move(name), std::move(options), std::move(persist), std::move(runner));
}

[[nodiscard]] inline Step step(std::string name, StepOptions options, CustomPersist persist, Step::Runner runner) {
  const auto found = custom_contracts_().find(persist.key);
  if (found == custom_contracts_().end()) {
    ::pluss::throw_error("pluss::ai::procedure::step(custom): no bound contract for key \"" + persist.key + "\".");
  }
  options.persistable = true;
  if (options.component.empty()) {
    options.component = persist.key;
  }
  StepPersistSpec spec{};
  spec.key = found->second.key;
  spec.schema_version = found->second.persist_schema_version;
  spec.component_schema_version = found->second.component_schema_version;
  spec.reloadable = persist.reloadable;
  spec.frozen = persist.frozen || found->second.frozen;
  spec.custom = true;
  spec.fields = std::move(persist.fields);
  spec.save = std::move(persist.save);
  return step(std::move(name), std::move(options), std::move(spec), std::move(runner));
}

template <class Module>
requires detail::SeriesForwardModule<Module>
[[nodiscard]] inline Step step(std::string name,
                               std::string input_name,
                               std::string output_name,
                               Module module) {
  if (input_name.empty() || output_name.empty()) {
    ::pluss::throw_error("pluss::ai::procedure::step: input and output names must not be empty.");
  }
  StepOptions options{};
  options.capabilities = {StepCapability::Map, StepCapability::Infer};
  options.inputs = {std::move(input_name)};
  options.outputs = {output_name};
  options.component = "nodek::Module";
  Step::Runner runner = [module = std::move(module), output = std::move(output_name)](
                            const std::vector<Artifact>& inputs, const RunOptions&) mutable -> Artifact {
    if (inputs.size() != 1) {
      ::pluss::throw_error("pluss::ai::procedure::step(nodek): expected exactly one input artifact.");
    }
    const Artifact& input = inputs.front();
    if (input.is<::pluss::Series>()) {
      return Artifact(output, Artifact::Value(module.forward(input.get<::pluss::Series>())));
    }
    ::pluss::throw_error("pluss::ai::procedure::step(nodek): expected Series input.");
  };
  return step(std::move(name), std::move(options), std::move(runner));
}

template <class Fn>
requires (!detail::SeriesForwardModule<Fn> &&
          (detail::SeriesStepCallable<Fn> || detail::DataFrameStepCallable<Fn>))
[[nodiscard]] inline Step step(std::string name,
                               std::string input_name,
                               std::string output_name,
                               Fn fn) {
  if (input_name.empty() || output_name.empty()) {
    ::pluss::throw_error("pluss::ai::procedure::step: input and output names must not be empty.");
  }
  StepOptions options{};
  options.capabilities = {StepCapability::Map};
  options.inputs = {std::move(input_name)};
  options.outputs = {output_name};
  options.component = "callable";
  Step::Runner runner = detail::make_callable_runner_(std::move(fn), std::move(output_name));
  return step(std::move(name), std::move(options), std::move(runner));
}

template <class Fn>
requires (!detail::SeriesForwardModule<Fn> &&
          (detail::SeriesStepCallable<Fn> || detail::DataFrameStepCallable<Fn>))
[[nodiscard]] inline Step step(std::string name,
                               std::string input_name,
                               std::string output_name,
                               Fn fn,
                               std::string key) {
  if (input_name.empty() || output_name.empty()) {
    ::pluss::throw_error("pluss::ai::procedure::step: input and output names must not be empty.");
  }
  StepOptions options{};
  options.capabilities = {StepCapability::Map};
  options.inputs = {std::move(input_name)};
  options.outputs = {output_name};
  options.component = key;
  auto runner = detail::make_callable_runner_(std::move(fn), std::move(output_name));
  return step(
      std::move(name),
      std::move(options),
      CustomPersist{
          .key = std::move(key),
          .fields = {{"external_ref", "registry"}},
      },
      std::move(runner));
}

template <class Fn>
requires (!detail::SeriesForwardModule<Fn> &&
          (detail::SeriesStepCallable<Fn> || detail::DataFrameStepCallable<Fn>))
inline void bind(std::string key, Fn fn) {
  if (key.empty()) {
    ::pluss::throw_error("pluss::ai::procedure::bind: key must not be empty.");
  }
  if (step_loaders_().contains(key) || custom_contracts_().contains(key)) {
    ::pluss::throw_error("pluss::ai::procedure::bind: duplicate contract key \"" + key + "\".");
  }
  const std::string bound_key = key;
  CustomContract contract{
      .key = bound_key,
      .load = [fn = std::move(fn), bound_key](std::string name,
                                              StepOptions options,
                                              std::string_view,
                                              const std::vector<std::pair<std::string, std::string>>&) mutable -> Step {
        if (options.inputs.size() != 1 || options.outputs.size() != 1) {
          ::pluss::throw_error("pluss::ai::procedure::bind(callable): expected exactly one input and one output.");
        }
        return step(std::move(name),
                    options.inputs.front(),
                    options.outputs.front(),
                    fn,
                    bound_key);
      },
  };
  custom_contracts_().emplace(bound_key, std::move(contract));
}

[[nodiscard]] inline Step step(std::string name,
                               std::string input_name,
                               std::string output_name,
                               lymph::workflow::Policy policy) {
  if (input_name.empty() || output_name.empty()) {
    ::pluss::throw_error("pluss::ai::procedure::step: input and output names must not be empty.");
  }
  StepOptions options{};
  options.capabilities = {StepCapability::Admit, StepCapability::Map};
  options.inputs = {std::move(input_name)};
  options.outputs = {output_name};
  options.persistable = true;
  options.component = "lymph::workflow::Policy";
  auto state = std::make_shared<lymph::workflow::Policy>(std::move(policy));
  StepPersistSpec persist{};
  persist.key = "lymph::workflow::Policy";
  persist.schema_version = 1;
  persist.component_schema_version = 2;
  persist.reloadable = true;
  persist.save = [state](std::string_view path) {
    lymph::workflow::save_policy(std::string(path), *state);
  };
  Step::Runner runner = [state = std::move(state), output = std::move(output_name)](
                            const std::vector<Artifact>& inputs, const RunOptions&) -> Artifact {
    if (inputs.size() != 1) {
      ::pluss::throw_error("pluss::ai::procedure::step(policy): expected exactly one input artifact.");
    }
    const Artifact& input = inputs.front();
    if (input.is<pluss::table::DataFrame>()) {
      return Artifact(output, Artifact::Value(state->apply(input.get<pluss::table::DataFrame>())));
    }
    if (input.is<lymph::workflow::Payload>()) {
      const lymph::workflow::Payload& payload = input.get<lymph::workflow::Payload>();
      if (!payload.has_frame()) {
        ::pluss::throw_error("pluss::ai::procedure::step(policy): Payload input must contain a DataFrame.");
      }
      return Artifact(output, Artifact::Value(payload.with_frame(state->apply(payload.frame()))));
    }
    ::pluss::throw_error("pluss::ai::procedure::step(policy): expected DataFrame or lymph::workflow::Payload input.");
  };
  return step(std::move(name), std::move(options), std::move(persist), std::move(runner));
}

[[nodiscard]] inline Step step(std::string name,
                               std::string input_name,
                               std::string output_name,
                               lymph::models::Model model,
                               lymph::TrainOptions train_options = {},
                               lymph::InferOptions infer_options = {},
                               lymph::AssessOptions assess_options = {}) {
  if (input_name.empty() || output_name.empty()) {
    ::pluss::throw_error("pluss::ai::procedure::step: input and output names must not be empty.");
  }
  StepOptions options{};
  options.capabilities = {StepCapability::Learn, StepCapability::Infer, StepCapability::Assess};
  options.inputs = {std::move(input_name)};
  options.outputs = {output_name};
  options.persistable = true;
  options.trained = model.trained();
  options.component = "lymph::models::Model";
  auto state = std::make_shared<lymph::models::Model>(std::move(model));
  StepPersistSpec persist{};
  persist.key = "lymph::models::Model";
  persist.schema_version = 1;
  persist.component_schema_version = 1;
  persist.reloadable = true;
  persist.fields = detail::lymph_train_fields_(train_options);
  {
    const auto infer_fields = detail::lymph_infer_fields_(infer_options);
    persist.fields.insert(persist.fields.end(), infer_fields.begin(), infer_fields.end());
    const auto assess_fields = detail::lymph_assess_fields_(assess_options);
    persist.fields.insert(persist.fields.end(), assess_fields.begin(), assess_fields.end());
  }
  persist.save = [state, train_options, assess_options](std::string_view path) {
    state->save(std::string(path));
    detail::sync_series_sidecar_(path, ".train_sample_weights.bin", train_options.sample_weights);
    detail::sync_series_sidecar_(path, ".train_class_weights.bin", train_options.class_weights);
    detail::sync_series_sidecar_(path, ".train_parameter_lower_bounds.bin", train_options.parameter_lower_bounds);
    detail::sync_series_sidecar_(path, ".train_parameter_upper_bounds.bin", train_options.parameter_upper_bounds);
    detail::sync_series_sidecar_(path, ".train_balance_counts.bin", train_options.balance.counts);
    detail::sync_series_sidecar_(path, ".assess_sample_weights.bin", assess_options.sample_weights);
  };
  Step::Runner runner = [state = std::move(state),
                         train_options = std::move(train_options),
                         infer_options = std::move(infer_options),
                         assess_options = std::move(assess_options),
                         output = std::move(output_name)](
                            const std::vector<Artifact>& inputs, const RunOptions& run_options) mutable -> Artifact {
    if (inputs.size() != 1) {
      ::pluss::throw_error("pluss::ai::procedure::step(model): expected exactly one input artifact.");
    }
    const Artifact& input = inputs.front();
    if (run_options.mode == ProcedureMode::Learn) {
      if (input.is<lymph::LabeledBatch>()) {
        state->learn_(input.get<lymph::LabeledBatch>(), train_options);
      } else if (input.is<lymph::workflow::Payload>()) {
        state->learn_(input.get<lymph::workflow::Payload>().labeled_batch(), train_options);
      } else {
        ::pluss::throw_error("pluss::ai::procedure::step(model): Learn mode expects LabeledBatch or Payload input.");
      }
      return Artifact(output, Artifact::Value(*state));
    }
    if (run_options.mode == ProcedureMode::Assess) {
      if (input.is<lymph::LabeledBatch>()) {
        return Artifact(output, Artifact::Value(state->assess(input.get<lymph::LabeledBatch>(), assess_options)));
      }
      if (input.is<lymph::workflow::Payload>()) {
        return Artifact(output, Artifact::Value(state->assess(input.get<lymph::workflow::Payload>().labeled_batch(), assess_options)));
      }
      ::pluss::throw_error("pluss::ai::procedure::step(model): Assess mode expects LabeledBatch or Payload input.");
    }
    if (input.is<lymph::FeatureBatch>()) {
      return Artifact(output, Artifact::Value(state->infer(input.get<lymph::FeatureBatch>(), infer_options)));
    }
    if (input.is<lymph::workflow::Payload>()) {
      return Artifact(output, Artifact::Value(state->infer(input.get<lymph::workflow::Payload>().feature_batch(), infer_options)));
    }
    if (input.is<pluss::table::DataFrame>()) {
      return Artifact(output, Artifact::Value(state->infer(input.get<pluss::table::DataFrame>(), infer_options)));
    }
    ::pluss::throw_error("pluss::ai::procedure::step(model): Infer mode expects FeatureBatch, Payload, or DataFrame input.");
  };
  return step(std::move(name), std::move(options), std::move(persist), std::move(runner));
}

[[nodiscard]] inline Step step(std::string name,
                               std::string input_name,
                               std::string output_name,
                               lymph::workflow::Workflow workflow,
                               lymph::TrainOptions train_options = {},
                               lymph::InferOptions infer_options = {},
                               lymph::AssessOptions assess_options = {}) {
  if (input_name.empty() || output_name.empty()) {
    ::pluss::throw_error("pluss::ai::procedure::step: input and output names must not be empty.");
  }
  StepOptions options{};
  options.capabilities = {StepCapability::Learn, StepCapability::Infer, StepCapability::Assess};
  options.inputs = {std::move(input_name)};
  options.outputs = {output_name};
  options.persistable = true;
  options.trained = workflow.trained();
  options.component = "lymph::workflow::Workflow";
  auto state = std::make_shared<lymph::workflow::Workflow>(std::move(workflow));
  StepPersistSpec persist{};
  persist.key = "lymph::workflow::Workflow";
  persist.schema_version = 1;
  persist.component_schema_version = 1;
  persist.reloadable = true;
  persist.fields = detail::lymph_train_fields_(train_options);
  {
    const auto infer_fields = detail::lymph_infer_fields_(infer_options);
    persist.fields.insert(persist.fields.end(), infer_fields.begin(), infer_fields.end());
    const auto assess_fields = detail::lymph_assess_fields_(assess_options);
    persist.fields.insert(persist.fields.end(), assess_fields.begin(), assess_fields.end());
  }
  persist.save = [state, train_options, assess_options](std::string_view path) {
    state->save(std::string(path));
    detail::sync_series_sidecar_(path, ".train_sample_weights.bin", train_options.sample_weights);
    detail::sync_series_sidecar_(path, ".train_class_weights.bin", train_options.class_weights);
    detail::sync_series_sidecar_(path, ".train_parameter_lower_bounds.bin", train_options.parameter_lower_bounds);
    detail::sync_series_sidecar_(path, ".train_parameter_upper_bounds.bin", train_options.parameter_upper_bounds);
    detail::sync_series_sidecar_(path, ".train_balance_counts.bin", train_options.balance.counts);
    detail::sync_series_sidecar_(path, ".assess_sample_weights.bin", assess_options.sample_weights);
  };
  Step::Runner runner = [state = std::move(state),
                         train_options = std::move(train_options),
                         infer_options = std::move(infer_options),
                         assess_options = std::move(assess_options),
                         output = std::move(output_name)](
                            const std::vector<Artifact>& inputs, const RunOptions& run_options) mutable -> Artifact {
    if (inputs.size() != 1) {
      ::pluss::throw_error("pluss::ai::procedure::step(workflow): expected exactly one input artifact.");
    }
    const Artifact& input = inputs.front();
    if (run_options.mode == ProcedureMode::Learn) {
      if (input.is<lymph::workflow::Payload>()) {
        state->learn_(input.get<lymph::workflow::Payload>(), train_options);
      } else if (input.is<lymph::LabeledBatch>()) {
        state->learn_(input.get<lymph::LabeledBatch>(), train_options);
      } else {
        ::pluss::throw_error("pluss::ai::procedure::step(workflow): Learn mode expects Payload or LabeledBatch input.");
      }
      return Artifact(output, Artifact::Value(*state));
    }
    if (run_options.mode == ProcedureMode::Assess) {
      if (input.is<lymph::workflow::Payload>()) {
        return Artifact(output, Artifact::Value(state->assess(input.get<lymph::workflow::Payload>(), assess_options)));
      }
      if (input.is<lymph::LabeledBatch>()) {
        return Artifact(output, Artifact::Value(state->assess(input.get<lymph::LabeledBatch>(), assess_options)));
      }
      ::pluss::throw_error("pluss::ai::procedure::step(workflow): Assess mode expects Payload or LabeledBatch input.");
    }
    if (input.is<lymph::workflow::Payload>()) {
      return Artifact(output, Artifact::Value(state->infer(input.get<lymph::workflow::Payload>(), infer_options)));
    }
    if (input.is<lymph::FeatureBatch>()) {
      return Artifact(output, Artifact::Value(state->infer(input.get<lymph::FeatureBatch>(), infer_options)));
    }
    if (input.is<pluss::table::DataFrame>()) {
      return Artifact(output, Artifact::Value(state->infer(input.get<pluss::table::DataFrame>(), infer_options)));
    }
    ::pluss::throw_error("pluss::ai::procedure::step(workflow): Infer mode expects Payload, FeatureBatch, or DataFrame input.");
  };
  return step(std::move(name), std::move(options), std::move(persist), std::move(runner));
}

[[nodiscard]] inline Step step(std::string name,
                               std::string input_name,
                               std::string output_name,
                               ice::Model& model,
                               ice::LossKind assess_loss = ice::LossKind::MSE,
                               ice::TrainOptions train_options = {}) {
  auto state = std::shared_ptr<ice::Model>(&model, [](ice::Model*) {});
  return detail::make_ice_step_(
      std::move(name),
      std::move(input_name),
      std::move(output_name),
      std::move(state),
      {},
      assess_loss,
      std::move(train_options));
}

template <class IceModel>
requires std::derived_from<std::remove_cvref_t<IceModel>, ice::Model>
[[nodiscard]] inline Step step(std::string name,
                               std::string input_name,
                               std::string output_name,
                               IceModel model,
                               std::string restore_key,
                               ice::LossKind assess_loss = ice::LossKind::MSE,
                               ice::TrainOptions train_options = {}) {
  auto state = std::make_shared<std::remove_cvref_t<IceModel>>(std::move(model));
  return detail::make_ice_step_(
      std::move(name),
      std::move(input_name),
      std::move(output_name),
      std::static_pointer_cast<ice::Model>(std::move(state)),
      std::move(restore_key),
      assess_loss,
      std::move(train_options));
}

[[nodiscard]] inline Step step(std::string name,
                               std::string input_name,
                               std::string output_name,
                               nodeb::Workflow workflow,
                               nodeb::TrainOptions train_options = {},
                               nodeb::AssessOptions assess_options = {}) {
  if (input_name.empty() || output_name.empty()) {
    ::pluss::throw_error("pluss::ai::procedure::step: input and output names must not be empty.");
  }
  StepOptions options{};
  options.capabilities = {StepCapability::Learn, StepCapability::Infer, StepCapability::Assess};
  options.inputs = {std::move(input_name)};
  options.outputs = {output_name};
  options.persistable = true;
  options.component = "nodeb::Workflow";
  auto state = std::make_shared<nodeb::Workflow>(std::move(workflow));
  StepPersistSpec persist{};
  persist.key = "nodeb::Workflow";
  persist.schema_version = 1;
  persist.component_schema_version = 0;
  persist.reloadable = true;
  persist.fields = {
      {"allow_empty_docs", train_options.allow_empty_docs ? "1" : "0"},
      {"train_seed", std::to_string(train_options.random.seed)},
      {"train_deterministic", train_options.random.deterministic ? "1" : "0"},
      {"require_sentences", assess_options.require_sentences ? "1" : "0"},
  };
  persist.save = [state](std::string_view path) {
    state->save(path);
  };
  Step::Runner runner = [state = std::move(state),
                         train_options = std::move(train_options),
                         assess_options = std::move(assess_options),
                         output = std::move(output_name)](
                            const std::vector<Artifact>& inputs, const RunOptions& run_options) mutable -> Artifact {
    if (inputs.size() != 1) {
      ::pluss::throw_error("pluss::ai::procedure::step(nodeb::Workflow): expected exactly one input artifact.");
    }
    const Artifact& input = inputs.front();
    if (run_options.mode == ProcedureMode::Learn) {
      if (input.is<std::vector<std::string>>()) {
        state->learn(input.get<std::vector<std::string>>(), train_options);
        return Artifact(output, Artifact::Value(*state));
      }
      if (input.is<pluss::Note>()) {
        state->learn(input.get<pluss::Note>(), train_options);
        return Artifact(output, Artifact::Value(*state));
      }
      if (input.is<std::vector<nodeb::Doc>>()) {
        state->learn(input.get<std::vector<nodeb::Doc>>(), train_options);
        return Artifact(output, Artifact::Value(*state));
      }
      ::pluss::throw_error("pluss::ai::procedure::step(nodeb::Workflow): Learn mode expects text vector, Note, or Doc vector.");
    }
    if (run_options.mode == ProcedureMode::Assess) {
      if (input.is<std::vector<std::string>>()) {
        return Artifact(output, Artifact::Value(state->assess(input.get<std::vector<std::string>>(), assess_options)));
      }
      if (input.is<pluss::Note>()) {
        return Artifact(output, Artifact::Value(state->assess(input.get<pluss::Note>(), assess_options)));
      }
      if (input.is<std::vector<nodeb::Doc>>()) {
        return Artifact(output, Artifact::Value(state->assess(input.get<std::vector<nodeb::Doc>>(), assess_options)));
      }
      ::pluss::throw_error("pluss::ai::procedure::step(nodeb::Workflow): Assess mode expects text vector, Note, or Doc vector.");
    }
    if (input.is<std::string>()) {
      return Artifact(output, Artifact::Value(state->infer(input.get<std::string>())));
    }
    if (input.is<std::vector<std::string>>()) {
      return Artifact(output, Artifact::Value(state->infer(input.get<std::vector<std::string>>())));
    }
    if (input.is<pluss::Note>()) {
      return Artifact(output, Artifact::Value(state->infer(input.get<pluss::Note>())));
    }
    ::pluss::throw_error("pluss::ai::procedure::step(nodeb::Workflow): Infer mode expects string, text vector, or Note input.");
  };
  return step(std::move(name), std::move(options), std::move(persist), std::move(runner));
}

[[nodiscard]] inline Step step(std::string name, SinkSpec sink_spec) {
  if (sink_spec.target.empty()) {
    ::pluss::throw_error("pluss::ai::procedure::step: sink target must not be empty.");
  }
  StepOptions options{};
  options.capabilities = {StepCapability::Sink};
  options.persistable = true;
  options.component = sink_spec.target;
  StepPersistSpec persist{};
  persist.key = "procedure::Sink";
  persist.schema_version = 1;
  persist.reloadable = true;
  persist.fields = {{"target", sink_spec.target}};
  return Step(std::move(name), std::move(options), std::move(persist), {});
}

[[nodiscard]] inline SinkSpec sink(std::string target) {
  if (target.empty()) {
    ::pluss::throw_error("pluss::ai::procedure::sink: target must not be empty.");
  }
  return SinkSpec{.target = std::move(target)};
}

[[nodiscard]] inline Link link(std::string from,
                               std::string to,
                               std::string from_output = {},
                               std::string to_input = {}) {
  if (from.empty() || to.empty()) {
    ::pluss::throw_error("pluss::ai::procedure::link: endpoints must not be empty.");
  }
  return Link{
      .from = std::move(from),
      .to = std::move(to),
      .from_output = std::move(from_output),
      .to_input = std::move(to_input),
  };
}

template <class T>
[[nodiscard]] inline Artifact input(std::string name, T value) {
  if (name.empty()) {
    ::pluss::throw_error("pluss::ai::procedure::input: name must not be empty.");
  }
  return Artifact(std::move(name), Artifact::Value(std::move(value)));
}

[[nodiscard]] inline Procedure make(std::vector<Step> steps,
                                    std::vector<Link> links = {},
                                    ProcedureOptions options = {}) {
  return Procedure(std::move(steps), std::move(links), options);
}

[[nodiscard]] inline Procedure make(std::initializer_list<Step> steps,
                                    std::initializer_list<Link> links = {},
                                    ProcedureOptions options = {}) {
  return make(std::vector<Step>(steps.begin(), steps.end()),
              std::vector<Link>(links.begin(), links.end()),
              options);
}

[[nodiscard]] inline std::unordered_map<std::string, StepLoader>& step_loaders_();
[[nodiscard]] inline std::unordered_map<std::string, CustomContract>& custom_contracts_() {
  static std::unordered_map<std::string, CustomContract> contracts{};
  return contracts;
}

[[nodiscard]] inline std::unordered_map<std::string, IceFactory>& ice_factories_() {
  static std::unordered_map<std::string, IceFactory> factories{};
  return factories;
}

inline void bind(CustomContract contract) {
  if (contract.key.empty()) {
    ::pluss::throw_error("pluss::ai::procedure::bind: key must not be empty.");
  }
  if (!contract.load) {
    ::pluss::throw_error("pluss::ai::procedure::bind: load hook must be callable.");
  }
  if (step_loaders_().contains(contract.key) || custom_contracts_().contains(contract.key)) {
    ::pluss::throw_error("pluss::ai::procedure::bind: duplicate contract key \"" + contract.key + "\".");
  }
  custom_contracts_().emplace(contract.key, std::move(contract));
}

inline void bind(std::string key, StepLoader loader) {
  bind(CustomContract{
      .key = std::move(key),
      .load = std::move(loader),
  });
}

inline void check_custom_contract_(std::string_view key,
                                   std::int64_t persist_schema,
                                   std::int64_t component_schema,
                                   const std::vector<std::pair<std::string, std::string>>& fields) {
  const auto found = custom_contracts_().find(std::string(key));
  if (found == custom_contracts_().end()) {
    ::pluss::throw_error("pluss::ai::procedure::load: no custom contract bound for key \"" + std::string(key) + "\".");
  }
  if (found->second.compatible) {
    if (const std::optional<std::string> why =
            found->second.compatible(persist_schema, component_schema, fields);
        why.has_value()) {
      ::pluss::throw_error("pluss::ai::procedure::load: custom contract \"" + found->second.key +
                           "\" rejected persisted step: " + *why);
    }
    return;
  }
  if (persist_schema != found->second.persist_schema_version) {
    ::pluss::throw_error("pluss::ai::procedure::load: custom contract \"" + found->second.key +
                         "\" persist schema mismatch.");
  }
  if (component_schema != found->second.component_schema_version) {
    ::pluss::throw_error("pluss::ai::procedure::load: custom contract \"" + found->second.key +
                         "\" component schema mismatch.");
  }
}

template <class Factory>
requires std::derived_from<std::remove_cvref_t<std::invoke_result_t<Factory&>>, ice::Model>
inline void bind_ice(std::string key, Factory factory) {
  if (key.empty()) {
    ::pluss::throw_error("pluss::ai::procedure::bind_ice: key must not be empty.");
  }
  if (ice_factories_().contains(key)) {
    ::pluss::throw_error("pluss::ai::procedure::bind_ice: duplicate key \"" + key + "\".");
  }
  ice_factories_().emplace(
      std::move(key),
      [factory = std::move(factory)]() mutable -> std::shared_ptr<ice::Model> {
        using Model = std::remove_cvref_t<std::invoke_result_t<Factory&>>;
        return std::make_shared<Model>(factory());
      });
}

[[nodiscard]] inline std::unordered_map<std::string, StepLoader>& step_loaders_() {
  static std::unordered_map<std::string, StepLoader> loaders{
      {"lymph::workflow::Policy",
       [](std::string name,
          StepOptions options,
          std::string_view path,
          const std::vector<std::pair<std::string, std::string>>&) -> Step {
         return step(std::move(name),
                     options.inputs.at(0),
                     options.outputs.at(0),
                     lymph::workflow::load_policy(std::string(path)));
       }},
      {"lymph::models::Model",
       [](std::string name,
          StepOptions options,
          std::string_view path,
          const std::vector<std::pair<std::string, std::string>>& fields) -> Step {
         lymph::TrainOptions train = detail::load_lymph_train_options_(fields, path);
         lymph::InferOptions infer = detail::load_lymph_infer_options_(fields);
         lymph::AssessOptions assess = detail::load_lymph_assess_options_(fields, path);
         return step(std::move(name),
                     options.inputs.at(0),
                     options.outputs.at(0),
                     lymph::models::Model::load(std::string(path)),
                     std::move(train),
                     std::move(infer),
                     std::move(assess));
       }},
      {"lymph::workflow::Workflow",
       [](std::string name,
          StepOptions options,
          std::string_view path,
          const std::vector<std::pair<std::string, std::string>>& fields) -> Step {
         lymph::TrainOptions train = detail::load_lymph_train_options_(fields, path);
         lymph::InferOptions infer = detail::load_lymph_infer_options_(fields);
         lymph::AssessOptions assess = detail::load_lymph_assess_options_(fields, path);
         return step(std::move(name),
                     options.inputs.at(0),
                     options.outputs.at(0),
                     lymph::workflow::load(std::string(path)),
                     std::move(train),
                     std::move(infer),
                     std::move(assess));
       }},
      {"nodeb::Workflow",
       [](std::string name,
          StepOptions options,
          std::string_view path,
          const std::vector<std::pair<std::string, std::string>>& fields) -> Step {
         nodeb::TrainOptions train{};
         train.allow_empty_docs = detail::parse_bool_(
             detail::field_or_(fields, "allow_empty_docs", "1"),
             "pluss::ai::procedure::load(nodeb.allow_empty_docs)");
         train.random.seed = static_cast<std::uint64_t>(detail::parse_i64_(
             detail::field_or_(fields, "train_seed", "0"),
             "pluss::ai::procedure::load(nodeb.train_seed)"));
         train.random.deterministic = detail::parse_bool_(
             detail::field_or_(fields, "train_deterministic", "1"),
             "pluss::ai::procedure::load(nodeb.train_deterministic)");
         nodeb::AssessOptions assess{};
         assess.require_sentences = detail::parse_bool_(
             detail::field_or_(fields, "require_sentences", "0"),
             "pluss::ai::procedure::load(nodeb.require_sentences)");
         return step(std::move(name),
                     options.inputs.at(0),
                     options.outputs.at(0),
                     nodeb::Workflow::load(path),
                     train,
                     assess);
       }},
      {"ice::Model",
       [](std::string name,
          StepOptions options,
          std::string_view path,
          const std::vector<std::pair<std::string, std::string>>& fields) -> Step {
         const std::string restore_key = detail::field_or_(fields, "ice.restore", {});
         if (restore_key.empty()) {
           ::pluss::throw_error("pluss::ai::procedure::load(ice::Model): missing restore key.");
         }
         const auto found = ice_factories_().find(restore_key);
         if (found == ice_factories_().end()) {
           ::pluss::throw_error("pluss::ai::procedure::load(ice::Model): no factory bound for key \"" + restore_key + "\".");
         }
         std::shared_ptr<ice::Model> state = found->second();
         if (!state) {
           ::pluss::throw_error("pluss::ai::procedure::load(ice::Model): bound factory returned null.");
         }
         state->load(path);
         return detail::make_ice_step_(
             std::move(name),
             options.inputs.at(0),
             options.outputs.at(0),
             std::move(state),
             restore_key,
             detail::parse_enum_<ice::LossKind>(
                 detail::field_or_(fields, "ice.assess_loss", detail::enum_text_(ice::LossKind::MSE)),
                 "pluss::ai::procedure::load(ice.assess_loss)"),
             detail::load_ice_train_options_(fields));
       }},
      {"procedure::Sink",
       [](std::string name,
          StepOptions,
          std::string_view,
          const std::vector<std::pair<std::string, std::string>>& fields) -> Step {
         return step(std::move(name), sink(detail::field_or_(fields, "target")));
       }},
  };
  return loaders;
}

inline void save_bundle_(const Procedure& proc, std::string_view path) {
  proc.check();
  const std::filesystem::path bundle(path);
  if (bundle.empty()) {
    ::pluss::throw_error("pluss::ai::Procedure::save: path must not be empty.");
  }
  if (std::filesystem::exists(bundle) && !std::filesystem::is_directory(bundle)) {
    ::pluss::throw_error("pluss::ai::Procedure::save: path \"" + bundle.string() + "\" must be a directory bundle.");
  }
  std::filesystem::create_directories(bundle / "steps");
  std::filesystem::create_directories(bundle / "audit");

  std::ofstream manifest(bundle / "manifest.plussproc", std::ios::trunc);
  if (!manifest) {
    ::pluss::throw_error("pluss::ai::Procedure::save: could not open manifest for writing.");
  }

  manifest << "PLUSS_PROCEDURE_V1\n";
  detail::write_line_(manifest, "schema", std::to_string(detail::kCurrentProcedureSchemaVersion));
  detail::write_line_(manifest, "producer", detail::kProcedureProducer);
  detail::write_line_(manifest, "producer_version", detail::kProcedureProducerVersion);
  detail::write_line_(manifest, "saved_at_epoch_us",
                      std::to_string(static_cast<std::int64_t>(
                          std::chrono::duration_cast<std::chrono::microseconds>(
                              std::chrono::system_clock::now().time_since_epoch()).count())));
  detail::write_line_(manifest, "workers", std::to_string(proc.options().workers));
  detail::write_line_(manifest, "schedule", std::to_string(static_cast<std::int64_t>(proc.options().schedule)));
  detail::write_line_(manifest, "cache", std::to_string(static_cast<std::int64_t>(proc.options().cache)));
  detail::write_line_(manifest, "fail_fast", detail::bool_text_(proc.options().fail_fast));
  detail::write_line_(manifest, "validate_on_make", detail::bool_text_(proc.options().validate_on_make));
  detail::write_line_(manifest, "step_count", std::to_string(proc.steps().size()));

  std::int64_t persistable_count = 0;
  std::int64_t reloadable_count = 0;
  std::int64_t custom_count = 0;
  std::int64_t external_ref_count = 0;
  std::int64_t time_contract_count = 0;
  std::int64_t future_contract_count = 0;
  for (const Step& step_ref : proc.steps()) {
    persistable_count += step_ref.persistable() ? 1 : 0;
    reloadable_count += step_ref.reloadable() ? 1 : 0;
    custom_count += step_ref.custom() ? 1 : 0;
    external_ref_count += (step_ref.custom() && !step_ref.can_save_component()) ? 1 : 0;
    time_contract_count += (step_ref.options().requires_time || !step_ref.options().time_feature.empty()) ? 1 : 0;
    future_contract_count += step_ref.options().learns_from_future ? 1 : 0;
  }
  const std::string external_code_policy = external_ref_count > 0 ? "registry" : "reject";
  detail::write_line_(manifest, "external_code_policy", external_code_policy);
  detail::write_line_(manifest, "persistable_steps", std::to_string(persistable_count));
  detail::write_line_(manifest, "reloadable_steps", std::to_string(reloadable_count));
  detail::write_line_(manifest, "custom_steps", std::to_string(custom_count));
  detail::write_line_(manifest, "external_ref_steps", std::to_string(external_ref_count));
  detail::write_line_(manifest, "time_contract_steps", std::to_string(time_contract_count));
  detail::write_line_(manifest, "future_contract_steps", std::to_string(future_contract_count));

  std::vector<std::string> checksum_lines{};
  checksum_lines.reserve(proc.steps().size());
  std::vector<std::string> contract_lines{};
  contract_lines.reserve(proc.steps().size() + 1);
  contract_lines.push_back("step\tcomponent\tpersist_key\tpersist_schema\tcomponent_schema\treloadable\tcustom\tfrozen\trequires_time\ttime_feature\tlearns_from_future\tcan_change_row_count");
  std::vector<std::string> artifact_lines{};
  artifact_lines.reserve(proc.steps().size() + 1);
  artifact_lines.push_back("step\tartifact\tchecksum\ttrained\tcapabilities");
  for (std::size_t i = 0; i < proc.steps().size(); ++i) {
    const Step& current = proc.steps()[i];
    if (!current.persistable()) {
      ::pluss::throw_error("pluss::ai::Procedure::save: step \"" + current.name() + "\" is not persistable.");
    }
    if (!current.reloadable()) {
      ::pluss::throw_error("pluss::ai::Procedure::save: step \"" + current.name() +
                           "\" has no reload contract for persistence.");
    }

    std::string artifact_rel{};
    std::string checksum{};
    if (current.can_save_component()) {
      std::ostringstream stem;
      stem << "steps/" << i << "_" << detail::sanitize_step_name_(current.name());
      artifact_rel = stem.str();
      const std::filesystem::path artifact_base = bundle / artifact_rel;
      current.save_component(artifact_base.string());
      checksum = detail::artifact_checksum_(artifact_base);
      if (checksum.empty()) {
        ::pluss::throw_error("pluss::ai::Procedure::save: step \"" + current.name() +
                             "\" did not produce any persisted component artifact.");
      }
    }

    manifest << "step\n";
    detail::write_line_(manifest, "id", std::to_string(static_cast<std::int64_t>(i)));
    detail::write_line_(manifest, "name", current.name());
    detail::write_line_(manifest, "component", current.component());
    detail::write_line_(manifest, "capabilities", detail::join_capabilities_(current.options().capabilities));
    detail::write_line_(manifest, "persistable", detail::bool_text_(current.persistable()));
    detail::write_line_(manifest, "trained", detail::bool_text_(current.trained()));
    detail::write_line_(manifest, "can_change_row_count", detail::bool_text_(current.can_change_row_count()));
    detail::write_line_(manifest, "time_feature", current.options().time_feature);
    detail::write_line_(manifest, "requires_time", detail::bool_text_(current.options().requires_time));
    detail::write_line_(manifest, "learns_from_future", detail::bool_text_(current.options().learns_from_future));
    detail::write_line_(manifest, "persist_key", current.persist_key());
    detail::write_line_(manifest, "persist_schema", std::to_string(current.persist_schema_version()));
    detail::write_line_(manifest, "component_schema", std::to_string(current.component_schema_version()));
    detail::write_line_(manifest, "reloadable", detail::bool_text_(current.reloadable()));
    detail::write_line_(manifest, "frozen", detail::bool_text_(current.frozen()));
    detail::write_line_(manifest, "custom", detail::bool_text_(current.custom()));
    detail::write_line_(manifest, "artifact", artifact_rel);
    detail::write_line_(manifest, "checksum", checksum);
    for (const std::string& input_name : current.inputs()) {
      detail::write_line_(manifest, "input", input_name);
    }
    for (const std::string& output_name : current.outputs()) {
      detail::write_line_(manifest, "output", output_name);
    }
    for (const std::string& column : current.options().require_columns) {
      detail::write_line_(manifest, "require_column", column);
    }
    for (const auto& field : current.persist_fields()) {
      detail::write_line_(manifest, "field", field.first + "=" + field.second);
    }
    manifest << "endstep\n";

    checksum_lines.push_back(current.name() + "\t" + checksum);
    contract_lines.push_back(current.name() + "\t" +
                             current.component() + "\t" +
                             current.persist_key() + "\t" +
                             std::to_string(current.persist_schema_version()) + "\t" +
                             std::to_string(current.component_schema_version()) + "\t" +
                             detail::bool_text_(current.reloadable()) + "\t" +
                             detail::bool_text_(current.custom()) + "\t" +
                             detail::bool_text_(current.frozen()) + "\t" +
                             detail::bool_text_(current.options().requires_time) + "\t" +
                             current.options().time_feature + "\t" +
                             detail::bool_text_(current.options().learns_from_future) + "\t" +
                             detail::bool_text_(current.can_change_row_count()));
    artifact_lines.push_back(current.name() + "\t" +
                             artifact_rel + "\t" +
                             checksum + "\t" +
                             detail::bool_text_(current.trained()) + "\t" +
                             detail::join_capabilities_(current.options().capabilities));
  }

  detail::write_line_(manifest, "link_count", std::to_string(proc.links().size()));
  for (const Link& link_ref : proc.links()) {
    manifest << "link\n";
    detail::write_line_(manifest, "from", link_ref.from);
    detail::write_line_(manifest, "to", link_ref.to);
    detail::write_line_(manifest, "from_output", link_ref.from_output);
    detail::write_line_(manifest, "to_input", link_ref.to_input);
    manifest << "endlink\n";
  }

  std::ofstream checksums(bundle / "audit" / "checksums.txt", std::ios::trunc);
  for (const std::string& line : checksum_lines) {
    checksums << line << "\n";
  }
  std::ofstream lineage(bundle / "audit" / "lineage.txt", std::ios::trunc);
  for (const Link& link_ref : proc.links()) {
    lineage << link_ref.from << "\t" << link_ref.to << "\t"
            << link_ref.from_output << "\t" << link_ref.to_input << "\n";
  }
  std::ofstream contracts(bundle / "audit" / "contracts.txt", std::ios::trunc);
  for (const std::string& line : contract_lines) {
    contracts << line << "\n";
  }
  std::ofstream artifacts(bundle / "audit" / "artifacts.txt", std::ios::trunc);
  for (const std::string& line : artifact_lines) {
    artifacts << line << "\n";
  }
  std::ofstream summary(bundle / "audit" / "summary.txt", std::ios::trunc);
  summary << "producer=" << detail::kProcedureProducer << "\n";
  summary << "producer_version=" << detail::kProcedureProducerVersion << "\n";
  summary << "schema=" << detail::kCurrentProcedureSchemaVersion << "\n";
  summary << "steps=" << proc.steps().size() << "\n";
  summary << "links=" << proc.links().size() << "\n";
  summary << "persistable_steps=" << persistable_count << "\n";
  summary << "reloadable_steps=" << reloadable_count << "\n";
  summary << "custom_steps=" << custom_count << "\n";
  summary << "external_ref_steps=" << external_ref_count << "\n";
  summary << "time_contract_steps=" << time_contract_count << "\n";
  summary << "future_contract_steps=" << future_contract_count << "\n";
  summary << "workers=" << proc.options().workers << "\n";
  summary << "schedule=" << static_cast<std::int64_t>(proc.options().schedule) << "\n";
  summary << "cache=" << static_cast<std::int64_t>(proc.options().cache) << "\n";
  summary << "fail_fast=" << detail::bool_text_(proc.options().fail_fast) << "\n";
  summary << "validate_on_make=" << detail::bool_text_(proc.options().validate_on_make) << "\n";
  summary << "external_code_policy=" << external_code_policy << "\n";
}

[[nodiscard]] inline Procedure load(const std::string& path) {
  auto& builtin = step_loaders_();
  (void)builtin;
  const std::filesystem::path bundle(path);
  std::ifstream manifest(bundle / "manifest.plussproc");
  if (!manifest) {
    ::pluss::throw_error("pluss::ai::procedure::load: could not open \"" +
                         (bundle / "manifest.plussproc").string() + "\".");
  }

  std::string line{};
  if (!std::getline(manifest, line) || line != "PLUSS_PROCEDURE_V1") {
    ::pluss::throw_error("pluss::ai::procedure::load: unsupported procedure manifest header.");
  }

  ProcedureOptions options{};
  std::vector<Step> steps{};
  std::vector<Link> links{};
  std::int64_t expected_steps = -1;
  std::int64_t expected_links = -1;
  bool saw_schema = false;

  while (std::getline(manifest, line)) {
    if (line.empty()) continue;
    if (line == "step") {
      std::string name{};
      StepOptions step_options{};
      std::string persist_key{};
      std::int64_t persist_schema = 1;
      std::int64_t component_schema = 0;
      bool reloadable = false;
      bool frozen = false;
      bool custom = false;
      std::string artifact_rel{};
      std::string checksum{};
      std::vector<std::pair<std::string, std::string>> fields{};
      while (std::getline(manifest, line) && line != "endstep") {
        const auto [key, value] = detail::split_line_(line, "pluss::ai::procedure::load(step)");
        if (key == "name") name = value;
        else if (key == "component") step_options.component = value;
        else if (key == "capabilities") step_options.capabilities = detail::split_capabilities_(value);
        else if (key == "persistable") step_options.persistable = detail::parse_bool_(value, "pluss::ai::procedure::load(persistable)");
        else if (key == "trained") step_options.trained = detail::parse_bool_(value, "pluss::ai::procedure::load(trained)");
        else if (key == "can_change_row_count") step_options.can_change_row_count = detail::parse_bool_(value, "pluss::ai::procedure::load(can_change_row_count)");
        else if (key == "time_feature") step_options.time_feature = value;
        else if (key == "requires_time") step_options.requires_time = detail::parse_bool_(value, "pluss::ai::procedure::load(requires_time)");
        else if (key == "learns_from_future") step_options.learns_from_future = detail::parse_bool_(value, "pluss::ai::procedure::load(learns_from_future)");
        else if (key == "persist_key") persist_key = value;
        else if (key == "persist_schema") persist_schema = detail::parse_i64_(value, "pluss::ai::procedure::load(persist_schema)");
        else if (key == "component_schema") component_schema = detail::parse_i64_(value, "pluss::ai::procedure::load(component_schema)");
        else if (key == "reloadable") reloadable = detail::parse_bool_(value, "pluss::ai::procedure::load(reloadable)");
        else if (key == "frozen") frozen = detail::parse_bool_(value, "pluss::ai::procedure::load(frozen)");
        else if (key == "custom") custom = detail::parse_bool_(value, "pluss::ai::procedure::load(custom)");
        else if (key == "artifact") artifact_rel = value;
        else if (key == "checksum") checksum = value;
        else if (key == "input") step_options.inputs.push_back(value);
        else if (key == "output") step_options.outputs.push_back(value);
        else if (key == "require_column") step_options.require_columns.push_back(value);
        else if (key == "field") {
          const std::size_t split = value.find('=');
          if (split == std::string::npos) {
            ::pluss::throw_error("pluss::ai::procedure::load: malformed persistence field \"" + value + "\".");
          }
          fields.emplace_back(value.substr(0, split), value.substr(split + 1));
        }
      }
      if (name.empty()) {
        ::pluss::throw_error("pluss::ai::procedure::load: encountered a step with no name.");
      }
      if (persist_key.empty()) {
        ::pluss::throw_error("pluss::ai::procedure::load: step \"" + name + "\" has no persistence key.");
      }
      if (!artifact_rel.empty()) {
        const std::filesystem::path artifact_base = bundle / artifact_rel;
        const std::string actual = detail::artifact_checksum_(artifact_base);
        if (actual != checksum) {
          ::pluss::throw_error("pluss::ai::procedure::load: checksum mismatch for step \"" + name + "\".");
        }
      }

      StepLoader loader{};
      if (const auto found = step_loaders_().find(persist_key); found != step_loaders_().end()) {
        loader = found->second;
      } else if (const auto found = custom_contracts_().find(persist_key); found != custom_contracts_().end()) {
        check_custom_contract_(persist_key, persist_schema, component_schema, fields);
        loader = found->second.load;
      } else {
        ::pluss::throw_error("pluss::ai::procedure::load: no loader registered for step key \"" + persist_key + "\".");
      }
      Step loaded = loader(name, step_options, (bundle / artifact_rel).string(), fields);
      detail::validate_loaded_contract_(loaded,
                                        step_options,
                                        persist_key,
                                        persist_schema,
                                        component_schema,
                                        reloadable,
                                        frozen,
                                        custom);
      loaded.options().require_columns = step_options.require_columns;
      loaded.options().time_feature = step_options.time_feature;
      loaded.options().requires_time = step_options.requires_time;
      loaded.options().learns_from_future = step_options.learns_from_future;
      loaded.options().can_change_row_count = step_options.can_change_row_count;
      steps.push_back(std::move(loaded));
      continue;
    }
    if (line == "link") {
      Link link_ref{};
      while (std::getline(manifest, line) && line != "endlink") {
        const auto [key, value] = detail::split_line_(line, "pluss::ai::procedure::load(link)");
        if (key == "from") link_ref.from = value;
        else if (key == "to") link_ref.to = value;
        else if (key == "from_output") link_ref.from_output = value;
        else if (key == "to_input") link_ref.to_input = value;
      }
      links.push_back(std::move(link_ref));
      continue;
    }

    const auto [key, value] = detail::split_line_(line, "pluss::ai::procedure::load");
    if (key == "schema") {
      saw_schema = true;
      const std::int64_t schema = detail::parse_i64_(value, "pluss::ai::procedure::load(schema)");
      if (schema < detail::kMinLoadableProcedureSchemaVersion ||
          schema > detail::kCurrentProcedureSchemaVersion) {
        ::pluss::throw_error("pluss::ai::procedure::load: unsupported manifest schema version.");
      }
    } else if (key == "workers") {
      options.workers = detail::parse_i64_(value, "pluss::ai::procedure::load(workers)");
    } else if (key == "schedule") {
      options.schedule = static_cast<ProcedureSchedule>(
          detail::parse_i64_(value, "pluss::ai::procedure::load(schedule)"));
    } else if (key == "cache") {
      options.cache = static_cast<ProcedureCache>(
          detail::parse_i64_(value, "pluss::ai::procedure::load(cache)"));
    } else if (key == "fail_fast") {
      options.fail_fast = detail::parse_bool_(value, "pluss::ai::procedure::load(fail_fast)");
    } else if (key == "validate_on_make") {
      options.validate_on_make = detail::parse_bool_(value, "pluss::ai::procedure::load(validate_on_make)");
    } else if (key == "step_count") {
      expected_steps = detail::parse_i64_(value, "pluss::ai::procedure::load(step_count)");
    } else if (key == "link_count") {
      expected_links = detail::parse_i64_(value, "pluss::ai::procedure::load(link_count)");
    }
  }

  if (!saw_schema) {
    ::pluss::throw_error("pluss::ai::procedure::load: manifest schema is missing.");
  }

  if (expected_steps >= 0 && expected_steps != static_cast<std::int64_t>(steps.size())) {
    ::pluss::throw_error("pluss::ai::procedure::load: step count mismatch.");
  }
  if (expected_links >= 0 && expected_links != static_cast<std::int64_t>(links.size())) {
    ::pluss::throw_error("pluss::ai::procedure::load: link count mismatch.");
  }

  return make(std::move(steps), std::move(links), options);
}

} // namespace procedure

inline void Procedure::save(std::string_view path) const {
  procedure::save_bundle_(*this, path);
}

} // namespace pluss::ai
