#pragma once

#include "pluss/codem/series_factory.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace pluss::subsp::math::optim {

namespace detail {

struct Node {
  explicit Node(Series v, bool req, bool leaf = true)
      : value(std::move(v)), requires_grad(req), is_leaf(leaf) {}

  Series value;
  std::optional<Series> grad{};
  bool requires_grad{false};
  bool retain_grad{false};
  bool is_leaf{true};
  std::vector<std::shared_ptr<Node>> parents{};
  std::function<void(const Series&)> backward{};
};

inline bool same_shape_(const Series& a, const Series& b) {
  if (!a.defined() || !b.defined()) return false;
  return a.sizes().size() == b.sizes().size() &&
         std::equal(a.sizes().begin(), a.sizes().end(), b.sizes().begin());
}

inline Series reduce_grad_to_like_(const Series& grad, const Series& like, const char* op) {
  if (same_shape_(grad, like)) return grad;
  if (like.numel() == 1) return grad.sum();
  throw std::runtime_error(std::string("subsp::math::optim::") + op +
                           ": unsupported broadcast gradient reduction");
}

inline void accumulate_grad_(const std::shared_ptr<Node>& node, const Series& g) {
  if (!node || !node->requires_grad) return;
  if (!node->grad.has_value()) {
    node->grad = g;
  } else {
    node->grad = node->grad->add(g);
  }
}

inline void topo_sort_dfs_(const std::shared_ptr<Node>& node,
                           std::unordered_set<const Node*>& seen,
                           std::vector<std::shared_ptr<Node>>& out) {
  if (!node) return;
  if (seen.find(node.get()) != seen.end()) return;
  seen.insert(node.get());
  for (const auto& p : node->parents) {
    topo_sort_dfs_(p, seen, out);
  }
  out.push_back(node);
}

inline std::vector<std::shared_ptr<Node>> topo_sorted_(const std::shared_ptr<Node>& node) {
  std::vector<std::shared_ptr<Node>> out;
  std::unordered_set<const Node*> seen;
  topo_sort_dfs_(node, seen, out);
  return out;
}

inline void clear_grads_(const std::vector<std::shared_ptr<Node>>& nodes) {
  for (const auto& n : nodes) {
    n->grad.reset();
  }
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

inline std::shared_ptr<Node> make_leaf_(const Series& value, bool requires_grad) {
  auto n = std::make_shared<Node>(value, requires_grad, true);
  return n;
}

} // namespace detail

class Variable {
public:
  Variable() = default;
  explicit Variable(const Series& value, bool requires_grad = true)
      : node_(detail::make_leaf_(value, requires_grad)) {}

  bool defined() const noexcept { return static_cast<bool>(node_) && node_->value.defined(); }
  bool requires_grad() const noexcept { return static_cast<bool>(node_) && node_->requires_grad; }
  bool is_leaf() const noexcept { return !node_ || node_->is_leaf; }

  const Series& value() const {
    if (!defined()) {
      throw std::runtime_error("subsp::math::optim::Variable::value: undefined variable");
    }
    return node_->value;
  }

  bool has_grad() const noexcept {
    return static_cast<bool>(node_) && node_->grad.has_value();
  }

  Series grad() const {
    if (!node_) {
      throw std::runtime_error("subsp::math::optim::Variable::grad: undefined variable");
    }
    if (node_->grad.has_value()) {
      return *node_->grad;
    }
    return pluss::zeros_like(node_->value, node_->value.scalar_type());
  }

  void retain_grad(bool retain = true) {
    if (node_) node_->retain_grad = retain;
  }

  void zero_grad() {
    if (node_) node_->grad.reset();
  }

  Variable detach() const {
    return Variable(value(), false);
  }

  void backward() const {
    if (!node_) {
      throw std::runtime_error("subsp::math::optim::Variable::backward: undefined variable");
    }
    if (node_->value.numel() != 1) {
      throw std::runtime_error("subsp::math::optim::Variable::backward: grad_output required for non-scalar outputs");
    }
    backward(pluss::ones_like(node_->value, node_->value.scalar_type()));
  }

  void backward(const Series& grad_output) const {
    if (!node_) {
      throw std::runtime_error("subsp::math::optim::Variable::backward: undefined variable");
    }
    auto topo = detail::topo_sorted_(node_);
    detail::clear_grads_(topo);
    detail::accumulate_grad_(node_, grad_output);

    for (auto it = topo.rbegin(); it != topo.rend(); ++it) {
      const auto& cur = *it;
      if (!cur->grad.has_value() || !cur->backward) continue;
      cur->backward(*cur->grad);
      if (!cur->retain_grad && !cur->is_leaf && cur.get() != node_.get()) {
        cur->grad.reset();
      }
    }
  }

private:
  explicit Variable(std::shared_ptr<detail::Node> node) : node_(std::move(node)) {}

  std::shared_ptr<detail::Node> node_{};

  const std::shared_ptr<detail::Node>& node_ref_() const noexcept { return node_; }

  friend Variable add(const Variable&, const Variable&);
  friend Variable sub(const Variable&, const Variable&);
  friend Variable mul(const Variable&, const Variable&);
  friend Variable div(const Variable&, const Variable&);
  friend Variable neg(const Variable&);
  friend Variable sum(const Variable&);
  friend Variable mean(const Variable&);
  friend Variable matmul(const Variable&, const Variable&);
};

inline Variable variable(const Series& value, bool requires_grad = true) {
  return Variable(value, requires_grad);
}

inline Variable constant(const Series& value) {
  return Variable(value, false);
}

inline Variable make_scalar_like_(const Variable& like, double value) {
  return constant(pluss::full_like(like.value(), value, like.value().scalar_type()));
}

inline Variable add(const Variable& a, const Variable& b) {
  detail::require_defined_(a.value(), "add");
  detail::require_defined_(b.value(), "add");
  detail::require_same_platform_(a.value(), b.value(), "add");
  Series out = a.value().add(b.value());

  const bool req = a.requires_grad() || b.requires_grad();
  if (!req) return Variable(out, false);

  auto n = std::make_shared<detail::Node>(out, true, false);
  n->parents = {a.node_ref_(), b.node_ref_()};
  n->backward = [pa = a.node_ref_(), pb = b.node_ref_()](const Series& g) {
    if (pa && pa->requires_grad) {
      detail::accumulate_grad_(pa, detail::reduce_grad_to_like_(g, pa->value, "add"));
    }
    if (pb && pb->requires_grad) {
      detail::accumulate_grad_(pb, detail::reduce_grad_to_like_(g, pb->value, "add"));
    }
  };
  return Variable(std::move(n));
}

inline Variable sub(const Variable& a, const Variable& b) {
  detail::require_defined_(a.value(), "sub");
  detail::require_defined_(b.value(), "sub");
  detail::require_same_platform_(a.value(), b.value(), "sub");
  Series out = a.value().sub(b.value());

  const bool req = a.requires_grad() || b.requires_grad();
  if (!req) return Variable(out, false);

  auto n = std::make_shared<detail::Node>(out, true, false);
  n->parents = {a.node_ref_(), b.node_ref_()};
  n->backward = [pa = a.node_ref_(), pb = b.node_ref_()](const Series& g) {
    if (pa && pa->requires_grad) {
      detail::accumulate_grad_(pa, detail::reduce_grad_to_like_(g, pa->value, "sub"));
    }
    if (pb && pb->requires_grad) {
      const Series neg_g = g.neg();
      detail::accumulate_grad_(pb, detail::reduce_grad_to_like_(neg_g, pb->value, "sub"));
    }
  };
  return Variable(std::move(n));
}

inline Variable mul(const Variable& a, const Variable& b) {
  detail::require_defined_(a.value(), "mul");
  detail::require_defined_(b.value(), "mul");
  detail::require_same_platform_(a.value(), b.value(), "mul");
  Series out = a.value().mul(b.value());

  const bool req = a.requires_grad() || b.requires_grad();
  if (!req) return Variable(out, false);

  auto n = std::make_shared<detail::Node>(out, true, false);
  n->parents = {a.node_ref_(), b.node_ref_()};
  n->backward = [pa = a.node_ref_(), pb = b.node_ref_()](const Series& g) {
    if (pa && pa->requires_grad) {
      const Series ga = g.mul(pb->value);
      detail::accumulate_grad_(pa, detail::reduce_grad_to_like_(ga, pa->value, "mul"));
    }
    if (pb && pb->requires_grad) {
      const Series gb = g.mul(pa->value);
      detail::accumulate_grad_(pb, detail::reduce_grad_to_like_(gb, pb->value, "mul"));
    }
  };
  return Variable(std::move(n));
}

inline Variable div(const Variable& a, const Variable& b) {
  detail::require_defined_(a.value(), "div");
  detail::require_defined_(b.value(), "div");
  detail::require_same_platform_(a.value(), b.value(), "div");
  Series out = a.value().div(b.value());

  const bool req = a.requires_grad() || b.requires_grad();
  if (!req) return Variable(out, false);

  auto n = std::make_shared<detail::Node>(out, true, false);
  n->parents = {a.node_ref_(), b.node_ref_()};
  n->backward = [pa = a.node_ref_(), pb = b.node_ref_()](const Series& g) {
    if (pa && pa->requires_grad) {
      const Series ga = g.div(pb->value);
      detail::accumulate_grad_(pa, detail::reduce_grad_to_like_(ga, pa->value, "div"));
    }
    if (pb && pb->requires_grad) {
      const Series denom = pb->value.mul(pb->value);
      const Series gb = g.mul(pa->value).div(denom).neg();
      detail::accumulate_grad_(pb, detail::reduce_grad_to_like_(gb, pb->value, "div"));
    }
  };
  return Variable(std::move(n));
}

inline Variable neg(const Variable& x) {
  detail::require_defined_(x.value(), "neg");
  Series out = x.value().neg();
  if (!x.requires_grad()) return Variable(out, false);

  auto n = std::make_shared<detail::Node>(out, true, false);
  n->parents = {x.node_ref_()};
  n->backward = [px = x.node_ref_()](const Series& g) {
    if (px && px->requires_grad) {
      detail::accumulate_grad_(px, g.neg());
    }
  };
  return Variable(std::move(n));
}

inline Variable sum(const Variable& x) {
  detail::require_defined_(x.value(), "sum");
  Series out = x.value().sum();
  if (!x.requires_grad()) return Variable(out, false);

  auto n = std::make_shared<detail::Node>(out, true, false);
  n->parents = {x.node_ref_()};
  n->backward = [px = x.node_ref_()](const Series& g) {
    if (px && px->requires_grad) {
      const Series gx = pluss::ones_like(px->value, px->value.scalar_type()).mul(g);
      detail::accumulate_grad_(px, gx);
    }
  };
  return Variable(std::move(n));
}

inline Variable mean(const Variable& x) {
  detail::require_defined_(x.value(), "mean");
  Series out = x.value().mean();
  if (!x.requires_grad()) return Variable(out, false);

  auto n = std::make_shared<detail::Node>(out, true, false);
  n->parents = {x.node_ref_()};
  n->backward = [px = x.node_ref_()](const Series& g) {
    if (px && px->requires_grad) {
      const double inv_n = 1.0 / static_cast<double>(px->value.numel());
      const Series gx = pluss::ones_like(px->value, px->value.scalar_type()).mul(g).mul(inv_n);
      detail::accumulate_grad_(px, gx);
    }
  };
  return Variable(std::move(n));
}

inline Variable matmul(const Variable& a, const Variable& b) {
  detail::require_defined_(a.value(), "matmul");
  detail::require_defined_(b.value(), "matmul");
  detail::require_same_platform_(a.value(), b.value(), "matmul");
  if (a.value().dim() != 2 || b.value().dim() != 2) {
    throw std::runtime_error("subsp::math::optim::matmul: expected 2D inputs");
  }
  Series out = a.value().matmul(b.value());
  const bool req = a.requires_grad() || b.requires_grad();
  if (!req) return Variable(out, false);

  auto n = std::make_shared<detail::Node>(out, true, false);
  n->parents = {a.node_ref_(), b.node_ref_()};
  n->backward = [pa = a.node_ref_(), pb = b.node_ref_()](const Series& g) {
    if (pa && pa->requires_grad) {
      const Series gb_t = pb->value.transpose(0, 1);
      detail::accumulate_grad_(pa, g.matmul(gb_t));
    }
    if (pb && pb->requires_grad) {
      const Series ga_t = pa->value.transpose(0, 1);
      detail::accumulate_grad_(pb, ga_t.matmul(g));
    }
  };
  return Variable(std::move(n));
}

inline Variable operator+(const Variable& a, const Variable& b) { return add(a, b); }
inline Variable operator-(const Variable& a, const Variable& b) { return sub(a, b); }
inline Variable operator*(const Variable& a, const Variable& b) { return mul(a, b); }
inline Variable operator/(const Variable& a, const Variable& b) { return div(a, b); }

inline Variable operator+(const Variable& a, double s) { return add(a, make_scalar_like_(a, s)); }
inline Variable operator-(const Variable& a, double s) { return sub(a, make_scalar_like_(a, s)); }
inline Variable operator*(const Variable& a, double s) { return mul(a, make_scalar_like_(a, s)); }
inline Variable operator/(const Variable& a, double s) { return div(a, make_scalar_like_(a, s)); }

inline Variable operator+(double s, const Variable& a) { return add(make_scalar_like_(a, s), a); }
inline Variable operator-(double s, const Variable& a) { return sub(make_scalar_like_(a, s), a); }
inline Variable operator*(double s, const Variable& a) { return mul(make_scalar_like_(a, s), a); }
inline Variable operator/(double s, const Variable& a) { return div(make_scalar_like_(a, s), a); }

inline Variable operator-(const Variable& x) { return neg(x); }

inline void zero_grad(std::vector<Variable>& params) {
  for (auto& p : params) {
    p.zero_grad();
  }
}

template <class Fn>
std::pair<Series, std::vector<Series>> value_and_grad(Fn&& fn, const std::vector<Series>& params) {
  std::vector<Variable> vars;
  vars.reserve(params.size());
  for (const auto& p : params) {
    vars.emplace_back(p, true);
  }

  Variable loss = std::forward<Fn>(fn)(vars);
  if (!loss.defined()) {
    throw std::runtime_error("subsp::math::optim::value_and_grad: objective returned undefined Variable");
  }

  loss.backward();

  std::vector<Series> grads;
  grads.reserve(vars.size());
  for (const auto& v : vars) {
    grads.push_back(v.grad());
  }
  return {loss.value(), grads};
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
                              const std::vector<Series>& grads,
                              AdamState& state,
                              const AdamOptions& opt = {}) {
  if (params.size() != grads.size()) {
    throw std::runtime_error("subsp::math::optim::adam_step_inplace: params/grads size mismatch");
  }
  if (state.m.size() != params.size() || state.v.size() != params.size()) {
    state = adam_state_like(params);
  }

  ++state.step;
  const double b1_correction = 1.0 - std::pow(opt.beta1, static_cast<double>(state.step));
  const double b2_correction = 1.0 - std::pow(opt.beta2, static_cast<double>(state.step));

  for (std::size_t i = 0; i < params.size(); ++i) {
    detail::require_defined_(params[i], "adam_step_inplace");
    detail::require_defined_(grads[i], "adam_step_inplace");
    detail::require_same_platform_(params[i], grads[i], "adam_step_inplace");
    detail::require_same_platform_(params[i], state.m[i], "adam_step_inplace");
    detail::require_same_platform_(params[i], state.v[i], "adam_step_inplace");
    detail::require_float_dtype_(params[i], "adam_step_inplace");
    Series g = grads[i];
    if (!detail::same_shape_(params[i], g)) {
      if (params[i].numel() == 1 && g.numel() == 1) {
        g = g.sum();
      } else {
        throw std::runtime_error("subsp::math::optim::adam_step_inplace: gradient shape mismatch");
      }
    }

    if (opt.weight_decay != 0.0) {
      g = g.add(params[i].mul(opt.weight_decay));
    }

    state.m[i] = state.m[i].mul(opt.beta1).add(g.mul(1.0 - opt.beta1));
    state.v[i] = state.v[i].mul(opt.beta2).add(g.mul(g).mul(1.0 - opt.beta2));

    const Series mhat = state.m[i].div(b1_correction);
    const Series vhat = state.v[i].div(b2_correction);
    const Series update = mhat.div(vhat.sqrt().add(opt.eps)).mul(opt.learning_rate);
    params[i] = params[i].sub(update);
  }
}

inline std::vector<Series> adam_step(std::vector<Series> params,
                                     const std::vector<Series>& grads,
                                     AdamState& state,
                                     const AdamOptions& opt = {}) {
  adam_step_inplace(params, grads, state, opt);
  return params;
}

} // namespace pluss::subsp::math::optim
