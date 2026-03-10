#pragma once

#include "NodeK/NN/modules/Module.h"
#include "CodeM/core/SeriesTape.h"

#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <string>
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

class PLUSS_API Optimizer {
public:
  explicit Optimizer(std::vector<Series*> parameters)
      : parameters_(std::move(parameters)) {
    validate_parameters_();
  }

  explicit Optimizer(Module& module)
      : Optimizer(module.parameters()) {}

  virtual ~Optimizer() = default;

  std::size_t size() const noexcept { return parameters_.size(); }

  const std::vector<Series*>& parameters() const noexcept { return parameters_; }

  void clear_sensitivity() const {
    for (Series* p : parameters_) {
      if (p && p->defined()) {
        p->clear_sensitivity();
      }
    }
  }

  void step() {
    std::vector<Series> grads;
    grads.reserve(parameters_.size());
    for (Series* p : parameters_) {
      if (!p || !p->defined() || !p->has_sensitivity()) {
        grads.emplace_back();
      } else {
        grads.push_back(p->sensitivity());
      }
    }
    step(grads);
  }

  void step(const std::vector<Series>& gradients) {
    if (gradients.size() != parameters_.size()) {
      throw std::runtime_error(
          "pluss::ai::nn::optim::Optimizer::step: gradients size must match parameter count");
    }
    validate_gradients_(gradients);
    apply_step_(gradients);
  }

protected:
  static Series clip_gradient_(const Series& g, const GradientClipOptions& clip) {
    if (!g.defined() || clip.mode == GradientClipMode::None) return g;
    if (!(clip.threshold > 0.0) || !std::isfinite(clip.threshold)) {
      throw std::runtime_error(
          "pluss::ai::nn::optim::Optimizer: clip threshold must be positive and finite");
    }
    if (!(clip.epsilon > 0.0) || !std::isfinite(clip.epsilon)) {
      throw std::runtime_error(
          "pluss::ai::nn::optim::Optimizer: clip epsilon must be positive and finite");
    }

    if (clip.mode == GradientClipMode::ByValue) {
      return g.clamp(-clip.threshold, clip.threshold);
    }

    const double norm = g.square().sum().sqrt().item<double>();
    if (!std::isfinite(norm) || norm <= clip.threshold) return g;
    const double scale = clip.threshold / (norm + clip.epsilon);
    return g.mul(scale);
  }

  static void assign_parameter_(Series& param, const Series& next_value) {
    if (!param.defined()) return;
    const bool tracks = param.tracks_slope();
    param = next_value.detach();
    pluss::codem::series_tape::set_tracks_slope(param, tracks);
    pluss::codem::series_tape::clear_sensitivity(param);
  }

  const std::vector<Series*>& mutable_parameters_() noexcept { return parameters_; }

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

  void validate_parameters_() const {
    if (parameters_.empty()) {
      throw std::runtime_error("pluss::ai::nn::optim::Optimizer: expected non-empty parameter list");
    }
    for (Series* p : parameters_) {
      if (p == nullptr || !p->defined()) {
        throw std::runtime_error("pluss::ai::nn::optim::Optimizer: expected defined parameter Series");
      }
      if (!pluss::codem::dtype_of(p->scalar_type()).is_floating()) {
        throw std::runtime_error(
            "pluss::ai::nn::optim::Optimizer: expected floating dtype parameters");
      }
    }
  }

  void validate_gradients_(const std::vector<Series>& gradients) const {
    for (std::size_t i = 0; i < gradients.size(); ++i) {
      const Series& g = gradients[i];
      const Series* p = parameters_[i];
      if (!g.defined()) continue;
      if (g.platform() != p->platform()) {
        throw std::runtime_error(
            "pluss::ai::nn::optim::Optimizer::step: gradient/platform mismatch");
      }
      if (!same_shape_(g, *p) && !(g.numel() == 1 && p->numel() == 1)) {
        throw std::runtime_error(
            "pluss::ai::nn::optim::Optimizer::step: gradient/shape mismatch");
      }
    }
  }

  virtual void apply_step_(const std::vector<Series>& gradients) = 0;

  std::vector<Series*> parameters_{};
};

} // namespace pluss::ai::nn::optim
