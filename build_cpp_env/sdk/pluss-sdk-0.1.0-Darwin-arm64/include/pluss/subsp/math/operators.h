#pragma once

#include "pluss/subsp/math/function.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace pluss::subsp::math::operators {

struct Capabilities final {
  bool structural = true;
  bool evaluated_only = false;
  bool linear = false;
  bool adjoint_known = false;
  bool serializable = false;
  bool platform_pure = true;
};

struct Options final {
  std::uint32_t version = 1;
  bool structural = true;
  bool evaluated_only = false;
  bool linear = false;
  bool adjoint_known = false;
  bool serializable = false;
  bool platform_pure = true;
  std::string payload{};
};

struct DiffOptions final {
  math::NonSmooth nonsmooth = math::NonSmooth::Strict;
  math::Fallback fallback = math::Fallback::Reject;
  std::size_t max_nodes = 100000;
};

namespace detail {
class OperatorConcept;
struct OperatorAccess;
} // namespace detail

class Operator final {
public:
  Operator() noexcept;
  Operator(const Operator &) noexcept;
  Operator(Operator &&) noexcept;
  Operator &operator=(const Operator &) noexcept;
  Operator &operator=(Operator &&) noexcept;
  ~Operator();

  [[nodiscard]] bool defined() const noexcept;
  explicit operator bool() const noexcept { return defined(); }

  [[nodiscard]] math::Function operator()(const math::Function &input) const;
  [[nodiscard]] Capabilities capabilities() const;
  [[nodiscard]] std::string_view key() const;
  [[nodiscard]] std::uint32_t version() const;

private:
  explicit Operator(
      pluss::msharp::intrusive_ptr<detail::OperatorConcept> impl) noexcept;

  pluss::msharp::intrusive_ptr<detail::OperatorConcept> impl_{};

  friend struct detail::OperatorAccess;
};

[[nodiscard]] Operator identity();
// |f^(n)|: the magnitude of the n-th derivative (formerly named
// absolute_differential; that name now belongs to geometry::).
[[nodiscard]] Operator magnitude_differential(std::string_view variable,
                                              std::int64_t order = 1,
                                              DiffOptions options = {});
[[nodiscard]] Operator covariant(std::string_view variable,
                                 math::Function connection,
                                 DiffOptions options = {});

// Differential geometry on a manifold with coordinates x^1..x^n. A connection
// is a math::Function of the coordinates with output shape [n, n, n] indexed
// Gamma[i][j][k] = Gamma^i_{jk}; a tensor field is a math::Function whose
// output axes are the tensor's indices, contravariant axes first and then
// covariant axes (TensorSignature).
namespace geometry {

struct TensorSignature final {
  std::int64_t contravariant = 0;
  std::int64_t covariant = 0;
};

// Levi-Civita connection of a metric g_ij(x): the output-rank-2 Function's
// variables (or `coordinates`, a subset in order) are the coordinates.
[[nodiscard]] math::Function
levi_civita(const math::Function &metric,
            pluss::StackArray<std::string_view> coordinates = {},
            DiffOptions options = {});

// Ricci / Levi-Civita absolute (intrinsic) differential along a curve:
//   DV/dt = dV/dt + sum_contravariant Gamma^i_{mk} V^m dx^k/dt
//                 - sum_covariant     Gamma^m_{jk} V_m dx^k/dt.
// `curve` is x^k(t), a rank-1 Function of `parameter`; the input is a tensor
// along the curve, a Function of `parameter` with `signature` indices.
[[nodiscard]] Operator
absolute_differential(math::Function curve, math::Function connection,
                      std::string_view parameter,
                      TensorSignature signature = {1, 0},
                      DiffOptions options = {});

// Covariant derivative of a tensor field V(x) in the direction of one
// coordinate: (nabla_k V) = d_k V + connection terms. The coordinate must be
// one of the connection's variables.
[[nodiscard]] Operator covariant(std::string_view coordinate,
                                 math::Function connection,
                                 TensorSignature signature,
                                 DiffOptions options = {});

} // namespace geometry
[[nodiscard]] Operator differential(std::string_view variable,
                                    std::int64_t order = 1,
                                    DiffOptions options = {});
[[nodiscard]] Operator
gradient(pluss::StackArray<std::string_view> variables = {},
         DiffOptions options = {});
[[nodiscard]] Operator
jacobian(pluss::StackArray<std::string_view> variables = {},
         DiffOptions options = {});
[[nodiscard]] Operator
hessian(pluss::StackArray<std::string_view> variables = {},
        DiffOptions options = {});
[[nodiscard]] Operator antiderivative(std::string_view variable);
[[nodiscard]] Operator integral(std::string_view variable, double lower);
[[nodiscard]] Operator integral(std::string_view variable, double lower,
                                double upper);
[[nodiscard]] Operator shift(std::string_view variable, double amount);
[[nodiscard]] Operator multiply(math::Function coefficient);
[[nodiscard]] Operator scale(double coefficient);
[[nodiscard]] Operator compose(const Operator &outer, const Operator &inner);
[[nodiscard]] Operator
linear_combination(pluss::RangeView<Operator> operators,
                   pluss::RangeView<double> coefficients);
using Term = std::pair<double, Operator>;
[[nodiscard]] Operator linear_combination(pluss::RangeView<Term> terms);
[[nodiscard]] Operator linear_combination(std::initializer_list<Term> terms);
[[nodiscard]] Operator commutator(const Operator &left, const Operator &right);

using Loader =
    std::function<Operator(std::string_view payload, std::uint32_t version)>;
bool bind(std::string_view key, std::uint32_t version, Loader loader);

namespace detail {
void require_callable(bool present);
[[nodiscard]] Operator
define_erased(std::string key,
              std::function<math::Function(const math::Function &)> callable,
              Options options);
} // namespace detail

template <class Callable>
[[nodiscard]] Operator define(std::string_view key, Callable &&callable,
                              Options options = {}) {
  using Value = std::remove_cvref_t<Callable>;
  static_assert(
      std::invocable<Value &, const math::Function &>,
      "operators::define: callable must accept const math::Function&");
  static_assert(
      std::same_as<std::invoke_result_t<Value &, const math::Function &>,
                   math::Function>,
      "operators::define: callable must return math::Function");
  if constexpr (requires(const Value &value) {
                  { static_cast<bool>(value) } -> std::same_as<bool>;
                }) {
    detail::require_callable(static_cast<bool>(callable));
  }
  auto stored = std::make_shared<Value>(std::forward<Callable>(callable));
  return detail::define_erased(
      std::string(key),
      [stored = std::move(stored)](const math::Function &input) {
        return std::invoke(*stored, input);
      },
      options);
}

template <class Callable> [[nodiscard]] Operator define(Callable &&callable) {
  return define({}, std::forward<Callable>(callable), Options{});
}

} // namespace pluss::subsp::math::operators
