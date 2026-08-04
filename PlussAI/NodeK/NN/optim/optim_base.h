#pragma once

#include "NodeK/NN/modules/Module.h"
#include "CodeM/core/SeriesTape.h"
#include "MSharp/RangeView.h"
#include "pluss/codem/codem.h"
#include "pluss/nn_detail.h"

#include <algorithm>
#include <concepts>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <span>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace pluss::ai::nn::optim {

enum class GradientClipMode {
  None,
  ByValue,
  ByNorm,
};

struct GradientClipOptions {
  GradientClipMode mode{GradientClipMode::None};
  double threshold{0.0};
  double epsilon{1e-12};
};

enum class UpdateLane {
  Auto,
  Loop,
  Foreach,
  Batched,
  Fused,
};

struct ExecutionOptions {
  UpdateLane lane{UpdateLane::Auto};
  bool strict{false};
  bool capturable{false};
};

struct StepDiagnosticsOptions {
  // Fail when at least one trainable parameter exists but none received sensitivity.
  bool fail_if_no_sensitivity{false};
  // Optional minimum fraction [0,1] of trainable parameters that must receive sensitivity.
  double minimum_tracked_coverage{0.0};
  // Fail when a provided sensitivity/gradient contains NaN/Inf.
  bool fail_if_non_finite_sensitivity{false};
  // Fail when a parameter contains NaN/Inf before applying an update.
  bool fail_if_non_finite_parameter{false};
  // Fail when closure-produced loss contains NaN/Inf.
  bool fail_if_non_finite_loss{false};
};

struct StepLaneTelemetry {
  std::size_t group_index{0};
  UpdateLane requested{UpdateLane::Auto};
  UpdateLane resolved{UpdateLane::Loop};
  Platform platform{pluss::pCPU};
  ScalarType dtype{ScalarType::Float32};
  std::size_t active_parameters{0};
  bool batch_shape_compatible{false};
  bool foreach_supported{false};
  bool batched_supported{false};
  bool fused_supported{false};
  bool capturable_supported{false};
};

class PLUSS_API Optimizer {
public:
  explicit Optimizer(pluss::msharp::RangeView::Of<Series*> parameters)
      : parameters_(parameters.begin(), parameters.end()),
        parameter_labels_(default_labels_(parameters.size())) {
    validate_parameters_();
  }

  explicit Optimizer(Module& module)
      : parameters_(module.parameters()),
        parameter_labels_(extract_module_labels_(module)) {
    if (parameter_labels_.size() != parameters_.size()) {
      parameter_labels_ = default_labels_(parameters_.size());
    }
    validate_parameters_();
  }

  virtual ~Optimizer() = default;

  std::size_t size() const noexcept { return parameters_.size(); }

  const std::vector<Series*>& parameters() const noexcept { return parameters_; }

  const std::vector<StepLaneTelemetry>& last_step_lane_telemetry() const noexcept {
    return last_step_lane_telemetry_;
  }

  void clear() const {
    pluss::codem::series_tape::prepare_tracked_leaves(
        std::span<Series* const>(parameters_.data(), parameters_.size()));
  }

  void step() {
    std::vector<Series> grads;
    grads.reserve(parameters_.size());
    std::size_t tracked_count = 0;
    std::size_t covered_count = 0;
    for (Series* p : parameters_) {
      if (!p || !p->defined()) {
        grads.emplace_back();
        continue;
      }
      const bool tracked = p->tracks_slope();
      if (tracked) {
        ++tracked_count;
      }
      if (!p->has_sensitivity()) {
        grads.emplace_back();
      } else {
        if (tracked) {
          ++covered_count;
        }
        Series sens = p->sensitivity();
        if (sens.defined() && !sens.is_contiguous()) {
          sens = sens.contiguous();
        }
        grads.push_back(std::move(sens));
      }
    }
    validate_step_diagnostics_(tracked_count, covered_count);
    step(grads);
  }

  void step(pluss::msharp::RangeView::Series gradients) {
    if (gradients.size() != parameters_.size()) {
      ::pluss::throw_error(
          "pluss::ai::nn::optim::Optimizer::step: gradients size must match parameter count");
    }
    const bool force_recording_step = []() {
      const char* raw = std::getenv("PLUSS_OPTIM_STEP_KEEP_RECORDING");
      if (!raw) return false;
      return std::strcmp(raw, "1") == 0 || std::strcmp(raw, "true") == 0 || std::strcmp(raw, "TRUE") == 0;
    }();
    if (requires_recording_during_step_() || force_recording_step) {
      validate_parameter_finiteness_();
      validate_gradients_(gradients);
      apply_step_(gradients);
      return;
    }

    const pluss::codem::series_tape::PauseRecording no_record;
    validate_parameter_finiteness_();
    validate_gradients_(gradients);
    apply_step_(gradients);
  }

  template <class Closure>
  requires(std::invocable<Closure&> &&
           std::same_as<std::remove_cvref_t<std::invoke_result_t<Closure&>>, Series>)
  Series step(Closure&& closure) {
    clear();
    Series loss = std::invoke(std::forward<Closure>(closure));
    if (!loss.defined()) {
      ::pluss::throw_error("pluss::ai::nn::optim::Optimizer::step(closure): closure returned undefined loss");
    }
    ::pluss::detail::nn::require_training_float_dtype_(
        loss.scalar_type(),
        "nn::optim::Optimizer::step(closure)",
        "loss");
    if (step_diagnostics_.fail_if_non_finite_loss && has_non_finite_(loss)) {
      ::pluss::throw_error("pluss::ai::nn::optim::Optimizer::step(closure): non-finite loss detected");
    }
    loss.reverse();
    step();
    return loss;
  }

  template <class Closure>
  requires(std::invocable<Closure&> &&
           std::same_as<std::remove_cvref_t<std::invoke_result_t<Closure&>>,
                        std::pair<Series, std::vector<Series>>>)
  Series step(Closure&& closure) {
    auto out = std::invoke(std::forward<Closure>(closure));
    Series loss = std::move(out.first);
    std::vector<Series> gradients = std::move(out.second);
    if (!loss.defined()) {
      ::pluss::throw_error(
          "pluss::ai::nn::optim::Optimizer::step(closure): closure returned undefined loss");
    }
    ::pluss::detail::nn::require_training_float_dtype_(
        loss.scalar_type(),
        "nn::optim::Optimizer::step(closure)",
        "loss");
    if (step_diagnostics_.fail_if_non_finite_loss && has_non_finite_(loss)) {
      ::pluss::throw_error("pluss::ai::nn::optim::Optimizer::step(closure): non-finite loss detected");
    }
    step(gradients);
    return loss;
  }

protected:
  static Series clip_gradient_(const Series& g, const GradientClipOptions& clip) {
    if (!g.defined() || clip.mode == GradientClipMode::None) return g;
    if (!(clip.threshold > 0.0) || !std::isfinite(clip.threshold)) {
      ::pluss::throw_error(
          "pluss::ai::nn::optim::Optimizer: clip threshold must be positive and finite");
    }
    if (!(clip.epsilon > 0.0) || !std::isfinite(clip.epsilon)) {
      ::pluss::throw_error(
          "pluss::ai::nn::optim::Optimizer: clip epsilon must be positive and finite");
    }

    if (clip.mode == GradientClipMode::ByValue) {
      return g.clamp(-clip.threshold, clip.threshold);
    }

    const Series norm = g.square().sum().sqrt();
    const Series one = scalar_on_(1.0, g);
    const Series max_scale =
        scalar_on_(clip.threshold, g).div(norm.add(scalar_on_(clip.epsilon, g)));
    const Series scale = max_scale.minimum(one);
    return g.mul(scale);
  }

  static void assign_parameter_(Series& param, const Series& next_value) {
    if (!param.defined()) return;
    const bool tracks = param.tracks_slope();
    param = next_value;
    // Optimizer outputs become fresh leaf parameters for the next iteration.
    pluss::codem::series_tape::clear_source_link(param);
    pluss::codem::series_tape::set_tracks_slope(param, tracks);
    pluss::codem::series_tape::set_keep_sensitivity(param, false);
    pluss::codem::series_tape::clear_sensitivity(param);
  }

  static UpdateLane resolve_update_lane_(const ExecutionOptions& exec,
                                         bool batched_supported,
                                         bool fused_supported,
                                         const char* where,
                                         bool capturable_supported = false) {
    const auto fail_or_loop = [&](std::string_view reason) -> UpdateLane {
      if (exec.strict) {
        ::pluss::throw_error(std::string(where) + ": " + std::string(reason));
      }
      return UpdateLane::Loop;
    };

    if (exec.capturable && !capturable_supported && exec.strict) {
      ::pluss::throw_error(std::string(where) +
                               ": capturable execution requested but unsupported for this optimizer");
    }

    switch (exec.lane) {
      case UpdateLane::Loop:
        return UpdateLane::Loop;
      case UpdateLane::Foreach:
        return batched_supported ? UpdateLane::Foreach
                                 : fail_or_loop("foreach lane requested but unsupported for this group");
      case UpdateLane::Batched:
        return batched_supported ? UpdateLane::Batched
                                 : fail_or_loop("batched lane requested but unsupported for this group");
      case UpdateLane::Fused:
        return fused_supported ? UpdateLane::Fused
                               : fail_or_loop("fused lane requested but unsupported for this group");
      case UpdateLane::Auto:
      default:
        if (batched_supported) return UpdateLane::Batched;
        if (fused_supported) return UpdateLane::Fused;
        return UpdateLane::Loop;
    }
  }

  static const Series* active_lane_anchor_(
      pluss::msharp::RangeView::Of<std::size_t> indices,
      pluss::msharp::RangeView::Of<Series*> params,
      pluss::msharp::RangeView::Series gradients) {
    for (const auto i : indices) {
      if (i >= params.size() || i >= gradients.size()) continue;
      const Series* p = params[i];
      if (p == nullptr || !p->defined()) continue;
      const Series& g = gradients[i];
      if (!g.defined()) continue;
      return p;
    }
    return nullptr;
  }

  static bool supports_fused_lane_platform_(const Platform platform,
                                            const ScalarType dtype) noexcept {
    if (!::pluss::detail::nn::is_training_float_dtype_(dtype)) {
      return false;
    }
    if (platform == pluss::pCPU) {
      // CPU fused lane is implemented through batched tensor update graphs.
      return true;
    }
    if (platform == pluss::pMPS) {
      // MPS fused lane shares the same graph-level fused update path.
      return pluss::mps::is_ready();
    }
    if (platform == pluss::pCUDA) {
      return pluss::cuda::is_ready() &&
             pluss::cuda::native_baseline_enabled() &&
             pluss::cuda::native_elementwise_enabled() &&
             pluss::cuda::native_reduction_enabled() &&
             pluss::cuda::native_compose_enabled() &&
             pluss::cuda::supports_elementwise_baseline(dtype) &&
             pluss::cuda::supports_reduction_baseline(dtype) &&
             pluss::cuda::supports_compose_baseline(dtype);
    }
    return false;
  }

  static bool supports_batched_lane_platform_(const Platform platform,
                                              const ScalarType dtype) noexcept {
    if (!::pluss::detail::nn::is_training_float_dtype_(dtype)) {
      return false;
    }
    if (platform == pluss::pCPU) {
      return true;
    }
    if (platform == pluss::pMPS) {
      return pluss::mps::is_ready();
    }
    if (platform == pluss::pCUDA) {
      return pluss::cuda::is_ready() &&
             pluss::cuda::native_baseline_enabled() &&
             pluss::cuda::native_elementwise_enabled() &&
             pluss::cuda::native_reduction_enabled() &&
             pluss::cuda::supports_elementwise_baseline(dtype) &&
             pluss::cuda::supports_reduction_baseline(dtype);
    }
    return false;
  }

  static bool supports_batched_lane_(
      pluss::msharp::RangeView::Of<std::size_t> indices,
      pluss::msharp::RangeView::Of<Series*> params,
      pluss::msharp::RangeView::Series gradients) {
    const Series* anchor = active_lane_anchor_(indices, params, gradients);
    if (anchor == nullptr) return false;
    return supports_batched_lane_platform_(anchor->platform(), anchor->scalar_type());
  }

  static bool supports_fused_lane_(
      pluss::msharp::RangeView::Of<std::size_t> indices,
      pluss::msharp::RangeView::Of<Series*> params,
      pluss::msharp::RangeView::Series gradients) {
    const Series* anchor = active_lane_anchor_(indices, params, gradients);
    if (anchor == nullptr) return false;
    return supports_fused_lane_platform_(anchor->platform(), anchor->scalar_type());
  }

  static bool supports_capturable_lane_(
      pluss::msharp::RangeView::Of<std::size_t> indices,
      pluss::msharp::RangeView::Of<Series*> params,
      pluss::msharp::RangeView::Series gradients) {
    const Series* anchor = active_lane_anchor_(indices, params, gradients);
    if (anchor == nullptr) return true;
    const Platform platform = anchor->platform();
    if (platform == pluss::pCPU) return true;
    if (platform == pluss::pMPS) return pluss::mps::is_ready();
    if (platform == pluss::pCUDA) return pluss::cuda::is_ready();
    return false;
  }

  static std::size_t active_lane_parameter_count_(
      pluss::msharp::RangeView::Of<std::size_t> indices,
      pluss::msharp::RangeView::Of<Series*> params,
      pluss::msharp::RangeView::Series gradients) {
    std::size_t count = 0;
    for (const auto i : indices) {
      if (i >= params.size() || i >= gradients.size()) continue;
      const Series* p = params[i];
      if (p == nullptr || !p->defined()) continue;
      if (!gradients[i].defined()) continue;
      ++count;
    }
    return count;
  }

  static std::vector<std::int64_t> with_batch_axis_(const Series& x) {
    std::vector<std::int64_t> shape;
    shape.reserve(x.sizes().size() + 1);
    shape.push_back(1);
    for (const auto d : x.sizes()) shape.push_back(d);
    return shape;
  }

  static Series pack_batched_(pluss::msharp::RangeView::Series slices, const char* where) {
    if (slices.empty()) {
      ::pluss::throw_error(std::string(where) + ": expected non-empty batched slices");
    }
    if (slices.size() == 1) return slices.front();
    return slices.front().unite(std::vector<Series>(slices.begin() + 1, slices.end()), 0);
  }

  static Series scalar_on_(double value, const Series& like) {
    return Series::full({}, value, like.scalar_type(), like.platform());
  }

  static Series state_zeros_like_(const Series& like) {
    return pluss::series::zeros_like(like, like.scalar_type());
  }

  void reset_step_lane_telemetry_() {
    last_step_lane_telemetry_.clear();
  }

  void append_step_lane_telemetry_(
      std::size_t group_index,
      UpdateLane requested,
      UpdateLane resolved,
      pluss::msharp::RangeView::Of<std::size_t> indices,
      pluss::msharp::RangeView::Of<Series*> params,
      pluss::msharp::RangeView::Series gradients,
      bool batch_shape_compatible,
      bool batched_supported,
      bool fused_supported,
      bool capturable_supported) {
    const Series* anchor = active_lane_anchor_(indices, params, gradients);
    StepLaneTelemetry item{};
    item.group_index = group_index;
    item.requested = requested;
    item.resolved = resolved;
    item.platform = anchor ? anchor->platform() : pluss::pCPU;
    item.dtype = anchor ? anchor->scalar_type() : ScalarType::Float32;
    item.active_parameters = active_lane_parameter_count_(indices, params, gradients);
    item.batch_shape_compatible = batch_shape_compatible;
    item.foreach_supported = batched_supported;
    item.batched_supported = batched_supported;
    item.fused_supported = fused_supported;
    item.capturable_supported = capturable_supported;
    last_step_lane_telemetry_.push_back(std::move(item));
  }

  void validate_state_vector_(pluss::msharp::RangeView::Series state, const char* where) const {
    if (state.size() != parameters_.size()) {
      ::pluss::throw_error(std::string(where) + ": state size must match parameter count");
    }
    for (std::size_t i = 0; i < state.size(); ++i) {
      const Series& s = state[i];
      const Series* p = parameters_[i];
      if (!s.defined()) {
        ::pluss::throw_error(std::string(where) + ": state tensors must be defined");
      }
      if (s.platform() != p->platform()) {
        ::pluss::throw_error(std::string(where) + ": state/platform mismatch");
      }
      if (!same_shape_(s, *p)) {
        ::pluss::throw_error(std::string(where) + ": state/shape mismatch");
      }
    }
  }

  const std::vector<Series*>& mutable_parameters_() noexcept { return parameters_; }

public:
  void set_step_diagnostics(StepDiagnosticsOptions options) {
    if (options.minimum_tracked_coverage < 0.0 || options.minimum_tracked_coverage > 1.0 ||
        !std::isfinite(options.minimum_tracked_coverage)) {
      ::pluss::throw_error(
          "pluss::ai::nn::optim::Optimizer: minimum_tracked_coverage must be finite in [0, 1]");
    }
    step_diagnostics_ = options;
  }

  const StepDiagnosticsOptions& step_diagnostics() const noexcept {
    return step_diagnostics_;
  }

private:
  static bool same_shape_(const Series& a, const Series& b) {
    const auto as = a.sizes();
    const auto bs = b.sizes();
    if (as.size() != bs.size()) return false;
    for (std::size_t i = 0; i < as.size(); ++i) {
      if (as[i] != bs[i]) return false;
    }
    return true;
  }

  static std::vector<std::string> default_labels_(std::size_t n) {
    std::vector<std::string> out;
    out.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
      out.push_back("param[" + std::to_string(i) + "]");
    }
    return out;
  }

  static std::vector<std::string> extract_module_labels_(Module& module) {
    std::vector<std::string> out;
    for (const auto& named : module.named_parameters()) {
      out.push_back(named.name);
    }
    return out;
  }

  static bool has_non_finite_(const Series& x) {
    const Series finite_all = x.isfinite().all();
    if (finite_all.platform() == pluss::pCUDA && !pluss::cuda::cpu_bridge_enabled()) {
      ::pluss::throw_error(
          "pluss::ai::nn::optim::Optimizer: non-finite CUDA diagnostics require explicit CPU scalar "
          "materialization; enable PLUSS_CUDA_ALLOW_CPU_BRIDGE=1 or disable the diagnostic option");
    }
    return !finite_all.to(pluss::pCPU).item<bool>();
  }

  void validate_parameters_() {
    if (parameters_.empty()) {
      ::pluss::throw_error("pluss::ai::nn::optim::Optimizer: expected non-empty parameter list");
    }
    for (std::size_t i = 0; i < parameters_.size(); ++i) {
      Series* p = parameters_[i];
      const std::string& label = parameter_labels_[i];
      if (p == nullptr || !p->defined()) {
        ::pluss::throw_error(
            "pluss::ai::nn::optim::Optimizer: expected defined parameter Series for " + label);
      }
      if (!pluss::codem::dtype_of(p->scalar_type()).is_floating()) {
        ::pluss::throw_error(
            "pluss::ai::nn::optim::Optimizer: expected floating dtype parameter for " + label);
      }
      if (!::pluss::detail::nn::is_training_float_dtype_(p->scalar_type())) {
        ::pluss::throw_error(
            "pluss::ai::nn::optim::Optimizer: training expects Float32/Float16/BFloat16 parameters; got unsupported dtype for " +
            label);
      }
      if (!p->tracks_slope()) {
        p->tracks_slope_(true);
      }
    }
  }

  void validate_gradients_(pluss::msharp::RangeView::Series gradients) const {
    for (std::size_t i = 0; i < gradients.size(); ++i) {
      const Series& g = gradients[i];
      const Series* p = parameters_[i];
      const std::string& label = parameter_labels_[i];
      if (!g.defined()) continue;
      if (g.platform() != p->platform()) {
        ::pluss::throw_error(
            "pluss::ai::nn::optim::Optimizer::step: gradient/platform mismatch for " + label);
      }
      if (g.scalar_type() != p->scalar_type()) {
        ::pluss::throw_error(
            "pluss::ai::nn::optim::Optimizer::step: gradient/dtype mismatch for " + label);
      }
      if (!same_shape_(g, *p) && !(g.numel() == 1 && p->numel() == 1)) {
        ::pluss::throw_error(
            "pluss::ai::nn::optim::Optimizer::step: gradient/shape mismatch for " + label);
      }
      if (step_diagnostics_.fail_if_non_finite_sensitivity && has_non_finite_(g)) {
        ::pluss::throw_error(
            "pluss::ai::nn::optim::Optimizer::step: non-finite sensitivity detected for " + label);
      }
    }
  }

  void validate_parameter_finiteness_() const {
    if (!step_diagnostics_.fail_if_non_finite_parameter) return;
    for (std::size_t i = 0; i < parameters_.size(); ++i) {
      const Series* p = parameters_[i];
      if (!p || !p->defined()) continue;
      if (has_non_finite_(*p)) {
        ::pluss::throw_error(
            "pluss::ai::nn::optim::Optimizer::step: non-finite parameter detected for " +
            parameter_labels_[i]);
      }
    }
  }

  void validate_step_diagnostics_(std::size_t tracked_count, std::size_t covered_count) const {
    if (tracked_count == 0) return;
    if (step_diagnostics_.fail_if_no_sensitivity && covered_count == 0) {
      ::pluss::throw_error(
          "pluss::ai::nn::optim::Optimizer::step: no sensitivities were produced for tracked parameters");
    }
    if (step_diagnostics_.minimum_tracked_coverage > 0.0) {
      const double coverage = static_cast<double>(covered_count) / static_cast<double>(tracked_count);
      if (coverage + 1e-15 < step_diagnostics_.minimum_tracked_coverage) {
        ::pluss::throw_error(
            "pluss::ai::nn::optim::Optimizer::step: tracked-parameter sensitivity coverage below minimum");
      }
    }
  }

  virtual void apply_step_(pluss::msharp::RangeView::Series gradients) = 0;
  virtual bool requires_recording_during_step_() const { return false; }

  std::vector<Series*> parameters_{};
  std::vector<std::string> parameter_labels_{};
  StepDiagnosticsOptions step_diagnostics_{};
  std::vector<StepLaneTelemetry> last_step_lane_telemetry_{};
};

} // namespace pluss::ai::nn::optim
