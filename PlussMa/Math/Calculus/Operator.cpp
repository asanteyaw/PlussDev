#include "OperatorInternal.h"

#include "FunctionInternal.h"
#include "pluss/error.h"

#include <optional>
#include <utility>
#include <vector>

namespace pluss::subsp::math::operators::detail {
namespace {

Capabilities combine(const Capabilities &left,
                     const Capabilities &right) noexcept {
  return Capabilities{
      .structural = left.structural && right.structural,
      .evaluated_only = left.evaluated_only || right.evaluated_only,
      .linear = left.linear && right.linear,
      .adjoint_known = left.adjoint_known && right.adjoint_known,
      .serializable = left.serializable && right.serializable,
      .platform_pure = left.platform_pure && right.platform_pure,
  };
}

Capabilities from_options(const Options &options) noexcept {
  return Capabilities{
      .structural = options.structural,
      .evaluated_only = options.evaluated_only,
      .linear = options.linear,
      .adjoint_known = options.adjoint_known,
      .serializable = options.serializable,
      .platform_pure = options.platform_pure,
  };
}

} // namespace

Operator leaf(std::string key, Capabilities capabilities,
              std::function<math::Function(const math::Function &)> apply,
              OperatorKind kind, std::uint32_t version) {
  return OperatorAccess::make(pluss::msharp::intrusive_ptr<OperatorConcept>(
      pluss::msharp::intrusive_ptr<OperatorConcept>::adopt,
      new LeafOperator(std::move(key), version, capabilities, std::move(apply),
                       kind)));
}

void append_flat(const Operator &op, std::vector<Operator> &chain) {
  const OperatorConcept &descriptor = OperatorAccess::get(op);
  if (descriptor.kind() == OperatorKind::Identity) {
    return;
  }
  if (descriptor.kind() != OperatorKind::Composition) {
    chain.push_back(op);
    return;
  }
  const auto &composition =
      static_cast<const CompositionOperator &>(descriptor);
  chain.insert(chain.end(), composition.chain().begin(),
               composition.chain().end());
}

Capabilities chain_capabilities(const std::vector<Operator> &chain) {
  Capabilities capabilities{
      .structural = true,
      .evaluated_only = false,
      .linear = true,
      .adjoint_known = true,
      .serializable = true,
      .platform_pure = true,
  };
  for (const Operator &op : chain) {
    capabilities = combine(capabilities, op.capabilities());
  }
  return capabilities;
}

math::Function CompositionOperator::apply(const math::Function &input) const {
  math::Function output = input;
  for (const Operator &op : chain_) {
    output = op(output);
  }
  return output;
}

Operator
OperatorAccess::make(pluss::msharp::intrusive_ptr<OperatorConcept> impl) {
  PLUSS_CHECK(impl, "operators::Operator: implementation must not be empty");
  return Operator(std::move(impl));
}

const OperatorConcept &OperatorAccess::get(const Operator &op) {
  PLUSS_CHECK(op.impl_,
              "operators::Operator: operation requires a defined operator");
  return *op.impl_;
}

std::size_t OperatorAccess::component_count(const Operator &op) {
  const OperatorConcept &descriptor = get(op);
  if (descriptor.kind() == OperatorKind::Identity) {
    return 0;
  }
  if (descriptor.kind() == OperatorKind::Composition) {
    return static_cast<const CompositionOperator &>(descriptor).chain().size();
  }
  return 1;
}

void require_callable(bool present) {
  PLUSS_CHECK(present, "operators::define: callable must not be empty");
}

Operator
define_erased(std::string key,
              std::function<math::Function(const math::Function &)> callable,
              Options options) {
  PLUSS_CHECK(callable, "operators::define: callable must not be empty");
  PLUSS_CHECK(options.version > 0,
              "operators::define: version must be positive");
  PLUSS_CHECK(!(options.structural && options.evaluated_only),
              "operators::define: structural and evaluated_only cannot both "
              "be true");
  PLUSS_CHECK(!options.serializable,
              "operators::define: serializable custom operators require the "
              "codec contract introduced in calculus tranche 8");
  return leaf(std::move(key), from_options(options), std::move(callable),
              OperatorKind::Leaf, options.version);
}

} // namespace pluss::subsp::math::operators::detail

namespace pluss::subsp::math::operators {
namespace {

Capabilities structural_linear(bool adjoint_known = false) noexcept {
  return Capabilities{
      .structural = true,
      .evaluated_only = false,
      .linear = true,
      .adjoint_known = adjoint_known,
      .serializable = false,
      .platform_pure = true,
  };
}

Capabilities combine_parallel(const std::vector<Operator> &ops) {
  Capabilities capabilities = structural_linear(true);
  for (const Operator &op : ops) {
    const Capabilities current = op.capabilities();
    capabilities.structural &= current.structural;
    capabilities.evaluated_only |= current.evaluated_only;
    capabilities.linear &= current.linear;
    capabilities.adjoint_known &= current.adjoint_known;
    capabilities.serializable &= current.serializable;
    capabilities.platform_pure &= current.platform_pure;
  }
  return capabilities;
}

} // namespace

Operator::Operator() noexcept = default;
Operator::Operator(const Operator &) noexcept = default;
Operator::Operator(Operator &&) noexcept = default;
Operator &Operator::operator=(const Operator &) noexcept = default;
Operator &Operator::operator=(Operator &&) noexcept = default;
Operator::~Operator() = default;

Operator::Operator(
    pluss::msharp::intrusive_ptr<detail::OperatorConcept> impl) noexcept
    : impl_(std::move(impl)) {}

bool Operator::defined() const noexcept { return static_cast<bool>(impl_); }

math::Function Operator::operator()(const math::Function &input) const {
  PLUSS_CHECK(input.defined(),
              "operators::Operator: input Function must be defined");
  math::Function output = detail::OperatorAccess::get(*this).apply(input);
  PLUSS_CHECK(output.defined(),
              "operators::Operator: application returned an undefined "
              "Function");
  return output;
}

Capabilities Operator::capabilities() const {
  return detail::OperatorAccess::get(*this).capabilities();
}

std::string_view Operator::key() const {
  return detail::OperatorAccess::get(*this).key();
}

std::uint32_t Operator::version() const {
  return detail::OperatorAccess::get(*this).version();
}

Operator identity() {
  return detail::leaf(
      "identity", structural_linear(true),
      [](const math::Function &input) { return input; },
      detail::OperatorKind::Identity, 1);
}

Operator differential(std::string_view variable, std::int64_t order,
                      DiffOptions options) {
  PLUSS_CHECK(!variable.empty(),
              "operators::differential: variable must not be empty");
  PLUSS_CHECK(order > 0, "operators::differential: order must be positive");
  PLUSS_CHECK(options.max_nodes > 0,
              "operators::differential: max_nodes must be positive");
  return detail::leaf(
      "differential", structural_linear(),
      [variable = std::string(variable), order,
       options](const math::Function &input) {
        return math::detail::FunctionAccess::differentiate(
            input, variable, order, options.nonsmooth, options.max_nodes);
      },
      detail::OperatorKind::Leaf, 1);
}

Operator gradient(pluss::StackArray<std::string_view> variables,
                  DiffOptions options) {
  PLUSS_CHECK(options.max_nodes > 0,
              "operators::gradient: max_nodes must be positive");
  return detail::leaf(
      "gradient", structural_linear(),
      [variables = std::move(variables), options](const math::Function &input) {
        return math::detail::FunctionAccess::gradient(
            input, variables.lister().data(), options.nonsmooth,
            options.max_nodes);
      },
      detail::OperatorKind::Leaf, 1);
}

Operator jacobian(pluss::StackArray<std::string_view> variables,
                  DiffOptions options) {
  PLUSS_CHECK(options.max_nodes > 0,
              "operators::jacobian: max_nodes must be positive");
  return detail::leaf(
      "jacobian", structural_linear(),
      [variables = std::move(variables), options](const math::Function &input) {
        return math::detail::FunctionAccess::jacobian(
            input, variables.lister().data(), options.nonsmooth,
            options.max_nodes);
      },
      detail::OperatorKind::Leaf, 1);
}

Operator hessian(pluss::StackArray<std::string_view> variables,
                 DiffOptions options) {
  PLUSS_CHECK(options.max_nodes > 0,
              "operators::hessian: max_nodes must be positive");
  return detail::leaf(
      "hessian", structural_linear(),
      [variables = std::move(variables), options](const math::Function &input) {
        return math::detail::FunctionAccess::hessian(
            input, variables.lister().data(), options.nonsmooth,
            options.max_nodes);
      },
      detail::OperatorKind::Leaf, 1);
}

Operator antiderivative(std::string_view variable) {
  PLUSS_CHECK(!variable.empty(),
              "operators::antiderivative: variable must not be empty");
  return detail::leaf(
      "antiderivative", structural_linear(),
      [variable = std::string(variable)](const math::Function &input) {
        return math::detail::FunctionAccess::antiderivative(input, variable);
      },
      detail::OperatorKind::Leaf, 1);
}

Operator integral(std::string_view variable, double lower) {
  PLUSS_CHECK(!variable.empty(),
              "operators::integral: variable must not be empty");
  return detail::leaf(
      "integral", structural_linear(),
      [variable = std::string(variable), lower](const math::Function &input) {
        return math::detail::FunctionAccess::integral(input, variable, lower,
                                                      std::nullopt);
      },
      detail::OperatorKind::Leaf, 1);
}

Operator integral(std::string_view variable, double lower, double upper) {
  PLUSS_CHECK(!variable.empty(),
              "operators::integral: variable must not be empty");
  return detail::leaf(
      "integral", structural_linear(),
      [variable = std::string(variable), lower,
       upper](const math::Function &input) {
        return math::detail::FunctionAccess::integral(input, variable, lower,
                                                      upper);
      },
      detail::OperatorKind::Leaf, 1);
}

Operator shift(std::string_view variable, double amount) {
  PLUSS_CHECK(!variable.empty(),
              "operators::shift: variable must not be empty");
  return detail::leaf(
      "shift", structural_linear(),
      [variable = std::string(variable), amount](const math::Function &input) {
        return math::detail::FunctionAccess::shift(input, variable, amount);
      },
      detail::OperatorKind::Leaf, 1);
}

Operator multiply(math::Function coefficient) {
  PLUSS_CHECK(coefficient.defined(),
              "operators::multiply: coefficient must be defined");
  return detail::leaf(
      "multiply", structural_linear(),
      [coefficient = std::move(coefficient)](const math::Function &input) {
        return coefficient * input;
      },
      detail::OperatorKind::Leaf, 1);
}

Operator scale(double coefficient) {
  return detail::leaf(
      "scale", structural_linear(true),
      [coefficient](const math::Function &input) {
        return input * coefficient;
      },
      detail::OperatorKind::Leaf, 1);
}

Operator compose(const Operator &outer, const Operator &inner) {
  (void)detail::OperatorAccess::get(outer);
  (void)detail::OperatorAccess::get(inner);
  std::vector<Operator> chain;
  chain.reserve(detail::OperatorAccess::component_count(inner) +
                detail::OperatorAccess::component_count(outer));
  detail::append_flat(inner, chain);
  detail::append_flat(outer, chain);
  if (chain.empty()) {
    return identity();
  }
  if (chain.size() == 1) {
    return chain.front();
  }
  const Capabilities capabilities = detail::chain_capabilities(chain);
  return detail::OperatorAccess::make(
      pluss::msharp::intrusive_ptr<detail::OperatorConcept>(
          pluss::msharp::intrusive_ptr<detail::OperatorConcept>::adopt,
          new detail::CompositionOperator(std::move(chain), capabilities)));
}

Operator linear_combination(pluss::RangeView<Operator> ops,
                            pluss::RangeView<double> coefficients) {
  PLUSS_CHECK(!ops.empty(),
              "operators::linear_combination: at least one operator is "
              "required");
  PLUSS_CHECK(ops.size() == coefficients.size(),
              "operators::linear_combination: operator and coefficient "
              "counts must match");
  std::vector<Operator> owned_ops(ops.begin(), ops.end());
  std::vector<double> owned_coefficients(coefficients.begin(),
                                         coefficients.end());
  const Capabilities capabilities = combine_parallel(owned_ops);
  return detail::leaf(
      "linear_combination", capabilities,
      [ops = std::move(owned_ops),
       coefficients =
           std::move(owned_coefficients)](const math::Function &input) {
        math::Function output = ops.front()(input) * coefficients.front();
        for (std::size_t i = 1; i < ops.size(); ++i) {
          output = output + ops[i](input) * coefficients[i];
        }
        return output;
      },
      detail::OperatorKind::Leaf, 1);
}

Operator linear_combination(pluss::RangeView<Term> terms) {
  std::vector<Operator> ops;
  std::vector<double> coefficients;
  ops.reserve(terms.size());
  coefficients.reserve(terms.size());
  for (const auto &[coefficient, op] : terms) {
    coefficients.push_back(coefficient);
    ops.push_back(op);
  }
  return linear_combination(ops, coefficients);
}

Operator linear_combination(std::initializer_list<Term> terms) {
  return linear_combination(pluss::RangeView<Term>(terms));
}

Operator commutator(const Operator &left, const Operator &right) {
  const Capabilities left_capabilities = left.capabilities();
  const Capabilities right_capabilities = right.capabilities();
  Capabilities capabilities{
      .structural =
          left_capabilities.structural && right_capabilities.structural,
      .evaluated_only =
          left_capabilities.evaluated_only || right_capabilities.evaluated_only,
      .linear = left_capabilities.linear && right_capabilities.linear,
      .adjoint_known = false,
      .serializable = false,
      .platform_pure =
          left_capabilities.platform_pure && right_capabilities.platform_pure,
  };
  return detail::leaf(
      "commutator", capabilities,
      [left, right](const math::Function &input) {
        return left(right(input)) - right(left(input));
      },
      detail::OperatorKind::Leaf, 1);
}

} // namespace pluss::subsp::math::operators
