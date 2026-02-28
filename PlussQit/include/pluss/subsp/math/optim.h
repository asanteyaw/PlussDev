#pragma once

#include "pluss/subsp/math/diff.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_set>
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

// Compatibility re-export: existing callers can continue to use the older
// optim-facing trace helpers while new derivative-oriented code targets
// pluss::subsp::math::diff directly.
using pluss::subsp::math::diff::Trace;
using pluss::subsp::math::diff::DualTrace;
using pluss::subsp::math::diff::trace;
using pluss::subsp::math::diff::fixed;
using pluss::subsp::math::diff::add;
using pluss::subsp::math::diff::sub;
using pluss::subsp::math::diff::mul;
using pluss::subsp::math::diff::div;
using pluss::subsp::math::diff::neg;
using pluss::subsp::math::diff::pow;
using pluss::subsp::math::diff::exp;
using pluss::subsp::math::diff::log;
using pluss::subsp::math::diff::sqrt;
using pluss::subsp::math::diff::sin;
using pluss::subsp::math::diff::cos;
using pluss::subsp::math::diff::tanh;
using pluss::subsp::math::diff::square;
using pluss::subsp::math::diff::rsqrt;
using pluss::subsp::math::diff::relu;
using pluss::subsp::math::diff::sigmoid;
using pluss::subsp::math::diff::softplus;
using pluss::subsp::math::diff::clamp;
using pluss::subsp::math::diff::sum;
using pluss::subsp::math::diff::mean;
using pluss::subsp::math::diff::matmul;
using pluss::subsp::math::diff::clear_sensitivities;
using pluss::subsp::math::diff::grad;
using pluss::subsp::math::diff::hvp;
using pluss::subsp::math::diff::jvp;
using pluss::subsp::math::diff::linearize;
using pluss::subsp::math::diff::vjp;
using pluss::subsp::math::diff::value_and_grad;

struct ParameterBlockLayout {
  std::string name;
  std::vector<std::int64_t> sizes;
  std::int64_t numel{0};
  ScalarType scalar_type{ScalarType::Float32};
  pluss::codem::Platform platform{pluss::codem::Platform::CPU};
};

class ParameterLayout {
public:
  ParameterLayout() = default;
  explicit ParameterLayout(std::vector<ParameterBlockLayout> blocks) : blocks_(std::move(blocks)) {}

  const std::vector<ParameterBlockLayout>& blocks() const noexcept { return blocks_; }
  std::size_t size() const noexcept { return blocks_.size(); }
  bool empty() const noexcept { return blocks_.empty(); }

  std::int64_t total_numel() const noexcept {
    std::int64_t total = 0;
    for (const auto& block : blocks_) total += block.numel;
    return total;
  }

  bool contains(std::string_view name) const noexcept {
    return std::any_of(
        blocks_.begin(), blocks_.end(), [&](const auto& block) { return block.name == name; });
  }

  const ParameterBlockLayout& at(std::string_view name) const {
    for (const auto& block : blocks_) {
      if (block.name == name) return block;
    }
    throw std::runtime_error("subsp::math::optim::ParameterLayout::at: unknown parameter block");
  }

private:
  std::vector<ParameterBlockLayout> blocks_{};
};

template <class Fn>
std::pair<Series, std::vector<Series>> value_and_sensitivity(Fn&& fn, const std::vector<Series>& params) {
  return pluss::subsp::math::diff::value_and_grad(std::forward<Fn>(fn), params);
}

template <class T>
class ParameterPack {
public:
  struct Block {
    std::string name;
    T value;
  };

  ParameterPack() = default;
  explicit ParameterPack(std::vector<Block> blocks) : blocks_(std::move(blocks)) {
    validate_names_();
  }

  std::size_t size() const noexcept { return blocks_.size(); }
  bool empty() const noexcept { return blocks_.empty(); }

  const Block& operator[](std::size_t index) const { return blocks_.at(index); }
  Block& operator[](std::size_t index) { return blocks_.at(index); }

  const std::vector<Block>& blocks() const noexcept { return blocks_; }
  std::int64_t total_numel() const noexcept {
    std::int64_t total = 0;
    for (const auto& block : blocks_) total += block.value.numel();
    return total;
  }

  bool contains(std::string_view name) const noexcept {
    return std::any_of(
        blocks_.begin(), blocks_.end(), [&](const Block& block) { return block.name == name; });
  }

  const T& at(std::string_view name) const {
    for (const auto& block : blocks_) {
      if (block.name == name) return block.value;
    }
    throw std::runtime_error("subsp::math::optim::ParameterPack::at: unknown parameter block");
  }

  T& at(std::string_view name) {
    for (auto& block : blocks_) {
      if (block.name == name) return block.value;
    }
    throw std::runtime_error("subsp::math::optim::ParameterPack::at: unknown parameter block");
  }

  std::vector<T> values() const {
    std::vector<T> out;
    out.reserve(blocks_.size());
    for (const auto& block : blocks_) {
      out.push_back(block.value);
    }
    return out;
  }

  template <class U>
  ParameterPack<U> rebind(std::vector<U> values) const {
    if (values.size() != blocks_.size()) {
      throw std::runtime_error("subsp::math::optim::ParameterPack::rebind: size mismatch");
    }

    std::vector<typename ParameterPack<U>::Block> rebound;
    rebound.reserve(blocks_.size());
    for (std::size_t i = 0; i < blocks_.size(); ++i) {
      rebound.push_back(typename ParameterPack<U>::Block{blocks_[i].name, std::move(values[i])});
    }
    return ParameterPack<U>(std::move(rebound));
  }

  ParameterLayout layout() const requires std::is_same_v<T, Series> {
    std::vector<ParameterBlockLayout> out;
    out.reserve(blocks_.size());
    for (const auto& block : blocks_) {
      detail::require_defined_(block.value, "ParameterPack::layout");
      const auto sizes = block.value.sizes();
      out.push_back(ParameterBlockLayout{
          block.name,
          std::vector<std::int64_t>(sizes.begin(), sizes.end()),
          block.value.numel(),
          block.value.scalar_type(),
          block.value.platform()});
    }
    return ParameterLayout(std::move(out));
  }

  bool same_layout(const ParameterPack<Series>& other) const requires std::is_same_v<T, Series> {
    if (blocks_.size() != other.blocks_.size()) return false;
    for (std::size_t i = 0; i < blocks_.size(); ++i) {
      const auto& lhs = blocks_[i];
      const auto& rhs = other.blocks_[i];
      if (lhs.name != rhs.name) return false;
      if (lhs.value.scalar_type() != rhs.value.scalar_type()) return false;
      if (lhs.value.platform() != rhs.value.platform()) return false;
      if (!detail::same_shape_(lhs.value, rhs.value)) return false;
    }
    return true;
  }

  Series flatten_values() const requires std::is_same_v<T, Series> {
    if (blocks_.empty()) {
      throw std::runtime_error("subsp::math::optim::ParameterPack::flatten_values: empty pack");
    }

    std::vector<Series> flat;
    flat.reserve(blocks_.size());
    const auto platform = blocks_.front().value.platform();
    const auto scalar_type = blocks_.front().value.scalar_type();
    for (const auto& block : blocks_) {
      detail::require_defined_(block.value, "ParameterPack::flatten_values");
      detail::require_same_platform_(blocks_.front().value, block.value, "ParameterPack::flatten_values");
      if (block.value.scalar_type() != scalar_type) {
        throw std::runtime_error(
            "subsp::math::optim::ParameterPack::flatten_values: expected matching dtypes");
      }
      (void)platform;
      flat.push_back(block.value.reshape({block.value.numel()}));
    }

    if (flat.size() == 1) return flat.front();
    return flat.front().unite(std::vector<Series>(flat.begin() + 1, flat.end()), 0);
  }

  ParameterPack<Series> zeros_like() const requires std::is_same_v<T, Series> {
    std::vector<typename ParameterPack<Series>::Block> out;
    out.reserve(blocks_.size());
    for (const auto& block : blocks_) {
      detail::require_defined_(block.value, "ParameterPack::zeros_like");
      out.push_back({block.name, pluss::zeros_like(block.value, block.value.scalar_type())});
    }
    return ParameterPack<Series>(std::move(out));
  }

private:
  void validate_names_() const {
    std::unordered_set<std::string> seen;
    seen.reserve(blocks_.size());
    for (const auto& block : blocks_) {
      if (block.name.empty()) {
        throw std::runtime_error("subsp::math::optim::ParameterPack: block names must be non-empty");
      }
      if (!seen.insert(block.name).second) {
        throw std::runtime_error("subsp::math::optim::ParameterPack: duplicate block name");
      }
    }
  }

  std::vector<Block> blocks_{};
};

template <class Fn>
std::pair<Series, ParameterPack<Series>> value_and_grad(Fn&& fn, const ParameterPack<Series>& params) {
  auto [value, grads] = pluss::subsp::math::diff::value_and_grad(
      [&](const std::vector<Trace>& xs) -> Trace {
        return std::forward<Fn>(fn)(params.template rebind<Trace>(xs));
      },
      params.values());
  return {value, params.template rebind<Series>(std::move(grads))};
}

inline ParameterPack<Series> unflatten_values(const ParameterLayout& layout, const Series& flat_values) {
  detail::require_defined_(flat_values, "unflatten_values");

  Series flat = flat_values.reshape({flat_values.numel()});
  if (flat.numel() != layout.total_numel()) {
    throw std::runtime_error("subsp::math::optim::unflatten_values: flat size mismatch");
  }

  std::vector<typename ParameterPack<Series>::Block> blocks;
  blocks.reserve(layout.size());

  std::int64_t offset = 0;
  for (const auto& block : layout.blocks()) {
    const Series idx = pluss::arange(offset, offset + block.numel, 1, ScalarType::Int64).to(flat.platform());
    Series piece = flat.index({idx}).contiguous();
    if (block.sizes.empty()) {
      piece = piece.reshape({});
    } else {
      piece = piece.reshape(block.sizes);
    }
    if (piece.scalar_type() != block.scalar_type) piece = piece.to(block.scalar_type);
    if (piece.platform() != block.platform) piece = piece.to(block.platform);
    blocks.push_back({block.name, std::move(piece)});
    offset += block.numel;
  }

  return ParameterPack<Series>(std::move(blocks));
}

template <class Fn>
ParameterPack<Series> grad(Fn&& fn, const ParameterPack<Series>& params) {
  return value_and_grad(std::forward<Fn>(fn), params).second;
}

template <class Fn>
std::pair<Series, Series> jvp(Fn&& fn,
                              const ParameterPack<Series>& params,
                              const ParameterPack<Series>& tangents) {
  return pluss::subsp::math::diff::jvp(
      [&](const std::vector<DualTrace>& xs) -> DualTrace {
        return std::forward<Fn>(fn)(params.template rebind<DualTrace>(xs));
      },
      params.values(),
      tangents.values());
}

template <class Fn>
std::pair<Series, ParameterPack<Series>> hvp(Fn&& fn,
                                             const ParameterPack<Series>& params,
                                             const ParameterPack<Series>& tangents,
                                             const pluss::subsp::math::diff::HvpOptions& opt = {}) {
  using FnType = std::decay_t<Fn>;
  auto params_values = params.values();
  auto tangent_values = tangents.values();

  std::pair<Series, std::vector<Series>> raw{};
  if constexpr (std::is_invocable_v<FnType&, const ParameterPack<DualTrace>&>) {
    FnType fn_copy(std::forward<Fn>(fn));
    raw = pluss::subsp::math::diff::hvp(
        [&](const std::vector<DualTrace>& xs) -> DualTrace {
          return fn_copy(params.template rebind<DualTrace>(xs));
        },
        params_values,
        tangent_values,
        opt);
  } else {
    raw = pluss::subsp::math::diff::hvp(
        [&](const std::vector<Trace>& xs) -> Trace {
          return std::forward<Fn>(fn)(params.template rebind<Trace>(xs));
        },
        params_values,
        tangent_values,
        opt);
  }
  auto& [value, hv] = raw;
  return {value, params.template rebind<Series>(std::move(hv))};
}

template <class Fn>
auto linearize(Fn&& fn, const ParameterPack<Series>& params) {
  auto [value, pushforward] = pluss::subsp::math::diff::linearize(
      [&](const std::vector<DualTrace>& xs) -> DualTrace {
        return std::forward<Fn>(fn)(params.template rebind<DualTrace>(xs));
      },
      params.values());

  auto pack_pushforward = [pushforward = std::move(pushforward)](const ParameterPack<Series>& tangents) mutable -> Series {
    return pushforward(tangents.values());
  };
  return std::pair{value, std::move(pack_pushforward)};
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
