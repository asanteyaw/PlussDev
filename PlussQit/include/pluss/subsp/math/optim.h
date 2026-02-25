#pragma once

#include "pluss/codem/series_factory.h"
#include "CodeM/compat/Invoke.h"

#include <cmath>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace pluss::subsp::math::optim {

namespace detail {

inline bool same_shape_(const Series& a, const Series& b) {
  if (!a.defined() || !b.defined()) return false;
  if (a.sizes().size() != b.sizes().size()) return false;
  for (std::size_t i = 0; i < a.sizes().size(); ++i) {
    if (a.sizes()[i] != b.sizes()[i]) return false;
  }
  return true;
}

inline bool query_bool_(std::string_view op, const Series& s) {
  return pluss::codem::_compat::invoke(op, s).item<bool>();
}

inline Series query_series_(std::string_view op, const Series& s) {
  return pluss::codem::_compat::invoke(op, s);
}

inline void invoke_inplace_(std::string_view op, Series& s) {
  pluss::codem::_compat::invoke_(op, s);
}

inline void require_defined_(const Series& x, const char* op) {
  if (!x.defined()) {
    throw std::runtime_error(std::string("subsp::math::optim::") + op + ": expected defined Series");
  }
}

inline void require_same_platform_(const Series& a, const Series& b, const char* op) {
  if (a.platform() != b.platform()) {
    throw std::runtime_error(std::string("subsp::math::optim::") + op + ": expected matching platforms");
  }
}

inline void require_float_dtype_(const Series& x, const char* op) {
  if (!x.dtype().is_floating() && !x.dtype().is_complex()) {
    throw std::runtime_error(std::string("subsp::math::optim::") + op +
                             ": expected floating or complex dtype");
  }
}

} // namespace detail

class Trace {
public:
  Trace() = default;
  explicit Trace(const Series& value) : value_(value) {}
  Trace(const Series& value, bool tracks_slope) : value_(value) {
    value_->requires_grad_(tracks_slope);
  }

  bool defined() const noexcept { return value_.has_value() && value_->defined(); }

  bool tracks_slope() const {
    if (!defined()) return false;
    return detail::query_bool_("tracks_slope", *value_);
  }

  bool is_leaf() const {
    if (!defined()) return true;
    return detail::query_bool_("is_leaf", *value_);
  }

  const Series& value() const {
    if (!defined()) {
      throw std::runtime_error("subsp::math::optim::Trace::value: undefined trace");
    }
    return *value_;
  }

  bool has_sensitivity() const {
    if (!defined()) return false;
    return detail::query_bool_("has_sensitivity", *value_);
  }

  Series sensitivity() const {
    if (!defined()) {
      throw std::runtime_error("subsp::math::optim::Trace::sensitivity: undefined trace");
    }
    return detail::query_series_("sensitivity", *value_);
  }

  void keep_sensitivity(bool keep = true) {
    if (defined()) value_->retain_grad(keep);
  }

  void clear_sensitivity() {
    if (defined()) detail::invoke_inplace_("clear_sensitivity", *value_);
  }

  Trace separate() const {
    if (!defined()) return Trace();
    return Trace(value_->detach());
  }

  void propagate() const {
    if (!defined()) {
      throw std::runtime_error("subsp::math::optim::Trace::propagate: undefined trace");
    }
    value_->_backward();
  }

  void propagate(const Series& sensitivity_output) const {
    if (!defined()) {
      throw std::runtime_error("subsp::math::optim::Trace::propagate: undefined trace");
    }
    value_->_backward(sensitivity_output);
  }

private:
  std::optional<Series> value_{};
};

inline Trace trace(const Series& value, bool tracks_slope = true) {
  return Trace(value, tracks_slope);
}

inline Trace fixed(const Series& value) {
  return Trace(value, false);
}

inline Trace make_scalar_like_(const Trace& like, double value) {
  return fixed(pluss::full_like(like.value(), value, like.value().scalar_type()));
}

inline Trace add(const Trace& a, const Trace& b) {
  detail::require_defined_(a.value(), "add");
  detail::require_defined_(b.value(), "add");
  detail::require_same_platform_(a.value(), b.value(), "add");
  return Trace(a.value().add(b.value()));
}

inline Trace sub(const Trace& a, const Trace& b) {
  detail::require_defined_(a.value(), "sub");
  detail::require_defined_(b.value(), "sub");
  detail::require_same_platform_(a.value(), b.value(), "sub");
  return Trace(a.value().sub(b.value()));
}

inline Trace mul(const Trace& a, const Trace& b) {
  detail::require_defined_(a.value(), "mul");
  detail::require_defined_(b.value(), "mul");
  detail::require_same_platform_(a.value(), b.value(), "mul");
  return Trace(a.value().mul(b.value()));
}

inline Trace div(const Trace& a, const Trace& b) {
  detail::require_defined_(a.value(), "div");
  detail::require_defined_(b.value(), "div");
  detail::require_same_platform_(a.value(), b.value(), "div");
  return Trace(a.value().div(b.value()));
}

inline Trace neg(const Trace& x) {
  detail::require_defined_(x.value(), "neg");
  return Trace(x.value().neg());
}

inline Trace pow(const Trace& a, const Trace& b) {
  detail::require_defined_(a.value(), "pow");
  detail::require_defined_(b.value(), "pow");
  detail::require_same_platform_(a.value(), b.value(), "pow");
  return Trace(a.value().pow(b.value()));
}

inline Trace pow(const Trace& a, double s) {
  detail::require_defined_(a.value(), "pow");
  return Trace(a.value().pow(s));
}

inline Trace exp(const Trace& x) {
  detail::require_defined_(x.value(), "exp");
  return Trace(x.value().exp());
}

inline Trace log(const Trace& x) {
  detail::require_defined_(x.value(), "log");
  return Trace(x.value().log());
}

inline Trace sqrt(const Trace& x) {
  detail::require_defined_(x.value(), "sqrt");
  return Trace(x.value().sqrt());
}

inline Trace sin(const Trace& x) {
  detail::require_defined_(x.value(), "sin");
  return Trace(x.value().sin());
}

inline Trace cos(const Trace& x) {
  detail::require_defined_(x.value(), "cos");
  return Trace(x.value().cos());
}

inline Trace tanh(const Trace& x) {
  detail::require_defined_(x.value(), "tanh");
  return Trace(x.value().tanh());
}

inline Trace square(const Trace& x) {
  detail::require_defined_(x.value(), "square");
  return Trace(x.value().square());
}

inline Trace rsqrt(const Trace& x) {
  detail::require_defined_(x.value(), "rsqrt");
  return Trace(x.value().rsqrt());
}

inline Trace relu(const Trace& x) {
  detail::require_defined_(x.value(), "relu");
  return Trace(x.value().relu());
}

inline Trace sigmoid(const Trace& x) {
  detail::require_defined_(x.value(), "sigmoid");
  return Trace(x.value().sigmoid());
}

inline Trace softplus(const Trace& x, double beta = 1.0, double threshold = 20.0) {
  detail::require_defined_(x.value(), "softplus");
  return Trace(x.value().softplus(beta, threshold));
}

inline Trace clamp(const Trace& x, double min, double max) {
  detail::require_defined_(x.value(), "clamp");
  return Trace(x.value().clamp(min, max));
}

inline Trace sum(const Trace& x) {
  detail::require_defined_(x.value(), "sum");
  return Trace(x.value().sum());
}

inline Trace mean(const Trace& x) {
  detail::require_defined_(x.value(), "mean");
  return Trace(x.value().mean());
}

inline Trace matmul(const Trace& a, const Trace& b) {
  detail::require_defined_(a.value(), "matmul");
  detail::require_defined_(b.value(), "matmul");
  detail::require_same_platform_(a.value(), b.value(), "matmul");
  if (a.value().dim() != 2 || b.value().dim() != 2) {
    throw std::runtime_error("subsp::math::optim::matmul: expected 2D inputs");
  }
  return Trace(a.value().matmul(b.value()));
}

inline Trace operator+(const Trace& a, const Trace& b) { return add(a, b); }
inline Trace operator-(const Trace& a, const Trace& b) { return sub(a, b); }
inline Trace operator*(const Trace& a, const Trace& b) { return mul(a, b); }
inline Trace operator/(const Trace& a, const Trace& b) { return div(a, b); }

inline Trace operator+(const Trace& a, double s) { return add(a, make_scalar_like_(a, s)); }
inline Trace operator-(const Trace& a, double s) { return sub(a, make_scalar_like_(a, s)); }
inline Trace operator*(const Trace& a, double s) { return mul(a, make_scalar_like_(a, s)); }
inline Trace operator/(const Trace& a, double s) { return div(a, make_scalar_like_(a, s)); }

inline Trace operator+(double s, const Trace& a) { return add(make_scalar_like_(a, s), a); }
inline Trace operator-(double s, const Trace& a) { return sub(make_scalar_like_(a, s), a); }
inline Trace operator*(double s, const Trace& a) { return mul(make_scalar_like_(a, s), a); }
inline Trace operator/(double s, const Trace& a) { return div(make_scalar_like_(a, s), a); }

inline Trace operator-(const Trace& x) { return neg(x); }

inline void clear_sensitivities(std::vector<Trace>& params) {
  for (auto& p : params) {
    p.clear_sensitivity();
  }
}

template <class Fn>
std::pair<Series, std::vector<Series>> value_and_sensitivity(Fn&& fn, const std::vector<Series>& params) {
  std::vector<Trace> vars;
  vars.reserve(params.size());
  for (const auto& p : params) {
    vars.emplace_back(p, true);
  }

  Trace loss = std::forward<Fn>(fn)(vars);
  if (!loss.defined()) {
    throw std::runtime_error("subsp::math::optim::value_and_sensitivity: objective returned undefined Trace");
  }

  loss.propagate();

  std::vector<Series> sensitivities;
  sensitivities.reserve(vars.size());
  for (const auto& v : vars) {
    sensitivities.push_back(v.sensitivity());
  }
  return {loss.value(), sensitivities};
}

struct AdamOptions {
  double learning_rate{1e-3};
  double beta1{0.9};
  double beta2{0.999};
  double eps{1e-8};
  double weight_decay{0.0};
};

struct AdamState {
  std::vector<Series> m{};
  std::vector<Series> v{};
  std::int64_t step{0};
};

inline AdamState adam_state_like(const std::vector<Series>& params) {
  AdamState st;
  st.m.reserve(params.size());
  st.v.reserve(params.size());
  for (const auto& p : params) {
    st.m.push_back(pluss::zeros_like(p, p.scalar_type()));
    st.v.push_back(pluss::zeros_like(p, p.scalar_type()));
  }
  return st;
}

inline void adam_step_inplace(std::vector<Series>& params,
                              const std::vector<Series>& sensitivities,
                              AdamState& state,
                              const AdamOptions& opt = {}) {
  if (params.size() != sensitivities.size()) {
    throw std::runtime_error("subsp::math::optim::adam_step_inplace: params/sensitivities size mismatch");
  }
  if (state.m.size() != params.size() || state.v.size() != params.size()) {
    state = adam_state_like(params);
  }

  ++state.step;
  const double b1_correction = 1.0 - std::pow(opt.beta1, static_cast<double>(state.step));
  const double b2_correction = 1.0 - std::pow(opt.beta2, static_cast<double>(state.step));

  for (std::size_t i = 0; i < params.size(); ++i) {
    detail::require_defined_(params[i], "adam_step_inplace");
    detail::require_defined_(sensitivities[i], "adam_step_inplace");
    detail::require_same_platform_(params[i], sensitivities[i], "adam_step_inplace");
    detail::require_same_platform_(params[i], state.m[i], "adam_step_inplace");
    detail::require_same_platform_(params[i], state.v[i], "adam_step_inplace");
    detail::require_float_dtype_(params[i], "adam_step_inplace");

    // Keep optimizer updates outside autodiff recording.
    Series p = params[i].detach();
    Series g = sensitivities[i];
    if (!detail::same_shape_(p, g)) {
      if (p.numel() == 1 && g.numel() == 1) {
        g = g.sum();
      } else {
        throw std::runtime_error("subsp::math::optim::adam_step_inplace: sensitivity shape mismatch");
      }
    }

    if (opt.weight_decay != 0.0) {
      g = g.add(p.mul(opt.weight_decay));
    }

    state.m[i] = state.m[i].mul(opt.beta1).add(g.mul(1.0 - opt.beta1));
    state.v[i] = state.v[i].mul(opt.beta2).add(g.mul(g).mul(1.0 - opt.beta2));

    const Series mhat = state.m[i].div(b1_correction);
    const Series vhat = state.v[i].div(b2_correction);
    const Series update = mhat.div(vhat.sqrt().add(opt.eps)).mul(opt.learning_rate);
    params[i] = p.sub(update);
  }
}

inline std::vector<Series> adam_step(std::vector<Series> params,
                                     const std::vector<Series>& sensitivities,
                                     AdamState& state,
                                     const AdamOptions& opt = {}) {
  adam_step_inplace(params, sensitivities, state, opt);
  return params;
}

} // namespace pluss::subsp::math::optim
