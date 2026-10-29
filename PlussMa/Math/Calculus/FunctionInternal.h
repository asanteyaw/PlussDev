#pragma once

#include "PrimitiveRegistry.h"
#include "pluss/subsp/math/function.h"

#include <cstdint>
#include <functional>
#include <iosfwd>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace pluss::subsp::math::detail {

[[nodiscard]] std::uint64_t fresh_trace_id() noexcept;

enum class NodeKind : std::uint8_t {
  Projection,
  ScalarConstant,
  SeriesConstant,
  Opaque,
  Primitive,
  Integral,
};

struct OutputMetadata final {
  std::vector<std::int64_t> shape{};

  [[nodiscard]] std::int64_t rank() const noexcept {
    return static_cast<std::int64_t>(shape.size());
  }
};

class TraceState final : public pluss::msharp::RefCounted {
public:
  TraceState(std::uint64_t id, pluss::msharp::Lister::StringView variables,
             std::vector<Assumption> assumptions = {});

  [[nodiscard]] std::uint64_t id() const noexcept { return id_; }
  [[nodiscard]] const pluss::msharp::Lister::StringView &
  variables() const noexcept {
    return variables_;
  }
  [[nodiscard]] const std::vector<Assumption> &assumptions() const noexcept {
    return assumptions_;
  }

private:
  std::uint64_t id_ = 0;
  pluss::msharp::Lister::StringView variables_{};
  std::vector<Assumption> assumptions_{};
};

class Node final : public pluss::msharp::RefCounted {
public:
  static pluss::msharp::intrusive_ptr<Node>
  projection(std::uint64_t trace_id, std::int64_t variable_id);
  static pluss::msharp::intrusive_ptr<Node> scalar(double value);
  static pluss::msharp::intrusive_ptr<Node> series(pluss::Series value);
  static pluss::msharp::intrusive_ptr<Node>
  opaque(std::function<pluss::Series(const pluss::Series &)> callable,
         std::int64_t variable_id, std::int64_t derivative_order = 0);
  static pluss::msharp::intrusive_ptr<Node>
  primitive(PrimitiveId primitive,
            std::vector<pluss::msharp::intrusive_ptr<Node>> children,
            OutputMetadata output, std::vector<std::int64_t> attributes = {});
  static pluss::msharp::intrusive_ptr<Node>
  integral(pluss::msharp::intrusive_ptr<Node> integrand,
           std::int64_t variable_id, double lower,
           std::optional<double> upper = std::nullopt);

  [[nodiscard]] NodeKind kind() const noexcept { return kind_; }
  [[nodiscard]] std::uint64_t trace_id() const noexcept { return trace_id_; }
  [[nodiscard]] std::int64_t variable_id() const noexcept {
    return variable_id_;
  }
  [[nodiscard]] const OutputMetadata &output() const noexcept {
    return output_;
  }
  [[nodiscard]] double scalar_value() const noexcept { return scalar_; }
  [[nodiscard]] const pluss::Series &series_value() const noexcept {
    return series_;
  }
  [[nodiscard]] PrimitiveId primitive_id() const noexcept { return primitive_; }
  [[nodiscard]] const std::function<pluss::Series(const pluss::Series &)> &
  opaque_callable() const noexcept {
    return opaque_;
  }
  [[nodiscard]] std::int64_t opaque_derivative_order() const noexcept {
    return opaque_derivative_order_;
  }
  [[nodiscard]] const std::vector<pluss::msharp::intrusive_ptr<Node>> &
  children() const noexcept {
    return children_;
  }
  // Integer parameters of tensor primitives (axes, ranks, indices).
  [[nodiscard]] const std::vector<std::int64_t> &attributes() const noexcept {
    return attributes_;
  }
  [[nodiscard]] std::int64_t integral_variable_id() const noexcept {
    return integral_variable_id_;
  }
  [[nodiscard]] double integral_lower() const noexcept {
    return integral_lower_;
  }
  [[nodiscard]] const std::optional<double> &integral_upper() const noexcept {
    return integral_upper_;
  }

private:
  Node(NodeKind kind, std::uint64_t trace_id, std::int64_t variable_id,
       double scalar, pluss::Series series, PrimitiveId primitive,
       std::vector<pluss::msharp::intrusive_ptr<Node>> children,
       OutputMetadata output, std::int64_t integral_variable_id = -1,
       double integral_lower = 0.0,
       std::optional<double> integral_upper = std::nullopt,
       std::function<pluss::Series(const pluss::Series &)> opaque = {},
       std::int64_t opaque_derivative_order = 0)
      : kind_(kind), trace_id_(trace_id), variable_id_(variable_id),
        scalar_(scalar), series_(std::move(series)), primitive_(primitive),
        children_(std::move(children)), output_(std::move(output)),
        integral_variable_id_(integral_variable_id),
        integral_lower_(integral_lower), integral_upper_(integral_upper),
        opaque_(std::move(opaque)),
        opaque_derivative_order_(opaque_derivative_order) {}

  NodeKind kind_ = NodeKind::ScalarConstant;
  std::uint64_t trace_id_ = 0;
  std::int64_t variable_id_ = -1;
  double scalar_ = 0.0;
  pluss::Series series_{};
  PrimitiveId primitive_ = PrimitiveId::Add;
  std::vector<pluss::msharp::intrusive_ptr<Node>> children_{};
  OutputMetadata output_{};
  std::int64_t integral_variable_id_ = -1;
  double integral_lower_ = 0.0;
  std::optional<double> integral_upper_{};
  std::function<pluss::Series(const pluss::Series &)> opaque_{};
  std::int64_t opaque_derivative_order_ = 0;
  std::vector<std::int64_t> attributes_{};
};

struct ExecutionStep final {
  pluss::msharp::intrusive_ptr<Node> node{};
  std::vector<std::size_t> inputs{};
  std::vector<std::size_t> releases{};
  // Integral children are deliberately excluded from the outer topological
  // order. Compile their immutable subplan once with the parent Function.
  std::vector<ExecutionStep> integral_plan{};
};

class FunctionImpl final : public pluss::msharp::RefCounted {
public:
  FunctionImpl(pluss::msharp::intrusive_ptr<TraceState> trace,
               pluss::msharp::intrusive_ptr<Node> root, bool sealed,
               bool depends_on_variables,
               std::vector<std::int64_t> bound_variables = {});

  [[nodiscard]] const TraceState &trace() const noexcept { return *trace_; }
  [[nodiscard]] const pluss::msharp::intrusive_ptr<TraceState> &
  trace_ptr() const noexcept {
    return trace_;
  }
  [[nodiscard]] const Node &root() const noexcept { return *root_; }
  [[nodiscard]] const pluss::msharp::intrusive_ptr<Node> &
  root_ptr() const noexcept {
    return root_;
  }
  [[nodiscard]] bool sealed() const noexcept { return sealed_; }
  [[nodiscard]] bool depends_on_variables() const noexcept {
    return depends_on_variables_;
  }
  [[nodiscard]] const std::vector<ExecutionStep> &plan() const noexcept {
    return plan_;
  }
  [[nodiscard]] const std::unordered_map<std::string, std::size_t> &
  binding_index() const noexcept {
    return binding_index_;
  }
  [[nodiscard]] const std::vector<std::int64_t> &
  active_variables() const noexcept {
    return active_variables_;
  }
  [[nodiscard]] const pluss::msharp::Lister::StringView &
  active_names() const noexcept {
    return active_names_;
  }
  [[nodiscard]] const std::unordered_map<std::string, std::size_t> &
  active_binding_index() const noexcept {
    return active_binding_index_;
  }
  [[nodiscard]] const std::vector<std::int64_t> &
  bound_variables() const noexcept {
    return bound_variables_;
  }

private:
  pluss::msharp::intrusive_ptr<TraceState> trace_{};
  pluss::msharp::intrusive_ptr<Node> root_{};
  bool sealed_ = false;
  bool depends_on_variables_ = false;
  std::vector<ExecutionStep> plan_{};
  std::unordered_map<std::string, std::size_t> binding_index_{};
  std::vector<std::int64_t> active_variables_{};
  pluss::msharp::Lister::StringView active_names_{};
  std::unordered_map<std::string, std::size_t> active_binding_index_{};
  std::vector<std::int64_t> bound_variables_{};
};

struct FunctionAccess final {
  [[nodiscard]] static Function
  make(pluss::msharp::intrusive_ptr<TraceState> trace,
       pluss::msharp::intrusive_ptr<Node> root, bool sealed,
       bool depends_on_variables,
       std::vector<std::int64_t> bound_variables = {});
  [[nodiscard]] static const FunctionImpl &get(const Function &function);
  [[nodiscard]] static Function unary(PrimitiveId primitive,
                                      const Function &input);
  [[nodiscard]] static Function
  binary(PrimitiveId primitive, const Function &left, const Function &right);
  [[nodiscard]] static Function tensor(PrimitiveId primitive,
                                       std::vector<Function> inputs,
                                       std::vector<std::int64_t> attributes);
  [[nodiscard]] static Function stack(pluss::RangeView<Function> inputs);
  [[nodiscard]] static bool contains_opaque(const Function &function);
  // Truncated Taylor coefficients f^(k)(a)/k!, k = 0..order, of a scalar
  // elementwise Function in `variable` about `center`, built as an analytic
  // graph of Function operations by power-series arithmetic (O(order^2) nodes
  // per operation, no derivative graph). The result has shape [order + 1] and
  // depends on the other variables only.
  [[nodiscard]] static bool supports_power_series(const Function &function);
  [[nodiscard]] static Function power_series_coefficients(const Function &function,
                                                          std::string_view variable,
                                                          double center,
                                                          std::int64_t order);
  [[nodiscard]] static std::int64_t variable_index(const Function &function,
                                                   std::string_view variable);
  [[nodiscard]] static Function compose(const Function &function,
                                        pluss::RangeView<Function> arguments);
  // Closed form of an integral transform of `lifted` (a Function whose trace already holds
  // the frequency variable) when the integrand is an exponential polynomial
  // sum c x^n exp(e1 x + e2 x^2) (sums, products, integer powers, exp / sin / cos / sinh / cosh of an
  // expression quadratic or linear in x, division by x-free expressions). kind: 0 Laplace,
  // 1 cosine, 2 sine, 3 Fourier, 4 inverse Fourier; `ordinary` uses the phase 2 pi w. Returns an
  // undefined Function when no closed form applies. Valid inside the region of convergence
  // (Re s above every exponent; a positive Gaussian coefficient); outside it the formula is the
  // analytic continuation.
  [[nodiscard]] static Function transform_table(const Function &lifted,
                                                std::string_view variable,
                                                std::string_view frequency,
                                                int kind, bool ordinary);
  // true when the graph is the constant zero (a derivative that vanished identically)
  [[nodiscard]] static bool is_zero(const Function &function);
  // `function` with every occurrence of `variable` replaced by `by` (both on one trace); the
  // variables of `by` join the dependence of the result.
  [[nodiscard]] static Function replace(const Function &function,
                                        std::string_view variable,
                                        const Function &by);
  // Closed form of a Riemann-Liouville integral (mode 0), Riemann-Liouville derivative (1) or Caputo
  // derivative (2) of order `alpha` from 0 when `function` is a polynomial in `variable`; undefined
  // otherwise. `order` is ceil(alpha) for the derivatives.
  [[nodiscard]] static Function fractional_table(const Function &function,
                                                 std::string_view variable,
                                                 double alpha, int mode);
  // The Function with `variable` fixed at `value`: the same trace, the variable
  // now bound (so it no longer counts as an argument).
  [[nodiscard]] static Function fix(const Function &function,
                                    std::string_view variable, double value);
  // The same graph over a new trace that appends one free variable `name`
  // (its projection comes back in `variable`). Every existing variable keeps its
  // index, so integrals and bound variables carry over unchanged.
  [[nodiscard]] static Function extend(const Function &function,
                                       std::string_view name,
                                       Function &variable);
  [[nodiscard]] static Function argument(const Function &function,
                                         std::string_view variable);
  [[nodiscard]] static Function assume(const Function &function,
                                       std::string_view variable,
                                       Assumption assumption);
  [[nodiscard]] static Function shift(const Function &function,
                                      std::string_view variable, double amount);
  [[nodiscard]] static Function rebind(const Function &function,
                                       const Function &target);
  [[nodiscard]] static Function
  differentiate(const Function &function, std::string_view variable,
                std::int64_t order, NonSmooth nonsmooth, Fallback fallback,
                std::size_t max_nodes);
  [[nodiscard]] static Function
  wirtinger(const Function &function, std::string_view variable,
            std::int64_t order, bool conjugate, NonSmooth nonsmooth,
            std::size_t max_nodes);
  [[nodiscard]] static Function
  gradient(const Function &function,
           pluss::RangeView<std::string_view> variables, NonSmooth nonsmooth,
           Fallback fallback, std::size_t max_nodes);
  [[nodiscard]] static Function
  jacobian(const Function &function,
           pluss::RangeView<std::string_view> variables, NonSmooth nonsmooth,
           Fallback fallback, std::size_t max_nodes);
  [[nodiscard]] static Function
  hessian(const Function &function,
          pluss::RangeView<std::string_view> variables, NonSmooth nonsmooth,
          Fallback fallback, std::size_t max_nodes);
  [[nodiscard]] static Function antiderivative(const Function &function,
                                               std::string_view variable);
  [[nodiscard]] static Function integral(const Function &function,
                                         std::string_view variable,
                                         double lower,
                                         std::optional<double> upper);
  [[nodiscard]] static pluss::Series
  evaluate(const Function &function, pluss::RangeView<pluss::Series> arguments);
  static void save(const Function &function, std::ostream &output);
  [[nodiscard]] static Function load(std::istream &input);
};

} // namespace pluss::subsp::math::detail
