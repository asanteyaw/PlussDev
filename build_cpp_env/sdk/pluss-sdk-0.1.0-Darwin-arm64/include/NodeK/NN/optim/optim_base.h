#pragma once

#include "NodeK/NN/modules/Module.h"
#include "CodeM/core/SeriesTape.h"
#include "MSharp/RangeView.h"
#include "pluss/codem/codem.h"
#include "pluss/nn_detail.h"

#include <algorithm>
#include <complex>
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
        grads.push_back(pluss::series::undefined());
        continue;
      }
      const bool tracked = p->tracks_slope();
      if (tracked) {
        ++tracked_count;
      }
      if (!p->has_sensitivity()) {
        grads.push_back(pluss::series::undefined());
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
      // Elementwise clamp needs a total order over the gradient's own
      // values, which complex numbers don't have -- Series::clamp already
      // rejects complex with a clear message (matches libtorch's own
      // clip_grad_value_, verified live to fail identically: "clamp is not
      // supported for complex types"), so this is a correct, deliberate
      // limitation to propagate, not something to work around here.
      return g.clamp(-clip.threshold, clip.threshold);
    }

    // ByNorm never needs to ORDER the (possibly complex) gradient values
    // themselves -- it reduces to a real, non-negative scalar (the norm)
    // and scales `g` by a real factor, which is why libtorch's own
    // clip_grad_norm_ supports complex gradients (verified live) even
    // though clip_grad_value_ does not. abs2() (|g|^2, real-valued even for
    // complex g) in place of square() (g^2, itself complex and NOT the
    // magnitude) is the only change needed; for a real g, abs(g)^2 == g^2
    // exactly (abs() is a sign-bit no-op on IEEE floats), so this is a
    // behavior-preserving generalization, not a special case.
    const Series norm = g.abs2().sum().sqrt();
    const Series one = scalar_on_(1.0, norm);
    const Series max_scale =
        scalar_on_(clip.threshold, norm).div(norm.add(scalar_on_(clip.epsilon, norm)));
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

  static bool is_complex_(const Series& x) {
    return x.defined() && pluss::codem::dtype_of(x.scalar_type()).is_complex();
  }

  // Pluss's counterpart to torch's `torch.view_as_real`: reinterpret a
  // complex Series as a real one with a NEW trailing dimension of size 2
  // (index 0 = real part, index 1 = imaginary part), so every concrete
  // optimizer's existing (already Float64-verified, dtype-generic) formula
  // runs completely unchanged and applies independently to the real and
  // imaginary channels -- exactly torch's own approach for complex
  // parameters (verified by reading tools/optim/adam.py etc. in the local
  // PyTorch checkout: `_view_as_real` is called on param/grad/exp_avg/
  // exp_avg_sq/max_exp_avg_sqs before running the ordinary real update,
  // because a complex gradient's second-moment estimate needs
  // real(g)^2 and imag(g)^2 tracked SEPARATELY -- not |g|^2, which mixes
  // them, and not g^2, which is itself complex).
  //
  // Real (non-complex) input passes through unchanged, so call sites don't
  // need to branch.
  //
  // UNLIKE torch's `view_as_real` (a true zero-copy view -- std::complex<T>
  // and T[2] share layout, so aliasing is possible in principle), this is a
  // COPY: CodeM has no reinterpret-cast-dtype view primitive today. Adding
  // one would be a real, separate CodeM-level engineering project (storage/
  // stride semantics, autodiff through a reinterpreted dtype boundary, and
  // per-backend layout for MPS/CUDA) -- deliberately out of scope here.
  // This still reaches the numerically identical values torch's view
  // approach does, at the cost of `real()`/`imag()` extraction and a
  // reconstruction on the way back (see view_as_complex_below) instead of a
  // free reinterpretation. `assign_parameter_`'s own final write is
  // similarly not the perf-critical path (once per step per parameter);
  // the actual moment-buffer arithmetic still runs entirely in the tracked/
  // fast Series primitives either way, just doubled in count (one real op
  // per real op that used to run once on the complex value).
  static Series view_as_real_(const Series& x) {
    if (!is_complex_(x)) return x;
    const Series re = x.real();
    const Series im = x.imag();
    return re.pile(std::vector<Series>{im}, re.dim());
  }

  // Inverse of view_as_real_: given a real Series shaped [..., 2] (as
  // produced by view_as_real_) and the ORIGINAL complex dtype, reconstruct
  // the complex Series. `original_dtype` not being complex means the value
  // was never converted in the first place (view_as_real_ passed it
  // through) -- returns it unchanged, matching that symmetry.
  static Series view_as_complex_(const Series& real_view, ScalarType original_dtype) {
    // An undefined state buffer (e.g. RMSprop's momentum_buffer_/grad_avg_
    // when momentum==0/centered==false, never allocated) stays undefined
    // regardless of the parameter's own dtype -- view_as_real_ already
    // passes an undefined Series through unchanged, so this must too rather
    // than dereference it below.
    if (!real_view.defined()) return real_view;
    if (!pluss::codem::dtype_of(original_dtype).is_complex()) return real_view;
    const std::int64_t last = real_view.dim() - 1;
    const Series re = real_view.select(last, 0);
    const Series im = real_view.select(last, 1);
    // No public "construct complex from real+imag parts" entry point
    // (CodeM's own complex_from_parts is internal-only), so build it from
    // ordinary arithmetic instead: casting a real Series to a complex dtype
    // embeds it with zero imaginary part, so `im_as_complex * i` isolates
    // `im` into the imaginary slot, and adding the (zero-imaginary) real
    // part recombines them exactly.
    const Series i_unit =
        pluss::make_series(std::vector<std::complex<double>>{{0.0, 1.0}}, {})
            .to(original_dtype)
            .to(re.platform());
    return re.to(original_dtype).add(im.to(original_dtype).mul(i_unit));
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
    // Same reasoning as Minimax::finite_series_: `isfinite()` has no
    // complex-dtype implementation, so check the real/imaginary channels
    // independently via view_as_real_ (a no-op for a real-valued x).
    const Series finite_all = view_as_real_(x).isfinite().all();
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
      // Parameters may be floating OR complex (matching real torch: its
      // Adam/AdamW/RMSprop/NAdam/Adagrad/SGD all accept complex
      // Parameters, verified live). The LOSS a closure returns is a
      // separate, narrower check (require_training_float_dtype_, real-only
      // -- see Optimizer::step(closure)): a loss has to be real for
      // "minimize" to have a direction, even when the parameters producing
      // it are complex.
      const auto dtype = pluss::codem::dtype_of(p->scalar_type());
      if (!dtype.is_floating() && !dtype.is_complex()) {
        ::pluss::throw_error(
            "pluss::ai::nn::optim::Optimizer: expected floating or complex dtype parameter for " + label);
      }
      if (!::pluss::detail::nn::is_training_numeric_dtype_(p->scalar_type())) {
        ::pluss::throw_error(
            "pluss::ai::nn::optim::Optimizer: training expects Float32/Float16/BFloat16/Float64/Complex64/Complex128 "
            "parameters; got unsupported dtype for " +
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
