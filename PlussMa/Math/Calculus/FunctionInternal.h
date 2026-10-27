#pragma once

#include "PrimitiveRegistry.h"
#include "pluss/subsp/math/function.h"

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace pluss::subsp::math::detail {

enum class NodeKind : std::uint8_t {
  Projection,
  ScalarConstant,
  SeriesConstant,
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
  primitive(PrimitiveId primitive,
            std::vector<pluss::msharp::intrusive_ptr<Node>> children,
            OutputMetadata output);
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
  [[nodiscard]] const std::vector<pluss::msharp::intrusive_ptr<Node>> &
  children() const noexcept {
    return children_;
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
       std::optional<double> integral_upper = std::nullopt)
      : kind_(kind), trace_id_(trace_id), variable_id_(variable_id),
        scalar_(scalar), series_(std::move(series)), primitive_(primitive),
        children_(std::move(children)), output_(std::move(output)),
        integral_variable_id_(integral_variable_id),
        integral_lower_(integral_lower), integral_upper_(integral_upper) {}

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
};

struct ExecutionStep final {
  pluss::msharp::intrusive_ptr<Node> node{};
  std::vector<std::size_t> inputs{};
};

class FunctionImpl final : public pluss::msharp::RefCounted {
public:
  FunctionImpl(pluss::msharp::intrusive_ptr<TraceState> trace,
               pluss::msharp::intrusive_ptr<Node> root, bool sealed,
               bool depends_on_variables);

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

private:
  pluss::msharp::intrusive_ptr<TraceState> trace_{};
  pluss::msharp::intrusive_ptr<Node> root_{};
  bool sealed_ = false;
  bool depends_on_variables_ = false;
  std::vector<ExecutionStep> plan_{};
  std::unordered_map<std::string, std::size_t> binding_index_{};
};

struct FunctionAccess final {
  [[nodiscard]] static Function
  make(pluss::msharp::intrusive_ptr<TraceState> trace,
       pluss::msharp::intrusive_ptr<Node> root, bool sealed,
       bool depends_on_variables);
  [[nodiscard]] static const FunctionImpl &get(const Function &function);
  [[nodiscard]] static Function unary(PrimitiveId primitive,
                                      const Function &input);
  [[nodiscard]] static Function
  binary(PrimitiveId primitive, const Function &left, const Function &right);
  [[nodiscard]] static Function argument(const Function &function,
                                         std::string_view variable);
  [[nodiscard]] static Function assume(const Function &function,
                                       std::string_view variable,
                                       Assumption assumption);
  [[nodiscard]] static Function shift(const Function &function,
                                      std::string_view variable, double amount);
  [[nodiscard]] static Function
  differentiate(const Function &function, std::string_view variable,
                std::int64_t order, NonSmooth nonsmooth, std::size_t max_nodes);
  [[nodiscard]] static Function
  gradient(const Function &function,
           pluss::RangeView<std::string_view> variables, NonSmooth nonsmooth,
           std::size_t max_nodes);
  [[nodiscard]] static Function
  jacobian(const Function &function,
           pluss::RangeView<std::string_view> variables, NonSmooth nonsmooth,
           std::size_t max_nodes);
  [[nodiscard]] static Function
  hessian(const Function &function,
          pluss::RangeView<std::string_view> variables, NonSmooth nonsmooth,
          std::size_t max_nodes);
  [[nodiscard]] static Function antiderivative(const Function &function,
                                               std::string_view variable);
  [[nodiscard]] static Function integral(const Function &function,
                                         std::string_view variable,
                                         double lower,
                                         std::optional<double> upper);
  [[nodiscard]] static pluss::Series
  evaluate(const Function &function, pluss::RangeView<pluss::Series> arguments);
};

} // namespace pluss::subsp::math::detail
