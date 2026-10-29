#include "OperatorInternal.h"

#include "FunctionInternal.h"
#include "pluss/error.h"
#include "pluss/subsp/math/calculus_io.h"

#include <bit>
#include <iomanip>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <sstream>
#include <tuple>
#include <unordered_map>
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
      .path_functional = left.path_functional || right.path_functional,
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

// Payload of operators that hold only numbers and names: one field per line (doubles as exact bit patterns).
std::string pack(std::initializer_list<std::string> fields) {
  std::string out;
  for (const std::string &field : fields) {
    PLUSS_CHECK(field.find('\n') == std::string::npos,
                "operators::stochastic: variable names must not contain line breaks");
    out += field;
    out += '\n';
  }
  return out;
}
std::vector<std::string> unpack(std::string_view payload, std::size_t expected) {
  std::vector<std::string> fields;
  std::size_t begin = 0;
  while (begin < payload.size()) {
    const std::size_t end = payload.find('\n', begin);
    PLUSS_CHECK(end != std::string_view::npos,
                "math::io: malformed stochastic operator descriptor");
    fields.emplace_back(payload.substr(begin, end - begin));
    begin = end + 1;
  }
  PLUSS_CHECK(fields.size() == expected,
              "math::io: malformed stochastic operator descriptor");
  return fields;
}
std::string number(double value) { return std::to_string(std::bit_cast<std::int64_t>(value)); }
double real(const std::string &text) { return std::bit_cast<double>(static_cast<std::int64_t>(std::stoll(text))); }

Capabilities serializable(Capabilities capabilities) {
  capabilities.serializable = true;
  return capabilities;
}

std::string pack_all(const std::vector<std::string> &fields) {
  std::string out;
  for (const std::string &field : fields) {
    PLUSS_CHECK(field.find('\n') == std::string::npos,
                "operators: names must not contain line breaks");
    out += field;
    out += '\n';
  }
  return out;
}
std::vector<std::string> unpack_all(std::string_view payload) {
  std::vector<std::string> fields;
  std::size_t begin = 0;
  while (begin < payload.size()) {
    const std::size_t end = payload.find('\n', begin);
    PLUSS_CHECK(end != std::string_view::npos,
                "math::io: malformed operator descriptor");
    fields.emplace_back(payload.substr(begin, end - begin));
    begin = end + 1;
  }
  return fields;
}

} // namespace

Operator leaf(std::string key, Capabilities capabilities,
              std::function<math::Function(const math::Function &)> apply,
              OperatorKind kind, std::uint32_t version,
              std::string payload = {},
              std::function<Operator()> adjoint = {}) {
  return OperatorAccess::make(pluss::msharp::intrusive_ptr<OperatorConcept>(
      pluss::msharp::intrusive_ptr<OperatorConcept>::adopt,
      new LeafOperator(std::move(key), version, capabilities, std::move(apply),
                       kind, std::move(payload), std::move(adjoint))));
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

std::string_view OperatorAccess::payload(const Operator &op) {
  return get(op).payload();
}

const std::vector<Operator> &OperatorAccess::chain(const Operator &op) {
  const OperatorConcept &descriptor = get(op);
  PLUSS_CHECK(descriptor.kind() == OperatorKind::Composition,
              "math::io: operator is not a composition");
  return static_cast<const CompositionOperator &>(descriptor).chain();
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
  PLUSS_CHECK(!options.serializable || !key.empty(),
              "operators::define: a serializable custom operator requires a "
              "stable key");
  return leaf(std::move(key), from_options(options), std::move(callable),
              OperatorKind::Leaf, options.version, std::move(options.payload));
}

} // namespace pluss::subsp::math::operators::detail

namespace pluss::subsp::math::operators {
namespace {

std::string codec_key(std::string_view key, std::uint32_t version) {
  return std::string(key) + "\n" + std::to_string(version);
}

struct LoaderRegistry final {
  mutable std::shared_mutex mutex{};
  std::unordered_map<std::string, Loader> loaders{};
};

LoaderRegistry &loader_registry() {
  static LoaderRegistry registry;
  return registry;
}

std::string diff_payload(std::string_view variable, std::int64_t order,
                         DiffOptions options) {
  std::ostringstream out;
  out << std::quoted(std::string(variable)) << ' ' << order << ' '
      << static_cast<unsigned>(options.nonsmooth) << ' '
      << static_cast<unsigned>(options.fallback) << ' ' << options.max_nodes;
  return out.str();
}

std::tuple<std::string, std::int64_t, DiffOptions>
read_diff_payload(std::string_view payload) {
  std::istringstream input{std::string(payload)};
  std::string variable;
  std::int64_t order = 0;
  unsigned nonsmooth = 0;
  unsigned fallback = 0;
  std::size_t max_nodes = 0;
  input >> std::quoted(variable) >> order >> nonsmooth >> fallback >> max_nodes;
  PLUSS_CHECK(input && input.peek() == std::char_traits<char>::eof(),
              "math::io: malformed differential operator payload");
  PLUSS_CHECK(nonsmooth <= static_cast<unsigned>(math::NonSmooth::Autodiff) &&
                  fallback <= static_cast<unsigned>(math::Fallback::Autodiff),
              "math::io: invalid differential operator policy");
  return {std::move(variable), order,
          DiffOptions{.nonsmooth = static_cast<math::NonSmooth>(nonsmooth),
                      .fallback = static_cast<math::Fallback>(fallback),
                      .max_nodes = max_nodes}};
}

std::string variables_payload(const pluss::StackArray<std::string_view> &names,
                              DiffOptions options) {
  std::ostringstream out;
  out << names.size();
  for (const std::string_view name : names) {
    out << ' ' << std::quoted(std::string(name));
  }
  out << ' ' << static_cast<unsigned>(options.nonsmooth) << ' '
      << static_cast<unsigned>(options.fallback) << ' ' << options.max_nodes;
  return out.str();
}

std::pair<pluss::StackArray<std::string_view>, DiffOptions>
read_variables_payload(std::string_view payload) {
  std::istringstream input{std::string(payload)};
  std::size_t count = 0;
  input >> count;
  PLUSS_CHECK(count <= 100000, "math::io: unreasonable variable count");
  std::vector<std::string> names;
  names.reserve(count);
  for (std::size_t i = 0; i < count; ++i) {
    std::string name;
    input >> std::quoted(name);
    names.push_back(std::move(name));
  }
  unsigned nonsmooth = 0;
  unsigned fallback = 0;
  std::size_t max_nodes = 0;
  input >> nonsmooth >> fallback >> max_nodes;
  PLUSS_CHECK(input && input.peek() == std::char_traits<char>::eof(),
              "math::io: malformed multivariable operator payload");
  return {pluss::StackArray<std::string_view>(std::move(names)),
          DiffOptions{.nonsmooth = static_cast<math::NonSmooth>(nonsmooth),
                      .fallback = static_cast<math::Fallback>(fallback),
                      .max_nodes = max_nodes}};
}

std::string scalar_payload(std::string_view variable, double first,
                           std::optional<double> second = std::nullopt) {
  std::ostringstream out;
  out << std::setprecision(17) << std::quoted(std::string(variable)) << ' '
      << first << ' ' << second.has_value();
  if (second) {
    out << ' ' << *second;
  }
  return out.str();
}

std::tuple<std::string, double, std::optional<double>>
read_scalar_payload(std::string_view payload) {
  std::istringstream input{std::string(payload)};
  std::string variable;
  double first = 0.0;
  bool has_second = false;
  input >> std::quoted(variable) >> first >> has_second;
  std::optional<double> second;
  if (has_second) {
    double value = 0.0;
    input >> value;
    second = value;
  }
  PLUSS_CHECK(input && input.peek() == std::char_traits<char>::eof(),
              "math::io: malformed scalar operator payload");
  return {std::move(variable), first, second};
}

Capabilities structural_linear(bool adjoint_known = false) noexcept {
  return Capabilities{
      .structural = true,
      .evaluated_only = false,
      .linear = true,
      .adjoint_known = adjoint_known,
      .serializable = true,
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

Operator magnitude_differential(std::string_view variable, std::int64_t order,
                                DiffOptions options) {
  const Operator derivative = differential(variable, order, options);
  Capabilities capabilities = structural_linear();
  capabilities.linear = false;
  return detail::leaf(
      "magnitude_differential", capabilities,
      [derivative](const math::Function &input) {
        return derivative(input).abs();
      },
      detail::OperatorKind::Leaf, 1, diff_payload(variable, order, options));
}

Operator covariant(std::string_view variable, math::Function connection,
                   DiffOptions options) {
  PLUSS_CHECK(connection.defined(),
              "operators::covariant: connection must be defined");
  const Operator derivative = differential(variable, 1, options);
  Capabilities capabilities = structural_linear();
  capabilities.serializable = false;
  return detail::leaf(
      "covariant", capabilities,
      [derivative,
       connection = std::move(connection)](const math::Function &input) {
        PLUSS_CHECK(input.rank() == 0 && connection.rank() == 0,
                    "operators::covariant: the built-in connection contract "
                    "requires scalar-valued sections and coefficients");
        const math::Function coefficient =
            math::detail::FunctionAccess::rebind(connection, input);
        return derivative(input) + coefficient * input;
      },
      detail::OperatorKind::Leaf, 1);
}

Operator differential(std::string_view variable, std::int64_t order,
                      DiffOptions options) {
  PLUSS_CHECK(!variable.empty(),
              "operators::differential: variable must not be empty");
  PLUSS_CHECK(order > 0, "operators::differential: order must be positive");
  PLUSS_CHECK(options.max_nodes > 0,
              "operators::differential: max_nodes must be positive");
  // formal adjoint on fields that vanish at the ends: (d/dx)^k* = (-1)^k (d/dx)^k
  return detail::leaf(
      "differential", structural_linear(true),
      [variable = std::string(variable), order,
       options](const math::Function &input) {
        return math::detail::FunctionAccess::differentiate(
            input, variable, order, options.nonsmooth, options.fallback,
            options.max_nodes);
      },
      detail::OperatorKind::Leaf, 1, diff_payload(variable, order, options),
      [variable = std::string(variable), order, options]() {
        const Operator same = differential(variable, order, options);
        return order % 2 == 0 ? same : compose(scale(-1.0), same);
      });
}

Operator wirtinger(std::string_view variable, std::int64_t order,
                   DiffOptions options) {
  PLUSS_CHECK(!variable.empty(),
              "operators::wirtinger: variable must not be empty");
  PLUSS_CHECK(order > 0, "operators::wirtinger: order must be positive");
  PLUSS_CHECK(options.max_nodes > 0,
              "operators::wirtinger: max_nodes must be positive");
  PLUSS_CHECK(options.fallback == math::Fallback::Reject,
              "operators::wirtinger: autodiff fallback has no unambiguous "
              "Wirtinger contract");
  return detail::leaf(
      "wirtinger", structural_linear(),
      [variable = std::string(variable), order,
       options](const math::Function &input) {
        return math::detail::FunctionAccess::wirtinger(
            input, variable, order, false, options.nonsmooth,
            options.max_nodes);
      },
      detail::OperatorKind::Leaf, 1, diff_payload(variable, order, options));
}

Operator conjugate_wirtinger(std::string_view variable, std::int64_t order,
                             DiffOptions options) {
  PLUSS_CHECK(!variable.empty(),
              "operators::conjugate_wirtinger: variable must not be empty");
  PLUSS_CHECK(order > 0,
              "operators::conjugate_wirtinger: order must be positive");
  PLUSS_CHECK(options.max_nodes > 0,
              "operators::conjugate_wirtinger: max_nodes must be positive");
  PLUSS_CHECK(options.fallback == math::Fallback::Reject,
              "operators::conjugate_wirtinger: autodiff fallback has no "
              "unambiguous Wirtinger contract");
  return detail::leaf(
      "conjugate_wirtinger", structural_linear(),
      [variable = std::string(variable), order,
       options](const math::Function &input) {
        return math::detail::FunctionAccess::wirtinger(
            input, variable, order, true, options.nonsmooth,
            options.max_nodes);
      },
      detail::OperatorKind::Leaf, 1, diff_payload(variable, order, options));
}

Operator gradient(pluss::StackArray<std::string_view> variables,
                  DiffOptions options) {
  PLUSS_CHECK(options.max_nodes > 0,
              "operators::gradient: max_nodes must be positive");
  const std::string payload = variables_payload(variables, options);
  return detail::leaf(
      "gradient", structural_linear(),
      [variables = std::move(variables), options](const math::Function &input) {
        return math::detail::FunctionAccess::gradient(
            input, variables.lister().data(), options.nonsmooth,
            options.fallback, options.max_nodes);
      },
      detail::OperatorKind::Leaf, 1, payload);
}

Operator jacobian(pluss::StackArray<std::string_view> variables,
                  DiffOptions options) {
  PLUSS_CHECK(options.max_nodes > 0,
              "operators::jacobian: max_nodes must be positive");
  const std::string payload = variables_payload(variables, options);
  return detail::leaf(
      "jacobian", structural_linear(),
      [variables = std::move(variables), options](const math::Function &input) {
        return math::detail::FunctionAccess::jacobian(
            input, variables.lister().data(), options.nonsmooth,
            options.fallback, options.max_nodes);
      },
      detail::OperatorKind::Leaf, 1, payload);
}

Operator hessian(pluss::StackArray<std::string_view> variables,
                 DiffOptions options) {
  PLUSS_CHECK(options.max_nodes > 0,
              "operators::hessian: max_nodes must be positive");
  const std::string payload = variables_payload(variables, options);
  return detail::leaf(
      "hessian", structural_linear(),
      [variables = std::move(variables), options](const math::Function &input) {
        return math::detail::FunctionAccess::hessian(
            input, variables.lister().data(), options.nonsmooth,
            options.fallback, options.max_nodes);
      },
      detail::OperatorKind::Leaf, 1, payload);
}

Operator antiderivative(std::string_view variable) {
  PLUSS_CHECK(!variable.empty(),
              "operators::antiderivative: variable must not be empty");
  return detail::leaf(
      "antiderivative", structural_linear(),
      [variable = std::string(variable)](const math::Function &input) {
        return math::detail::FunctionAccess::antiderivative(input, variable);
      },
      detail::OperatorKind::Leaf, 1, scalar_payload(variable, 0.0));
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
      detail::OperatorKind::Leaf, 1, scalar_payload(variable, lower));
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
      detail::OperatorKind::Leaf, 1, scalar_payload(variable, lower, upper));
}

Operator shift(std::string_view variable, double amount) {
  PLUSS_CHECK(!variable.empty(),
              "operators::shift: variable must not be empty");
  return detail::leaf(
      "shift", structural_linear(true),
      [variable = std::string(variable), amount](const math::Function &input) {
        return math::detail::FunctionAccess::shift(input, variable, amount);
      },
      detail::OperatorKind::Leaf, 1, scalar_payload(variable, amount),
      [variable = std::string(variable), amount]() { return shift(variable, -amount); });
}

Operator multiply(math::Function coefficient) {
  PLUSS_CHECK(coefficient.defined(),
              "operators::multiply: coefficient must be defined");
  Capabilities capabilities = structural_linear(true);
  capabilities.serializable = false;
  return detail::leaf(
      "multiply", capabilities,
      [coefficient](const math::Function &input) {
        // the coefficient's variables are matched to the input's by name
        return math::detail::FunctionAccess::rebind(coefficient, input) * input;
      },
      detail::OperatorKind::Leaf, 1, {},
      [coefficient]() { return multiply(coefficient); });  // real coefficients: self-adjoint
}

Operator scale(double coefficient) {
  return detail::leaf(
      "scale", structural_linear(true),
      [coefficient](const math::Function &input) {
        return input * coefficient;
      },
      detail::OperatorKind::Leaf, 1, scalar_payload({}, coefficient),
      [coefficient]() { return scale(coefficient); });
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
  Capabilities stored_capabilities = capabilities;
  stored_capabilities.serializable = false;
  return detail::leaf(
      "linear_combination", stored_capabilities,
      [ops = std::move(owned_ops),
       coefficients =
           std::move(owned_coefficients)](const math::Function &input) {
        math::Function output = ops.front()(input) * coefficients.front();
        for (std::size_t i = 1; i < ops.size(); ++i) {
          output = output + ops[i](input) * coefficients[i];
        }
        return output;
      },
      detail::OperatorKind::Leaf, 1, {},
      [ops = std::vector<Operator>(ops.begin(), ops.end()),
       coefficients = std::vector<double>(coefficients.begin(), coefficients.end())]() {
        std::vector<Operator> adjoints;
        for (const Operator &op : ops) adjoints.push_back(adjoint(op));
        return linear_combination(pluss::RangeView<Operator>(adjoints), pluss::RangeView<double>(coefficients));
      });
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

// The formal adjoint with respect to the L2 inner product on fields that vanish at the ends of their
// domain: differential(k) -> (-1)^k differential(k), multiply and scale -> themselves (real
// coefficients), shift(a) -> shift(-a), a composition reverses, a linear combination combines the
// adjoints. Operators without a known adjoint throw.
Operator adjoint(const Operator &op) {
  const detail::OperatorConcept &descriptor = detail::OperatorAccess::get(op);
  if (descriptor.kind() == detail::OperatorKind::Identity) return op;
  if (descriptor.kind() == detail::OperatorKind::Composition) {
    const std::vector<Operator> &chain = detail::OperatorAccess::chain(op);
    Operator result = identity();
    for (const Operator &link : chain) result = compose(result, adjoint(link));  // (A B)* = B* A*
    return result;
  }
  const std::function<Operator()> *factory = descriptor.adjoint_factory();
  PLUSS_CHECK(factory != nullptr && descriptor.capabilities().adjoint_known,
              "operators::adjoint: operator '" + std::string(descriptor.key()) + "' has no known adjoint");
  return (*factory)();
}

bool bind(std::string_view key, std::uint32_t version, Loader loader) {
  PLUSS_CHECK(!key.empty(), "operators::bind: key must not be empty");
  PLUSS_CHECK(version > 0, "operators::bind: version must be positive");
  PLUSS_CHECK(loader, "operators::bind: loader must not be empty");
  LoaderRegistry &registry = loader_registry();
  std::unique_lock lock(registry.mutex);
  return registry.loaders
      .try_emplace(codec_key(key, version), std::move(loader))
      .second;
}

} // namespace pluss::subsp::math::operators

namespace pluss::subsp::math::operators::stochastic {
Operator restore_stochastic(std::string_view key, std::string_view payload);
} // namespace pluss::subsp::math::operators::stochastic
namespace pluss::subsp::math::operators::expansion {
Operator restore_expansion(std::string_view key, std::string_view payload);
} // namespace pluss::subsp::math::operators::expansion
namespace pluss::subsp::math::operators::fourier {
Operator restore_fourier(std::string_view payload);
} // namespace pluss::subsp::math::operators::fourier
namespace pluss::subsp::math::operators {
Operator restore_extension(std::string_view key, std::string_view payload);
namespace fractional {
Operator restore_fractional(std::string_view key, std::string_view payload);
} // namespace fractional
namespace measure {
Operator restore_measure(std::string_view key, std::string_view payload);
} // namespace measure
namespace stochastic {
Operator restore_malliavin(std::string_view key, std::string_view payload);
} // namespace stochastic
} // namespace pluss::subsp::math::operators
namespace pluss::subsp::math::operators::integral_transforms {
bool is_transform_key(std::string_view key);
Operator restore_transform(std::string_view key, std::string_view payload);
} // namespace pluss::subsp::math::operators::integral_transforms

namespace pluss::subsp::math::operators::detail {

Operator OperatorAccess::restore(std::string_view key, std::uint32_t version,
                                 std::string_view payload) {
  const auto require_builtin_version = [key, version] {
    PLUSS_CHECK(version == 1,
                "math::io: incompatible " + std::string(key) +
                    " operator descriptor version " +
                    std::to_string(version));
  };
  if (key == "identity") {
    require_builtin_version();
    PLUSS_CHECK(payload.empty(),
                "math::io: incompatible identity operator descriptor");
    return identity();
  }
  // "absolute_differential" at schema version 1 is the legacy name of the
  // magnitude operator; version 2 of that key is the geometric operator and is
  // not serializable.
  if (key == "differential" || key == "wirtinger" ||
      key == "conjugate_wirtinger" || key == "magnitude_differential" ||
      (key == "absolute_differential" && version == 1)) {
    require_builtin_version();
    auto [variable, order, options] = read_diff_payload(payload);
    if (key == "differential") return differential(variable, order, options);
    if (key == "wirtinger") return wirtinger(variable, order, options);
    if (key == "conjugate_wirtinger")
      return conjugate_wirtinger(variable, order, options);
    return magnitude_differential(variable, order, options);
  }
  if (key == "gradient" || key == "jacobian" || key == "hessian") {
    require_builtin_version();
    auto [variables, options] = read_variables_payload(payload);
    if (key == "gradient") {
      return gradient(std::move(variables), options);
    }
    if (key == "jacobian") {
      return jacobian(std::move(variables), options);
    }
    return hessian(std::move(variables), options);
  }
  if (key == "antiderivative" || key == "integral" || key == "shift") {
    require_builtin_version();
    auto [variable, first, second] = read_scalar_payload(payload);
    if (key == "antiderivative") {
      return antiderivative(variable);
    }
    if (key == "shift") {
      return shift(variable, first);
    }
    return second ? integral(variable, first, *second)
                  : integral(variable, first);
  }
  if (key == "scale") {
    require_builtin_version();
    auto [variable, coefficient, ignored] = read_scalar_payload(payload);
    PLUSS_CHECK(variable.empty() && !ignored,
                "math::io: malformed scale operator descriptor");
    return scale(coefficient);
  }

  if (key == "stochastic_integral" || key == "quadratic_variation") {
    require_builtin_version();
    return stochastic::restore_stochastic(key, payload);
  }
  if (key == "fractional_integral" || key == "fractional_differential") {
    require_builtin_version();
    return fractional::restore_fractional(key, payload);
  }
  if (key == "measure_lebesgue" || key == "measure_atomic") {
    require_builtin_version();
    return measure::restore_measure(key, payload);
  }
  if (key == "malliavin_derivative" || key == "skorokhod_integral") {
    require_builtin_version();
    return stochastic::restore_malliavin(key, payload);
  }
  if (key.starts_with("vector_divergence") || key == "vector_curl" || key == "vector_laplacian" ||
      key == "variational_functional_derivative") {
    require_builtin_version();
    return restore_extension(key, payload);
  }
  if (integral_transforms::is_transform_key(key)) {
    require_builtin_version();
    return integral_transforms::restore_transform(key, payload);
  }
  if (key.starts_with("fourier_") || key == "fourier_series" || key == "fourier_coefficients") {
    require_builtin_version();
    return fourier::restore_fourier(payload);
  }
  if (key == "taylor" || key == "taylor_coefficients" || key == "taylor_remainder" ||
      key == "power_series" || key == "series_derivative" || key == "series_integral") {
    require_builtin_version();
    return expansion::restore_expansion(key, payload);
  }

  Loader loader;
  {
    LoaderRegistry &registry = loader_registry();
    std::shared_lock lock(registry.mutex);
    const auto found = registry.loaders.find(codec_key(key, version));
    PLUSS_CHECK(found != registry.loaders.end(),
                "math::io: no operator loader is bound for key '" +
                    std::string(key) + "' version " + std::to_string(version));
    loader = found->second;
  }
  Operator restored = loader(payload, version);
  PLUSS_CHECK(restored.defined(),
              "math::io: custom operator loader returned undefined");
  return restored;
}

} // namespace pluss::subsp::math::operators::detail

namespace pluss::subsp::math::operators::geometry {
namespace {

using math::Function;

std::int64_t dimension_of(const Function &connection, const char *who) {
  PLUSS_CHECK(connection.defined() && connection.rank() == 3 &&
                  connection.shape()[0] == connection.shape()[1] &&
                  connection.shape()[1] == connection.shape()[2],
              std::string(who) +
                  ": the connection must have output shape [n, n, n]");
  return connection.shape()[0];
}

// move output axis `from` of `value` to position `to` by adjacent swaps
Function move_axis(Function value, std::int64_t from, std::int64_t to) {
  while (from > to) {
    value = value.transpose(from - 1, from);
    --from;
  }
  while (from < to) {
    value = value.transpose(from, from + 1);
    ++from;
  }
  return value;
}

// Adds the connection terms of `field` for the connection slice N^i_m (rank 2):
//   + N^i_m V^{..m..} on each contravariant axis, - N^m_j V_{..m..} on each covariant axis.
Function connection_terms(const Function &field, const Function &slice,
                          TensorSignature signature, const char *who) {
  const std::int64_t n = slice.shape()[0];
  PLUSS_CHECK(signature.contravariant >= 0 && signature.covariant >= 0,
              std::string(who) + ": tensor signature must be non-negative");
  PLUSS_CHECK(field.rank() == signature.contravariant + signature.covariant,
              std::string(who) +
                  ": the tensor's output rank must equal its signature");
  for (std::int64_t axis = 0; axis < field.rank(); ++axis) {
    PLUSS_CHECK(field.shape()[static_cast<std::size_t>(axis)] == n,
                std::string(who) +
                    ": every tensor index must range over the manifold "
                    "dimension");
  }
  Function sum;
  const auto accumulate = [&sum](Function term, bool positive) {
    if (!sum.defined()) {
      sum = positive ? std::move(term) : -term;
    } else {
      sum = positive ? sum + term : sum - term;
    }
  };
  for (std::int64_t axis = 0; axis < signature.contravariant; ++axis) {
    accumulate(move_axis(slice.contract(field, {1}, {axis}), 0, axis), true);
  }
  for (std::int64_t k = 0; k < signature.covariant; ++k) {
    const std::int64_t axis = signature.contravariant + k;
    accumulate(move_axis(slice.contract(field, {0}, {axis}), 0, axis), false);
  }
  return sum;
}

} // namespace

Function levi_civita(const Function &metric,
                     pluss::StackArray<std::string_view> coordinates,
                     DiffOptions options) {
  PLUSS_CHECK(metric.defined() && metric.rank() == 2 &&
                  metric.shape()[0] == metric.shape()[1],
              "geometry::levi_civita: the metric must be a square matrix "
              "Function");
  const std::int64_t n = metric.shape()[0];
  std::vector<std::string> names;
  const auto owned = std::move(coordinates).lister();
  if (owned.size() == 0) {
    for (const std::string_view variable : metric.variables()) {
      names.emplace_back(variable);
    }
  } else {
    for (const std::string_view variable : owned) {
      names.emplace_back(variable);
    }
  }
  PLUSS_CHECK(static_cast<std::int64_t>(names.size()) == n,
              "geometry::levi_civita: one coordinate per metric dimension is "
              "required");
  std::vector<Function> derivatives;
  derivatives.reserve(names.size());
  for (const std::string &name : names) {
    derivatives.push_back(differential(name, 1, options)(metric));
  }
  // D[l][k][j] = d_j g_lk (derivative axis trailing, as jacobian places it)
  const Function d = math::stack(pluss::RangeView<Function>(derivatives));
  const Function bracket =
      d.transpose(1, 2) + d - d.transpose(0, 2).transpose(1, 2);
  return metric.inverse().contract(bracket, {1}, {0}) * 0.5;
}

Operator absolute_differential(Function curve, Function connection,
                               std::string_view parameter,
                               TensorSignature signature,
                               DiffOptions options) {
  const std::int64_t n = dimension_of(connection, "geometry::absolute_differential");
  PLUSS_CHECK(curve.defined() && curve.rank() == 1 && curve.shape()[0] == n,
              "geometry::absolute_differential: the curve must be a rank-1 "
              "Function with one component per coordinate");
  PLUSS_CHECK(!parameter.empty(),
              "geometry::absolute_differential: parameter must not be empty");
  const Operator derivative = differential(parameter, 1, options);
  Capabilities capabilities = structural_linear();
  capabilities.serializable = false;
  return detail::leaf(
      "absolute_differential", capabilities,
      [=, curve = std::move(curve), connection = std::move(connection),
       parameter = std::string(parameter)](const Function &input) {
        const Function along =
            math::detail::FunctionAccess::rebind(curve, input);
        std::vector<Function> components;
        components.reserve(static_cast<std::size_t>(n));
        for (std::int64_t k = 0; k < n; ++k) {
          components.push_back(along.select(0, k));
        }
        const Function gamma =
            connection.compose(pluss::RangeView<Function>(components));
        const Function velocity = derivative(along);
        const Function slice = gamma.contract(velocity, {2}, {0}); // Gamma^i_{mk} xdot^k
        const Function terms = connection_terms(input, slice, signature,
                                                "geometry::absolute_differential");
        return terms.defined() ? derivative(input) + terms : derivative(input);
      },
      detail::OperatorKind::Leaf, 2);
}

Operator covariant(std::string_view coordinate, Function connection,
                   TensorSignature signature, DiffOptions options) {
  const std::int64_t n = dimension_of(connection, "geometry::covariant");
  std::int64_t direction = -1;
  const auto variables = connection.variables();
  for (std::size_t i = 0; i < variables.size(); ++i) {
    if (variables[i] == coordinate) {
      direction = static_cast<std::int64_t>(i);
    }
  }
  PLUSS_CHECK(direction >= 0 && direction < n,
              "geometry::covariant: the coordinate must be one of the "
              "connection's variables");
  const Operator derivative = differential(coordinate, 1, options);
  Capabilities capabilities = structural_linear();
  capabilities.serializable = false;
  return detail::leaf(
      "covariant_tensor", capabilities,
      [=, connection = std::move(connection)](const Function &input) {
        const Function gamma =
            math::detail::FunctionAccess::rebind(connection, input);
        const Function slice = gamma.select(2, direction); // Gamma^i_{m,direction}
        const Function terms =
            connection_terms(input, slice, signature, "geometry::covariant");
        return terms.defined() ? derivative(input) + terms : derivative(input);
      },
      detail::OperatorKind::Leaf, 1);
}

} // namespace pluss::subsp::math::operators::geometry

namespace pluss::subsp::math::operators::stochastic {
namespace {

using math::Function;
using detail::number;
using detail::pack;
using detail::real;
using detail::serializable;
using detail::unpack;

Capabilities structural_path() {
  Capabilities capabilities = structural_linear();
  capabilities.serializable = false;
  capabilities.path_functional = true;
  return capabilities;
}

std::vector<std::string> own_names(pluss::StackArray<std::string_view> states) {
  std::vector<std::string> names;
  for (const std::string_view state : std::move(states).lister()) {
    names.emplace_back(state);
  }
  PLUSS_CHECK(!names.empty(), "operators::stochastic: at least one state "
                              "variable is required");
  return names;
}

std::vector<std::string_view> views(const std::vector<std::string> &names) {
  return {names.begin(), names.end()};
}

struct Coefficients {
  std::int64_t states = 1;
  std::int64_t noises = 1;
  bool scalar = true;
};

Coefficients check_coefficients(const Function &drift, const Function &diffusion,
                                std::size_t state_count, const char *who) {
  PLUSS_CHECK(drift.defined() && diffusion.defined(),
              std::string(who) + ": drift and diffusion must be defined");
  Coefficients c;
  if (drift.rank() == 0 && diffusion.rank() == 0) {
    PLUSS_CHECK(state_count == 1,
                std::string(who) + ": scalar coefficients describe exactly one "
                                   "state variable");
    return c;
  }
  PLUSS_CHECK(drift.rank() == 1 && diffusion.rank() == 2 &&
                  drift.shape()[0] == static_cast<std::int64_t>(state_count) &&
                  diffusion.shape()[0] == drift.shape()[0],
              std::string(who) + ": use scalar coefficients, or a drift vector "
                                 "[n] with a diffusion matrix [n, m]");
  c.scalar = false;
  c.states = drift.shape()[0];
  c.noises = diffusion.shape()[1];
  return c;
}

void check_theta(double theta, const char *who) {
  PLUSS_CHECK(theta >= 0.0 && theta <= 1.0,
              std::string(who) + ": theta must lie in [0, 1]");
}

// tr(sigma sigma^T H) for a scalar g
Function noise_curvature(const Function &sigma, const Function &g,
                         const std::vector<std::string> &names,
                         const Coefficients &c, const DiffOptions &options) {
  if (c.scalar) {
    return sigma * sigma * differential(names[0], 2, options)(g);
  }
  const Function hessian_g = hessian(pluss::StackArray<std::string_view>(views(names)), options)(g);
  return sigma.contract(sigma, {1}, {1}).contract(hessian_g, {0, 1}, {0, 1});
}

Function drift_term(const Function &mu, const Function &g,
                    const std::vector<std::string> &names,
                    const Coefficients &c, const DiffOptions &options) {
  if (c.scalar) {
    return mu * differential(names[0], 1, options)(g);
  }
  const Function grad = gradient(pluss::StackArray<std::string_view>(views(names)), options)(g);
  return mu.contract(grad, {0}, {0});
}

} // namespace

Operator stochastic_integral(std::string_view noise, IntegralOptions options) {
  PLUSS_CHECK(!noise.empty(),
              "operators::stochastic::stochastic_integral: noise variable "
              "must not be empty");
  const double theta = theta_of(options.convention, options.theta);
  check_theta(theta, "operators::stochastic::stochastic_integral");
  return detail::leaf(
      "stochastic_integral", serializable(structural_path()),
      [noise = std::string(noise), theta, options](const Function &input) {
        PLUSS_CHECK(input.rank() == 0,
                    "operators::stochastic::stochastic_integral: the "
                    "integrand must be scalar-valued");
        PLUSS_CHECK(!math::detail::FunctionAccess::contains_opaque(input),
                    "operators::stochastic::stochastic_integral: an opaque "
                    "callable can look ahead along the path, so it is not a "
                    "non-anticipating integrand; build the integrand from "
                    "Function operations, or use sde::ito_integral with its "
                    "adaptedness probe");
        const Function path = input.arg(noise);
        return math::detail::FunctionAccess::tensor(
            math::detail::PrimitiveId::PathIntegral, {input, path},
            {std::bit_cast<std::int64_t>(theta),
             options.cumulative ? 1 : 0, options.path_axis});
      },
      detail::OperatorKind::Leaf, 1,
      pack({std::string(noise), number(theta), std::to_string(options.cumulative ? 1 : 0),
            std::to_string(options.path_axis)}));
}

Operator quadratic_variation(PathOptions options) {
  return detail::leaf(
      "quadratic_variation", [] { Capabilities c = serializable(structural_path()); c.linear = false; return c; }(),
      [options](const Function &input) {
        PLUSS_CHECK(input.rank() == 0,
                    "operators::stochastic::quadratic_variation: the "
                    "process must be scalar-valued");
        return math::detail::FunctionAccess::tensor(
            math::detail::PrimitiveId::PathCovariation, {input, input},
            {options.cumulative ? 1 : 0, options.path_axis});
      },
      detail::OperatorKind::Leaf, 1,
      pack({std::to_string(options.cumulative ? 1 : 0), std::to_string(options.path_axis)}));
}

Operator covariation(Function other, PathOptions options) {
  PLUSS_CHECK(other.defined() && other.rank() == 0,
              "operators::stochastic::covariation: the second process must "
              "be a scalar-valued Function");
  return detail::leaf(
      "covariation", structural_path(),
      [options, other = std::move(other)](const Function &input) {
        PLUSS_CHECK(input.rank() == 0,
                    "operators::stochastic::covariation: the process must be "
                    "scalar-valued");
        const Function partner =
            math::detail::FunctionAccess::rebind(other, input);
        return math::detail::FunctionAccess::tensor(
            math::detail::PrimitiveId::PathCovariation, {input, partner},
            {options.cumulative ? 1 : 0, options.path_axis});
      },
      detail::OperatorKind::Leaf, 1);
}

Operator generator(Function drift, Function diffusion,
                   pluss::StackArray<std::string_view> states,
                   DiffOptions options) {
  const std::vector<std::string> names = own_names(std::move(states));
  const Coefficients c = check_coefficients(drift, diffusion, names.size(),
                                            "operators::stochastic::generator");
  Capabilities capabilities = structural_linear();
  capabilities.serializable = false;
  return detail::leaf(
      "generator", capabilities,
      [=, drift = std::move(drift), diffusion = std::move(diffusion)](const Function &g) {
        PLUSS_CHECK(g.rank() == 0, "operators::stochastic::generator: the "
                                   "test function must be scalar-valued");
        const Function mu = math::detail::FunctionAccess::rebind(drift, g);
        const Function sigma = math::detail::FunctionAccess::rebind(diffusion, g);
        return drift_term(mu, g, names, c, options) +
               noise_curvature(sigma, g, names, c, options) * 0.5;
      },
      detail::OperatorKind::Leaf, 1);
}

Operator stochastic_differential(Function drift, Function diffusion,
                                 pluss::StackArray<std::string_view> states,
                                 Convention convention, double theta,
                                 DiffOptions options) {
  const std::vector<std::string> names = own_names(std::move(states));
  const Coefficients c = check_coefficients(
      drift, diffusion, names.size(),
      "operators::stochastic::stochastic_differential");
  const double rule = theta_of(convention, theta);
  check_theta(rule, "operators::stochastic::stochastic_differential");
  Capabilities capabilities = structural_linear();
  capabilities.serializable = false;
  return detail::leaf(
      "stochastic_differential", capabilities,
      [=, drift = std::move(drift), diffusion = std::move(diffusion)](const Function &g) {
        PLUSS_CHECK(g.rank() == 0,
                    "operators::stochastic::stochastic_differential: the "
                    "function of the state must be scalar-valued");
        const Function mu = math::detail::FunctionAccess::rebind(drift, g);
        const Function sigma = math::detail::FunctionAccess::rebind(diffusion, g);
        const Function a = drift_term(mu, g, names, c, options) +
                           noise_curvature(sigma, g, names, c, options) * (0.5 - rule);
        if (c.scalar) {
          const Function b = differential(names[0], 1, options)(g) * sigma;
          return math::stack({a, b});
        }
        const Function grad = gradient(pluss::StackArray<std::string_view>(views(names)), options)(g);
        const Function b = sigma.contract(grad, {0}, {0}); // diffusion^T grad g, length m
        std::vector<Function> components{a};
        for (std::int64_t k = 0; k < c.noises; ++k) {
          components.push_back(b.select(0, k));
        }
        return math::stack(pluss::RangeView<Function>(components));
      },
      detail::OperatorKind::Leaf, 1);
}

Operator convention_change(double theta_from, double theta_to, Function diffusion,
                           pluss::StackArray<std::string_view> states,
                           DiffOptions options) {
  const std::vector<std::string> names = own_names(std::move(states));
  check_theta(theta_from, "operators::stochastic::convention_change");
  check_theta(theta_to, "operators::stochastic::convention_change");
  PLUSS_CHECK(diffusion.defined() &&
                  ((diffusion.rank() == 0 && names.size() == 1) ||
                   (diffusion.rank() == 2 &&
                    diffusion.shape()[0] == static_cast<std::int64_t>(names.size()))),
              "operators::stochastic::convention_change: diffusion must be "
              "scalar for one state or an [n, m] matrix");
  Capabilities capabilities = structural_linear();
  capabilities.serializable = false;
  return detail::leaf(
      "convention_change", capabilities,
      [=, diffusion = std::move(diffusion)](const Function &drift) {
        const Function sigma = math::detail::FunctionAccess::rebind(diffusion, drift);
        Function correction;
        if (diffusion.rank() == 0) {
          PLUSS_CHECK(drift.rank() == 0,
                      "operators::stochastic::convention_change: scalar "
                      "diffusion needs a scalar drift");
          correction = sigma * differential(names[0], 1, options)(sigma);
        } else {
          PLUSS_CHECK(drift.rank() == 1 &&
                          drift.shape()[0] == static_cast<std::int64_t>(names.size()),
                      "operators::stochastic::convention_change: the drift "
                      "must be a vector with one entry per state");
          std::vector<Function> columns;
          for (const std::string &name : names) {
            columns.push_back(differential(name, 1, options)(sigma)); // d_j sigma [n, m]
          }
          const Function d_sigma = math::stack(pluss::RangeView<Function>(columns)); // [i, k, j]
          correction = d_sigma.contract(sigma, {1, 2}, {1, 0});            // c_i
        }
        return drift + correction * (theta_from - theta_to);
      },
      detail::OperatorKind::Leaf, 1);
}

} // namespace pluss::subsp::math::operators::stochastic


namespace pluss::subsp::math::operators::stochastic {

Operator restore_stochastic(std::string_view key, std::string_view payload) {
  const auto count = [&](std::size_t n) { return unpack(payload, n); };
  if (key == "stochastic_integral") {
    const auto f = count(4);
    IntegralOptions options;
    options.convention = Convention::Theta;
    options.theta = real(f[1]);
    options.cumulative = f[2] == "1";
    options.path_axis = std::stoll(f[3]);
    return stochastic_integral(f[0], options);
  }
  if (key == "quadratic_variation") {
    const auto f = count(2);
    return quadratic_variation({.cumulative = f[0] == "1", .path_axis = std::stoll(f[1])});
  }
  PLUSS_FAIL("math::io: unknown stochastic operator key '" + std::string(key) + "'");
}

} // namespace pluss::subsp::math::operators::stochastic

namespace pluss::subsp::math::operators::expansion {
namespace {

using math::Function;
using detail::number;
using detail::pack_all;
using detail::real;
using detail::unpack_all;

void check_expansion(std::int64_t order, const char *who) {
  PLUSS_CHECK(order >= 0 && order <= 64,
              std::string(who) + ": order must lie in [0, 64]");
}
void check_center(double center, const char *who) {
  PLUSS_CHECK(std::isfinite(center),
              std::string(who) + ": the centre must be finite");
}

// f with the named variables replaced by constants, the others kept
Function at_center(const Function &f, const std::vector<std::string> &names,
                   const std::vector<double> &centers) {
  // fix each expanded variable at its centre on the same trace, so the result keeps the variable (bound)
  // and stays a Function of the others
  Function result = f;
  for (std::size_t i = 0; i < names.size(); ++i) {
    result = math::detail::FunctionAccess::fix(result, names[i], centers[i]);
  }
  return result;
}

Function derivative_of(const Function &f, const std::vector<std::string> &names,
                       const std::vector<std::int64_t> &orders,
                       const DiffOptions &options) {
  Function g = f;
  for (std::size_t i = 0; i < names.size(); ++i) {
    if (orders[i] > 0) {
      g = differential(names[i], orders[i], options)(g);
    }
  }
  return g;
}

// all multi-indices with |alpha| <= order
void multi_indices(std::size_t variables, std::int64_t order,
                   std::vector<std::vector<std::int64_t>> &out,
                   std::vector<std::int64_t> &current, std::int64_t left) {
  if (current.size() == variables) {
    out.push_back(current);
    return;
  }
  for (std::int64_t a = 0; a <= left; ++a) {
    current.push_back(a);
    multi_indices(variables, order, out, current, left - a);
    current.pop_back();
  }
}

double factorial(std::int64_t n) {
  double value = 1.0;
  for (std::int64_t k = 2; k <= n; ++k) value *= static_cast<double>(k);
  return value;
}

Function taylor_polynomial(const Function &f, const std::vector<std::string> &names,
                           const std::vector<double> &centers, std::int64_t order,
                           const DiffOptions &options) {
  std::vector<std::vector<std::int64_t>> indices;
  std::vector<std::int64_t> current;
  multi_indices(names.size(), order, indices, current, order);
  Function sum;
  for (const auto &alpha : indices) {
    Function term = at_center(derivative_of(f, names, alpha, options), names, centers);
    double scale = 1.0;
    for (std::size_t i = 0; i < alpha.size(); ++i) {
      scale *= factorial(alpha[i]);
      if (alpha[i] > 0) {
        term = term * (f.arg(names[i]) - centers[i]).pow(static_cast<double>(alpha[i]));
      }
    }
    term = term * (1.0 / scale);
    sum = sum.defined() ? sum + term : term;
  }
  return sum;
}

void diff_fields(const DiffOptions &o, std::vector<std::string> &fields) {
  fields.push_back(std::to_string(static_cast<int>(o.nonsmooth)));
  fields.push_back(std::to_string(static_cast<int>(o.fallback)));
  fields.push_back(std::to_string(o.max_nodes));
}
DiffOptions read_diff(const std::vector<std::string> &f, std::size_t at) {
  DiffOptions o;
  o.nonsmooth = static_cast<math::NonSmooth>(std::stoi(f[at]));
  o.fallback = static_cast<math::Fallback>(std::stoi(f[at + 1]));
  o.max_nodes = static_cast<std::size_t>(std::stoull(f[at + 2]));
  return o;
}

std::string name_field(std::string_view name) { return std::string(name); }

Capabilities expansion_capabilities() {
  Capabilities c = structural_linear();
  c.serializable = true;
  return c;
}

} // namespace

bool use_jet(Engine engine, const Function &f, std::int64_t order, const char *who) {
  const bool supported = math::detail::FunctionAccess::supports_power_series(f);
  if (engine == Engine::Jet) {
    PLUSS_CHECK(supported, std::string(who) + ": the power-series engine supports scalar elementwise Functions only");
    return true;
  }
  return engine == Engine::Auto && order > 8 && supported;
}

Operator taylor(std::string_view variable, double center, std::int64_t order,
                DiffOptions options, Engine engine) {
  check_expansion(order, "operators::expansion::taylor");
  check_center(center, "operators::expansion::taylor");
  std::vector<std::string> fields{"1", std::string(variable), number(center), std::to_string(order)};
  diff_fields(options, fields);
  fields.push_back(std::to_string(static_cast<int>(engine)));
  return detail::leaf(
      "taylor", expansion_capabilities(),
      [name = std::string(variable), center, order, options, engine](const Function &f) {
        if (use_jet(engine, f, order, "operators::expansion::taylor")) {
          const Function coefficients = math::detail::FunctionAccess::power_series_coefficients(f, name, center, order);
          return power_series(name, center)(coefficients);
        }
        return taylor_polynomial(f, {name}, {center}, order, options);
      },
      detail::OperatorKind::Leaf, 1, pack_all(fields));
}

Operator taylor(pluss::StackArray<std::string_view> variables,
                pluss::RangeView<double> center, std::int64_t order,
                DiffOptions options) {
  check_expansion(order, "operators::expansion::taylor");
  std::vector<std::string> names;
  for (const std::string_view variable : std::move(variables).lister()) names.emplace_back(variable);
  PLUSS_CHECK(!names.empty() && names.size() == center.size(),
              "operators::expansion::taylor: one centre per variable is required");
  std::vector<double> centers(center.begin(), center.end());
  for (const double c : centers) check_center(c, "operators::expansion::taylor");
  std::vector<std::string> fields{std::to_string(names.size())};
  for (const std::string &name : names) fields.push_back(name);
  for (const double c : centers) fields.push_back(number(c));
  fields.push_back(std::to_string(order));
  diff_fields(options, fields);
  fields.push_back(std::to_string(static_cast<int>(Engine::Structural)));
  return detail::leaf(
      "taylor", expansion_capabilities(),
      [names, centers, order, options](const Function &f) {
        return taylor_polynomial(f, names, centers, order, options);
      },
      detail::OperatorKind::Leaf, 1, pack_all(fields));
}

Operator taylor_coefficients(std::string_view variable, double center,
                             std::int64_t order, DiffOptions options, Engine engine) {
  check_expansion(order, "operators::expansion::taylor_coefficients");
  check_center(center, "operators::expansion::taylor_coefficients");
  std::vector<std::string> fields{"1", std::string(variable), number(center), std::to_string(order)};
  diff_fields(options, fields);
  fields.push_back(std::to_string(static_cast<int>(engine)));
  return detail::leaf(
      "taylor_coefficients", expansion_capabilities(),
      [name = std::string(variable), center, order, options, engine](const Function &f) {
        if (use_jet(engine, f, order, "operators::expansion::taylor_coefficients")) {
          return math::detail::FunctionAccess::power_series_coefficients(f, name, center, order);
        }
        std::vector<Function> coefficients;
        for (std::int64_t k = 0; k <= order; ++k) {
          coefficients.push_back(at_center(derivative_of(f, {name}, {k}, options), {name}, {center}) * (1.0 / factorial(k)));
        }
        return math::stack(pluss::RangeView<Function>(coefficients));
      },
      detail::OperatorKind::Leaf, 1, pack_all(fields));
}

Operator taylor_remainder(std::string_view variable, std::int64_t order,
                          DiffOptions options) {
  check_expansion(order, "operators::expansion::taylor_remainder");
  std::vector<std::string> fields{"1", std::string(variable), number(0.0), std::to_string(order)};
  diff_fields(options, fields);
  fields.push_back("1");
  return detail::leaf(
      "taylor_remainder", expansion_capabilities(),
      [name = std::string(variable), order, options](const Function &f) {
        return differential(name, order + 1, options)(f) * (1.0 / factorial(order + 1));
      },
      detail::OperatorKind::Leaf, 1, pack_all(fields));
}

Operator power_series(std::string_view variable, double center) {
  check_center(center, "operators::expansion::power_series");
  return detail::leaf(
      "power_series", expansion_capabilities(),
      [name = std::string(variable), center](const Function &c) {
        PLUSS_CHECK(c.rank() == 1,
                    "operators::expansion::power_series: the input must be a "
                    "coefficient vector (a rank-1 Function)");
        const Function x = c.arg(name);
        Function sum;
        for (std::int64_t k = 0; k < c.shape()[0]; ++k) {
          Function term = c.select(0, k);
          if (k > 0) term = term * (x - center).pow(static_cast<double>(k));
          sum = sum.defined() ? sum + term : term;
        }
        return sum;
      },
      detail::OperatorKind::Leaf, 1, pack_all({name_field(variable), number(center)}));
}

Operator series_derivative() {
  return detail::leaf(
      "series_derivative", expansion_capabilities(),
      [](const Function &c) {
        PLUSS_CHECK(c.rank() == 1 && c.shape()[0] >= 2,
                    "operators::expansion::series_derivative: needs a "
                    "coefficient vector with at least two entries");
        std::vector<Function> out;
        for (std::int64_t k = 0; k + 1 < c.shape()[0]; ++k) out.push_back(c.select(0, k + 1) * static_cast<double>(k + 1));
        return math::stack(pluss::RangeView<Function>(out));
      },
      detail::OperatorKind::Leaf, 1, std::string{});
}

Operator series_integral() {
  return detail::leaf(
      "series_integral", expansion_capabilities(),
      [](const Function &c) {
        PLUSS_CHECK(c.rank() == 1, "operators::expansion::series_integral: "
                                   "needs a coefficient vector");
        std::vector<Function> out{c.select(0, 0) * 0.0};
        for (std::int64_t k = 0; k < c.shape()[0]; ++k) out.push_back(c.select(0, k) * (1.0 / static_cast<double>(k + 1)));
        return math::stack(pluss::RangeView<Function>(out));
      },
      detail::OperatorKind::Leaf, 1, std::string{});
}

Operator series_product(Function other) {
  PLUSS_CHECK(other.defined() && other.rank() == 1,
              "operators::expansion::series_product: the second factor must "
              "be a coefficient vector");
  Capabilities capabilities = structural_linear();
  capabilities.serializable = false;
  return detail::leaf(
      "series_product", capabilities,
      [other = std::move(other)](const Function &a) {
        PLUSS_CHECK(a.rank() == 1, "operators::expansion::series_product: "
                                   "the input must be a coefficient vector");
        const Function b = math::detail::FunctionAccess::rebind(other, a);
        const std::int64_t n = std::min(a.shape()[0], b.shape()[0]);
        std::vector<Function> out;
        for (std::int64_t k = 0; k < n; ++k) {
          Function sum;
          for (std::int64_t i = 0; i <= k; ++i) {
            const Function term = a.select(0, i) * b.select(0, k - i);
            sum = sum.defined() ? sum + term : term;
          }
          out.push_back(sum);
        }
        return math::stack(pluss::RangeView<Function>(out));
      },
      detail::OperatorKind::Leaf, 1);
}

Operator restore_expansion(std::string_view key, std::string_view payload) {
  if (key == "power_series") {
    const auto f = unpack_all(payload);
    PLUSS_CHECK(f.size() == 2, "math::io: malformed expansion operator descriptor");
    return power_series(f[0], real(f[1]));
  }
  if (key == "series_derivative") return series_derivative();
  if (key == "series_integral") return series_integral();
  const std::vector<std::string> f = unpack_all(payload);
  PLUSS_CHECK(!f.empty(), "math::io: malformed expansion operator descriptor");
  const std::size_t n = static_cast<std::size_t>(std::stoull(f[0]));
  PLUSS_CHECK(f.size() == 2 * n + 6,
              "math::io: malformed expansion operator descriptor");
  if (key == "taylor") {
    std::vector<std::string> names(f.begin() + 1, f.begin() + 1 + static_cast<std::ptrdiff_t>(n));
    std::vector<double> centers;
    for (std::size_t i = 0; i < n; ++i) centers.push_back(real(f[1 + n + i]));
    const std::int64_t order = std::stoll(f[1 + 2 * n]);
    const DiffOptions options = read_diff(f, 2 + 2 * n);
    const Engine engine = static_cast<Engine>(std::stoi(f[5 + 2 * n]));
    if (n == 1) return taylor(names[0], centers[0], order, options, engine);
    std::vector<std::string_view> views(names.begin(), names.end());
    return taylor(pluss::StackArray<std::string_view>(std::move(views)), pluss::RangeView<double>(centers), order, options);
  }
  const double center = real(f[2]);
  const std::int64_t order = std::stoll(f[3]);
  const DiffOptions options = read_diff(f, 4);
  if (key == "taylor_coefficients") return taylor_coefficients(f[1], center, order, options, static_cast<Engine>(std::stoi(f[7])));
  return taylor_remainder(f[1], order, options);
}

} // namespace pluss::subsp::math::operators::expansion

namespace pluss::subsp::math::operators::fourier {
namespace {

using math::Function;
using detail::number;
using detail::pack_all;
using detail::real;
using detail::unpack_all;

constexpr double kPi = 3.14159265358979323846;

enum class Kind : int { CosineCoefficients, SineCoefficients, CosineSeries, SineSeries, FullCoefficients, FullSeries };

struct Spec {
  Kind kind = Kind::CosineCoefficients;
  std::string variable;
  double lower = 0.0, upper = 1.0;
  std::int64_t terms = 1;
  double from = 0.0, to = 1.0;
};

void check(const Spec &s, const char *who) {
  PLUSS_CHECK(!s.variable.empty(), std::string(who) + ": variable must not be empty");
  PLUSS_CHECK(std::isfinite(s.lower) && std::isfinite(s.upper) && s.upper > s.lower,
              std::string(who) + ": the interval must be finite with lower < upper");
  PLUSS_CHECK(s.from >= s.lower && s.to <= s.upper && s.to > s.from,
              std::string(who) + ": the support must be a non-empty sub-interval");
  PLUSS_CHECK(s.terms >= 1 && s.terms <= 4096, std::string(who) + ": terms must lie in [1, 4096]");
}

// 2/(b-a) * integral over the support of f * trig(u (x - anchor)); `scale` is 2/(b-a) or 1/L
Function integral_term(const Function &f, const Spec &s, double anchor, double u, bool cosine, double scale) {
  const Function x = f.arg(s.variable);
  const Function phase = (x - anchor) * u;
  const Function integrand = f * (cosine ? phase.cos() : phase.sin());
  return integral(s.variable, s.from, s.to)(integrand) * scale;
}

std::vector<Function> half_range_coefficients(const Function &f, const Spec &s, bool cosine) {
  const double width = s.upper - s.lower;
  std::vector<Function> out;
  const std::int64_t first = cosine ? 0 : 1, last = cosine ? s.terms - 1 : s.terms;
  for (std::int64_t k = first; k <= last; ++k) {
    out.push_back(integral_term(f, s, s.lower, static_cast<double>(k) * kPi / width, cosine, 2.0 / width));
  }
  return out;
}

std::vector<Function> full_coefficients(const Function &f, const Spec &s) {
  const double half = s.upper; // [-L, L]
  std::vector<Function> out;
  for (std::int64_t k = 0; k < s.terms; ++k) {
    out.push_back(integral_term(f, s, 0.0, static_cast<double>(k) * kPi / half, true, 1.0 / half));
  }
  for (std::int64_t k = 1; k < s.terms; ++k) {
    out.push_back(integral_term(f, s, 0.0, static_cast<double>(k) * kPi / half, false, 1.0 / half));
  }
  return out;
}

Function apply_spec(const Function &f, const Spec &s) {
  PLUSS_CHECK(f.defined() && f.rank() == 0, "operators::fourier: the function must be scalar-valued");
  const Function x = f.arg(s.variable);
  switch (s.kind) {
  case Kind::CosineCoefficients:
    return math::stack(pluss::RangeView<Function>(half_range_coefficients(f, s, true)));
  case Kind::SineCoefficients:
    return math::stack(pluss::RangeView<Function>(half_range_coefficients(f, s, false)));
  case Kind::FullCoefficients:
    return math::stack(pluss::RangeView<Function>(full_coefficients(f, s)));
  case Kind::CosineSeries: {
    const double width = s.upper - s.lower;
    const std::vector<Function> a = half_range_coefficients(f, s, true);
    Function sum = a[0] * 0.5;
    for (std::size_t k = 1; k < a.size(); ++k) {
      sum = sum + a[k] * ((x - s.lower) * (static_cast<double>(k) * kPi / width)).cos();
    }
    return sum;
  }
  case Kind::SineSeries: {
    const double width = s.upper - s.lower;
    const std::vector<Function> b = half_range_coefficients(f, s, false);
    Function sum;
    for (std::size_t i = 0; i < b.size(); ++i) {
      const Function term = b[i] * ((x - s.lower) * (static_cast<double>(i + 1) * kPi / width)).sin();
      sum = sum.defined() ? sum + term : term;
    }
    return sum;
  }
  case Kind::FullSeries: {
    const double half = s.upper;
    const std::vector<Function> c = full_coefficients(f, s);
    Function sum = c[0] * 0.5;
    for (std::int64_t k = 1; k < s.terms; ++k) {
      const double u = static_cast<double>(k) * kPi / half;
      sum = sum + c[static_cast<std::size_t>(k)] * (x * u).cos() +
            c[static_cast<std::size_t>(s.terms - 1 + k)] * (x * u).sin();
    }
    return sum;
  }
  }
  PLUSS_FAIL("operators::fourier: unknown kind");
}

const char *key_of(Kind k) {
  switch (k) {
  case Kind::CosineCoefficients: return "fourier_cosine_coefficients";
  case Kind::SineCoefficients: return "fourier_sine_coefficients";
  case Kind::CosineSeries: return "fourier_cosine_series";
  case Kind::SineSeries: return "fourier_sine_series";
  case Kind::FullCoefficients: return "fourier_coefficients";
  case Kind::FullSeries: return "fourier_series";
  }
  return "fourier";
}

Operator make(const Spec &s, const char *who) {
  check(s, who);
  Capabilities capabilities = structural_linear();
  capabilities.serializable = true;
  return detail::leaf(
      key_of(s.kind), capabilities, [s](const Function &f) { return apply_spec(f, s); },
      detail::OperatorKind::Leaf, 1,
      pack_all({std::to_string(static_cast<int>(s.kind)), s.variable, number(s.lower), number(s.upper),
                std::to_string(s.terms), number(s.from), number(s.to)}));
}

} // namespace

Operator cosine_coefficients(std::string_view variable, double lower, double upper, std::int64_t terms) {
  return make({Kind::CosineCoefficients, std::string(variable), lower, upper, terms, lower, upper}, "operators::fourier::cosine_coefficients");
}
Operator cosine_coefficients(std::string_view variable, double lower, double upper, std::int64_t terms, double from, double to) {
  return make({Kind::CosineCoefficients, std::string(variable), lower, upper, terms, from, to}, "operators::fourier::cosine_coefficients");
}
Operator sine_coefficients(std::string_view variable, double lower, double upper, std::int64_t terms) {
  return make({Kind::SineCoefficients, std::string(variable), lower, upper, terms, lower, upper}, "operators::fourier::sine_coefficients");
}
Operator sine_coefficients(std::string_view variable, double lower, double upper, std::int64_t terms, double from, double to) {
  return make({Kind::SineCoefficients, std::string(variable), lower, upper, terms, from, to}, "operators::fourier::sine_coefficients");
}
Operator cosine_series(std::string_view variable, double lower, double upper, std::int64_t terms) {
  return make({Kind::CosineSeries, std::string(variable), lower, upper, terms, lower, upper}, "operators::fourier::cosine_series");
}
Operator sine_series(std::string_view variable, double lower, double upper, std::int64_t terms) {
  return make({Kind::SineSeries, std::string(variable), lower, upper, terms, lower, upper}, "operators::fourier::sine_series");
}
Operator coefficients(std::string_view variable, double half_period, std::int64_t terms) {
  PLUSS_CHECK(half_period > 0.0 && std::isfinite(half_period), "operators::fourier::coefficients: half_period must be positive");
  return make({Kind::FullCoefficients, std::string(variable), -half_period, half_period, terms, -half_period, half_period}, "operators::fourier::coefficients");
}
Operator series(std::string_view variable, double half_period, std::int64_t terms) {
  PLUSS_CHECK(half_period > 0.0 && std::isfinite(half_period), "operators::fourier::series: half_period must be positive");
  return make({Kind::FullSeries, std::string(variable), -half_period, half_period, terms, -half_period, half_period}, "operators::fourier::series");
}

Operator restore_fourier(std::string_view payload) {
  const auto f = unpack_all(payload);
  PLUSS_CHECK(f.size() == 7, "math::io: malformed Fourier operator descriptor");
  Spec s{static_cast<Kind>(std::stoi(f[0])), f[1], real(f[2]), real(f[3]), std::stoll(f[4]), real(f[5]), real(f[6])};
  return make(s, "math::io::load_operator");
}

} // namespace pluss::subsp::math::operators::fourier

namespace pluss::subsp::math::operators::integral_transforms {

using math::Function;
using detail::pack_all;
using detail::unpack_all;

namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kInf = std::numeric_limits<double>::infinity();

enum class Kind : int { Fourier, InverseFourier, Cosine, Sine, Laplace, InverseLaplace };

struct Spec {
  Kind kind = Kind::Fourier;
  std::string variable;   // integration variable of the input (the frequency for the inverse)
  std::string frequency;  // new variable of the result (the original variable for the inverse)
  int convention = 0;  // for the inverse Laplace transform, the number of contour points
};

const char *key_of(Kind kind) {
  switch (kind) {
  case Kind::Fourier: return "fourier_transform";
  case Kind::InverseFourier: return "fourier_inverse_transform";
  case Kind::Cosine: return "fourier_cosine_transform";
  case Kind::Sine: return "fourier_sine_transform";
  case Kind::Laplace: return "laplace_transform";
  case Kind::InverseLaplace: return "laplace_inverse_transform";
  }
  return "fourier_transform";
}

// Talbot inversion on Weideman's optimized contour z(theta) = (N / t)(0.5017 theta cot(0.6407 theta) - 0.6122
// + 0.2645 i theta), theta in (-pi, pi), by the midpoint rule. Since e^{z t} = e^{N c(theta)} and
// z' = (N / t) c'(theta), the inverse is f(t) = (2 / t) sum_k Im[e^{N c_k} c'_k F(N c_k / t)], a finite sum of
// F composed with constants over t: an analytic graph, with no quadrature at evaluation time.
Function apply_inverse_laplace(const Function &f, const Spec &s) {
  const std::int64_t n = s.convention;  // the number of contour points
  std::vector<std::string> names;
  bool found = false;
  for (std::size_t i = 0; i < f.variables().size(); ++i) {
    const std::string_view name = f.variables()[i];
    if (name == s.variable) {
      found = true;
    } else {
      PLUSS_CHECK(name != s.frequency, "operators: the variable '" + s.frequency + "' already names a variable of the Function");
      names.emplace_back(name);
    }
  }
  PLUSS_CHECK(found, "operators: unknown variable '" + s.variable + "'");
  names.push_back(s.frequency);
  std::vector<std::string_view> views(names.begin(), names.end());
  math::detail::Trace trace{pluss::StackArray<std::string_view>{views}};
  const std::span<const Function> targets = trace.arguments();
  const Function t = targets[names.size() - 1];
  Function sum;
  for (std::int64_t k = 0; k < n / 2; ++k) {
    const double theta = kPi * (2.0 * static_cast<double>(k) + 1.0) / static_cast<double>(n);
    const double a = 0.6407 * theta;
    const double cot = std::cos(a) / std::sin(a);
    const std::complex<double> c(0.5017 * theta * cot - 0.6122, 0.2645 * theta);
    const std::complex<double> dc(0.5017 * (cot - a / (std::sin(a) * std::sin(a))), 0.2645);
    const std::complex<double> weight = std::exp(static_cast<double>(n) * c) * dc;
    const Function z = math::function(static_cast<double>(n) * c) / t;
    std::vector<Function> arguments;
    std::size_t next = 0;
    for (std::size_t i = 0; i < f.variables().size(); ++i) {
      arguments.push_back(f.variables()[i] == s.variable ? z : targets[next++]);
    }
    const Function term = (f.compose(pluss::RangeView<Function>(arguments)) * math::function(weight)).imag();
    sum = sum.defined() ? sum + term : term;
  }
  sum = std::move(trace).finish(sum * (2.0 / t));
  return sum;
}

Function apply_spec(const Function &f, const Spec &s) {
  PLUSS_CHECK(f.defined() && f.rank() == 0, "operators: the transform needs a scalar-valued Function");
  PLUSS_CHECK(math::detail::FunctionAccess::variable_index(f, s.variable) >= 0, "operators: unknown variable");
  Function w;
  if (s.kind == Kind::InverseLaplace) return apply_inverse_laplace(f, s);
  const Function lifted = math::detail::FunctionAccess::extend(f, s.frequency, w);
  const Function x = lifted.arg(s.variable);

  const auto convention = static_cast<fourier::Convention>(s.convention);
  const bool ordinary = convention == fourier::Convention::Ordinary;
  const bool unitary = convention == fourier::Convention::Unitary;
  const Function i_unit = math::function(std::complex<double>(0.0, 1.0));
  Function integrand;
  double prefactor = 1.0, lower = -kInf, upper = kInf;
  switch (s.kind) {
  case Kind::Fourier:
  case Kind::InverseFourier: {
    const Function phase = ordinary ? w * x * (2.0 * kPi) : w * x;
    const bool inverse = s.kind == Kind::InverseFourier;
    const Function wave = inverse ? phase.cos() + i_unit * phase.sin() : phase.cos() - i_unit * phase.sin();
    integrand = lifted * wave;
    prefactor = unitary ? 1.0 / std::sqrt(2.0 * kPi) : (inverse && !ordinary ? 1.0 / (2.0 * kPi) : 1.0);
    break;
  }
  case Kind::Cosine:
  case Kind::Sine: {
    const Function phase = ordinary ? w * x * (2.0 * kPi) : w * x;
    integrand = lifted * (s.kind == Kind::Cosine ? phase.cos() : phase.sin());
    prefactor = unitary ? std::sqrt(2.0 / kPi) : (ordinary ? 2.0 : 1.0);
    lower = 0.0;
    break;
  }
  case Kind::Laplace:
    integrand = lifted * (-(w * x)).exp();
    lower = 0.0;
    break;
  case Kind::InverseLaplace:
    break;
  }
  // closed form when the integrand is an exponential polynomial (the transform table), else the lazy integral
  const int table_kind = s.kind == Kind::Laplace ? 0 : s.kind == Kind::Cosine ? 1 : s.kind == Kind::Sine ? 2
                         : s.kind == Kind::Fourier ? 3 : 4;
  Function result = math::detail::FunctionAccess::transform_table(lifted, s.variable, s.frequency, table_kind, ordinary);
  if (!result.defined()) result = operators::integral(s.variable, lower, upper)(integrand);
  return prefactor == 1.0 ? result : result * prefactor;
}

} // namespace

Operator make(Spec s) {
  PLUSS_CHECK(!s.variable.empty() && !s.frequency.empty() && s.variable != s.frequency,
              "operators: the integration and frequency variables must be distinct, non-empty names");
  Capabilities capabilities = structural_linear();
  capabilities.serializable = true;
  const char *key = key_of(s.kind);
  std::string payload =
      pack_all({std::to_string(static_cast<int>(s.kind)), s.variable, s.frequency, std::to_string(s.convention)});
  return detail::leaf(
      key, capabilities, [s = std::move(s)](const Function &f) { return apply_spec(f, s); },
      detail::OperatorKind::Leaf, 1, std::move(payload));
}

bool is_transform_key(std::string_view key) {
  return key == "fourier_transform" || key == "fourier_inverse_transform" || key == "fourier_cosine_transform" ||
         key == "fourier_sine_transform" || key == "laplace_transform" || key == "laplace_inverse_transform";
}

Operator restore_transform(std::string_view, std::string_view payload) {
  const auto f = unpack_all(payload);
  PLUSS_CHECK(f.size() == 4, "math::io: malformed integral transform descriptor");
  return make({static_cast<Kind>(std::stoi(f[0])), f[1], f[2], std::stoi(f[3])});
}

} // namespace pluss::subsp::math::operators::integral_transforms

namespace pluss::subsp::math::operators::fourier {

Operator transform(std::string_view variable, std::string_view frequency, Convention convention) {
  return integral_transforms::make({integral_transforms::Kind::Fourier, std::string(variable), std::string(frequency),
                                    static_cast<int>(convention)});
}
Operator inverse_transform(std::string_view frequency, std::string_view variable, Convention convention) {
  return integral_transforms::make({integral_transforms::Kind::InverseFourier, std::string(frequency),
                                    std::string(variable), static_cast<int>(convention)});
}
Operator cosine_transform(std::string_view variable, std::string_view frequency, Convention convention) {
  return integral_transforms::make({integral_transforms::Kind::Cosine, std::string(variable), std::string(frequency),
                                    static_cast<int>(convention)});
}
Operator sine_transform(std::string_view variable, std::string_view frequency, Convention convention) {
  return integral_transforms::make({integral_transforms::Kind::Sine, std::string(variable), std::string(frequency),
                                    static_cast<int>(convention)});
}

} // namespace pluss::subsp::math::operators::fourier

namespace pluss::subsp::math::operators::laplace {

Operator transform(std::string_view variable, std::string_view frequency) {
  return integral_transforms::make(
      {integral_transforms::Kind::Laplace, std::string(variable), std::string(frequency), 0});
}

} // namespace pluss::subsp::math::operators::laplace

namespace pluss::subsp::math::operators::laplace {

Operator inverse_transform(std::string_view frequency, std::string_view variable, std::int64_t terms) {
  PLUSS_CHECK(terms >= 8 && terms <= 256 && terms % 2 == 0,
              "operators::laplace::inverse_transform: terms must be even and lie in [8, 256]");
  return integral_transforms::make({integral_transforms::Kind::InverseLaplace, std::string(frequency),
                                    std::string(variable), static_cast<int>(terms)});
}

} // namespace pluss::subsp::math::operators::laplace

namespace pluss::subsp::math::operators::measure {
namespace {

using math::Function;

std::vector<double> sorted_inside(pluss::RangeView<double> points, double lower, double upper) {
  std::vector<double> out;
  for (const double p : points) {
    PLUSS_CHECK(std::isfinite(p), "operators::measure: null points must be finite");
    if (p > lower && p < upper) out.push_back(p);
  }
  std::sort(out.begin(), out.end());
  out.erase(std::unique(out.begin(), out.end()), out.end());
  return out;
}

// the integral over [lower, upper], split at the null points
Function integrate(const Function &integrand, std::string_view variable, double lower, double upper,
                   const std::vector<double> &cuts) {
  Function total;
  double from = lower;
  for (std::size_t i = 0; i <= cuts.size(); ++i) {
    const double to = i < cuts.size() ? cuts[i] : upper;
    const Function piece = operators::integral(variable, from, to)(integrand);
    total = total.defined() ? total + piece : piece;
    from = to;
  }
  return total;
}

Function atoms_sum(const Function &f, std::string_view variable, const std::vector<double> &points,
                   const std::vector<double> &weights) {
  Function total;
  for (std::size_t i = 0; i < points.size(); ++i) {
    const Function term = math::detail::FunctionAccess::fix(f, variable, points[i]) * weights[i];
    total = total.defined() ? total + term : term;
  }
  return total;
}

Capabilities measure_capabilities(bool serializable) {
  Capabilities capabilities = structural_linear();
  capabilities.serializable = serializable;
  return capabilities;
}

} // namespace

Operator lebesgue(std::string_view variable, double lower, double upper, pluss::RangeView<double> null_points) {
  PLUSS_CHECK(!variable.empty() && lower < upper, "operators::measure::lebesgue: needs a variable and lower < upper");
  std::vector<double> cuts = sorted_inside(null_points, lower, upper);
  std::vector<std::string> fields{std::string(variable), detail::number(lower), detail::number(upper)};
  for (const double c : cuts) fields.push_back(detail::number(c));
  return detail::leaf(
      "measure_lebesgue", measure_capabilities(true),
      [name = std::string(variable), lower, upper, cuts](const Function &f) {
        return integrate(f, name, lower, upper, cuts);
      },
      detail::OperatorKind::Leaf, 1, detail::pack_all(fields));
}

Operator weighted(std::string_view variable, const Function &density, double lower, double upper,
                  pluss::RangeView<double> null_points) {
  PLUSS_CHECK(!variable.empty() && lower < upper && density.defined(),
              "operators::measure::weighted: needs a variable, a density and lower < upper");
  std::vector<double> cuts = sorted_inside(null_points, lower, upper);
  return detail::leaf(
      "measure_weighted", measure_capabilities(false),
      [name = std::string(variable), density, lower, upper, cuts](const Function &f) {
        return integrate(f * math::detail::FunctionAccess::rebind(density, f), name, lower, upper, cuts);
      },
      detail::OperatorKind::Leaf, 1);
}

Operator atomic(std::string_view variable, pluss::RangeView<double> points, pluss::RangeView<double> weights) {
  PLUSS_CHECK(!variable.empty() && points.size() == weights.size() && points.size() > 0,
              "operators::measure::atomic: needs matching, non-empty points and weights");
  std::vector<double> x(points.begin(), points.end()), w(weights.begin(), weights.end());
  for (std::size_t i = 0; i < x.size(); ++i)
    PLUSS_CHECK(std::isfinite(x[i]) && std::isfinite(w[i]), "operators::measure::atomic: points and weights must be finite");
  std::vector<std::string> fields{std::string(variable)};
  for (std::size_t i = 0; i < x.size(); ++i) {
    fields.push_back(detail::number(x[i]));
    fields.push_back(detail::number(w[i]));
  }
  return detail::leaf(
      "measure_atomic", measure_capabilities(true),
      [name = std::string(variable), x, w](const Function &f) { return atoms_sum(f, name, x, w); },
      detail::OperatorKind::Leaf, 1, detail::pack_all(fields));
}

Operator stieltjes(std::string_view variable, const Function &cdf, double lower, double upper,
                   pluss::RangeView<double> jump_points, pluss::RangeView<double> jump_sizes) {
  PLUSS_CHECK(!variable.empty() && lower < upper && cdf.defined() && jump_points.size() == jump_sizes.size(),
              "operators::measure::stieltjes: needs a variable, a cdf, lower < upper and matching jumps");
  std::vector<double> x(jump_points.begin(), jump_points.end()), s(jump_sizes.begin(), jump_sizes.end());
  const Operator derivative = operators::differential(variable);
  return detail::leaf(
      "measure_stieltjes", measure_capabilities(false),
      [name = std::string(variable), cdf, lower, upper, x, s, derivative](const Function &f) {
        const Function density = derivative(cdf);
        Function result = integrate(f * math::detail::FunctionAccess::rebind(density, f), name, lower, upper, {});
        if (!x.empty()) result = result + atoms_sum(f, name, x, s);
        return result;
      },
      detail::OperatorKind::Leaf, 1);
}

Operator restore_measure(std::string_view key, std::string_view payload) {
  const auto f = detail::unpack_all(payload);
  PLUSS_CHECK(!f.empty(), "math::io: malformed measure operator descriptor");
  std::vector<double> numbers;
  for (std::size_t i = 1; i < f.size(); ++i) numbers.push_back(detail::real(f[i]));
  if (key == "measure_lebesgue") {
    PLUSS_CHECK(numbers.size() >= 2, "math::io: malformed measure operator descriptor");
    const std::vector<double> cuts(numbers.begin() + 2, numbers.end());
    return lebesgue(f[0], numbers[0], numbers[1], pluss::RangeView<double>(cuts));
  }
  PLUSS_CHECK(numbers.size() % 2 == 0, "math::io: malformed measure operator descriptor");
  std::vector<double> points, weights;
  for (std::size_t i = 0; i < numbers.size(); i += 2) {
    points.push_back(numbers[i]);
    weights.push_back(numbers[i + 1]);
  }
  return atomic(f[0], pluss::RangeView<double>(points), pluss::RangeView<double>(weights));
}

} // namespace pluss::subsp::math::operators::measure

namespace pluss::subsp::math::operators::stochastic {

Operator malliavin_derivative(pluss::StackArray<std::string_view> path, DiffOptions options) {
  std::vector<std::string> names;
  for (const std::string_view v : std::move(path).lister()) names.emplace_back(v);
  PLUSS_CHECK(!names.empty(), "operators::stochastic::malliavin_derivative: the path needs at least one variable");
  Capabilities capabilities = structural_linear();
  capabilities.serializable = false;
  capabilities.serializable = true;
  std::vector<std::string> payload{std::to_string(static_cast<int>(options.nonsmooth)),
                                   std::to_string(static_cast<int>(options.fallback)), std::to_string(options.max_nodes)};
  payload.insert(payload.end(), names.begin(), names.end());
  return detail::leaf(
      "malliavin_derivative", capabilities,
      [names, options](const Function &F) {
        PLUSS_CHECK(F.rank() == 0, "operators::stochastic::malliavin_derivative: the functional must be scalar-valued");
        std::vector<Function> derivative(names.size());
        Function running;
        for (std::size_t k = names.size(); k-- > 0;) {
          const Function partial = differential(names[k], 1, options)(F);
          running = running.defined() ? running + partial : partial;
          derivative[k] = running;
        }
        return math::stack(pluss::RangeView<Function>(derivative));
      },
      detail::OperatorKind::Leaf, 1, detail::pack_all(payload));
}

Operator skorokhod_integral(pluss::StackArray<std::string_view> path, double dt, DiffOptions options) {
  std::vector<std::string> names;
  for (const std::string_view v : std::move(path).lister()) names.emplace_back(v);
  PLUSS_CHECK(!names.empty() && dt > 0.0 && std::isfinite(dt),
              "operators::stochastic::skorokhod_integral: needs a non-empty path and a positive step");
  Capabilities capabilities = structural_linear();
  capabilities.serializable = false;
  capabilities.serializable = true;
  std::vector<std::string> payload{std::to_string(static_cast<int>(options.nonsmooth)),
                                   std::to_string(static_cast<int>(options.fallback)), std::to_string(options.max_nodes),
                                   detail::number(dt)};
  payload.insert(payload.end(), names.begin(), names.end());
  return detail::leaf(
      "skorokhod_integral", capabilities,
      [names, dt, options](const Function &u) {
        PLUSS_CHECK(u.rank() == 1 && u.shape()[0] == static_cast<std::int64_t>(names.size()),
                    "operators::stochastic::skorokhod_integral: u must have one component per path step");
        Function total;
        for (std::size_t i = 0; i < names.size(); ++i) {
          const Function component = u.select(0, static_cast<std::int64_t>(i));
          const Function increment = i == 0 ? u.arg(names[0]) : u.arg(names[i]) - u.arg(names[i - 1]);
          Function term = component * increment;
          for (std::size_t j = i; j < names.size(); ++j) {
            term = term - differential(names[j], 1, options)(component) * dt;
          }
          total = total.defined() ? total + term : term;
        }
        return total;
      },
      detail::OperatorKind::Leaf, 1, detail::pack_all(payload));
}

Operator restore_malliavin(std::string_view key, std::string_view payload) {
  const auto f = detail::unpack_all(payload);
  PLUSS_CHECK(f.size() >= 4, "math::io: malformed Malliavin operator descriptor");
  DiffOptions options;
  options.nonsmooth = static_cast<math::NonSmooth>(std::stoi(f[0]));
  options.fallback = static_cast<math::Fallback>(std::stoi(f[1]));
  options.max_nodes = static_cast<std::size_t>(std::stoull(f[2]));
  if (key == "malliavin_derivative") {
    std::vector<std::string_view> views(f.begin() + 3, f.end());
    return malliavin_derivative(pluss::StackArray<std::string_view>{views}, options);
  }
  std::vector<std::string_view> views(f.begin() + 4, f.end());
  return skorokhod_integral(pluss::StackArray<std::string_view>{views}, detail::real(f[3]), options);
}

} // namespace pluss::subsp::math::operators::stochastic

namespace pluss::subsp::math::operators {
namespace {

using math::Function;

std::vector<std::string> names_of(pluss::StackArray<std::string_view> variables) {
  std::vector<std::string> names;
  for (const std::string_view v : std::move(variables).lister()) names.emplace_back(v);
  return names;
}

Capabilities extension_capabilities(bool serializable) {
  Capabilities c = structural_linear();
  c.serializable = serializable;
  return c;
}

Function partial(const Function &f, const std::string &variable, const DiffOptions &options) {
  return differential(variable, 1, options)(f);
}

std::string names_payload(const std::vector<std::string> &names, const DiffOptions &options) {
  std::vector<std::string> fields{std::to_string(static_cast<int>(options.nonsmooth)),
                                  std::to_string(static_cast<int>(options.fallback)),
                                  std::to_string(options.max_nodes)};
  fields.insert(fields.end(), names.begin(), names.end());
  return detail::pack_all(fields);
}

} // namespace

Operator divergence(pluss::StackArray<std::string_view> variables, DiffOptions options) {
  const std::vector<std::string> names = names_of(std::move(variables));
  PLUSS_CHECK(!names.empty(), "operators::divergence: needs at least one coordinate");
  return detail::leaf(
      "vector_divergence", extension_capabilities(true),
      [names, options](const Function &F) {
        PLUSS_CHECK(F.rank() == 1 && F.shape()[0] == static_cast<std::int64_t>(names.size()),
                    "operators::divergence: the field needs one component per coordinate");
        Function total;
        for (std::size_t i = 0; i < names.size(); ++i) {
          const Function term = partial(F.select(0, static_cast<std::int64_t>(i)), names[i], options);
          total = total.defined() ? total + term : term;
        }
        return total;
      },
      detail::OperatorKind::Leaf, 1, names_payload(names, options));
}

Operator curl(pluss::StackArray<std::string_view> variables, DiffOptions options) {
  const std::vector<std::string> names = names_of(std::move(variables));
  PLUSS_CHECK(names.size() == 2 || names.size() == 3, "operators::curl: needs two or three coordinates");
  return detail::leaf(
      "vector_curl", extension_capabilities(true),
      [names, options](const Function &F) {
        PLUSS_CHECK(F.rank() == 1 && F.shape()[0] == static_cast<std::int64_t>(names.size()),
                    "operators::curl: the field needs one component per coordinate");
        const auto d = [&](std::int64_t component, std::size_t along) {
          return partial(F.select(0, component), names[along], options);
        };
        if (names.size() == 2) return d(1, 0) - d(0, 1);
        return math::stack({d(2, 1) - d(1, 2), d(0, 2) - d(2, 0), d(1, 0) - d(0, 1)});
      },
      detail::OperatorKind::Leaf, 1, names_payload(names, options));
}

Operator laplacian(pluss::StackArray<std::string_view> variables, DiffOptions options) {
  const std::vector<std::string> names = names_of(std::move(variables));
  PLUSS_CHECK(!names.empty(), "operators::laplacian: needs at least one coordinate");
  return detail::leaf(
      "vector_laplacian", extension_capabilities(true),
      [names, options](const Function &f) {
        PLUSS_CHECK(f.rank() == 0, "operators::laplacian: the function must be scalar-valued");
        Function total;
        for (const std::string &name : names) {
          const Function term = differential(name, 2, options)(f);
          total = total.defined() ? total + term : term;
        }
        return total;
      },
      detail::OperatorKind::Leaf, 1, names_payload(names, options));
}

Operator directional(Function direction, pluss::StackArray<std::string_view> variables, DiffOptions options) {
  const std::vector<std::string> names = names_of(std::move(variables));
  PLUSS_CHECK(direction.defined() && direction.rank() == 1 &&
                  direction.shape()[0] == static_cast<std::int64_t>(names.size()),
              "operators::directional: the direction needs one component per coordinate");
  return detail::leaf(
      "vector_directional", extension_capabilities(false),
      [names, direction, options](const Function &f) {
        PLUSS_CHECK(f.rank() == 0, "operators::directional: the function must be scalar-valued");
        const Function v = math::detail::FunctionAccess::rebind(direction, f);
        Function total;
        for (std::size_t i = 0; i < names.size(); ++i) {
          const Function term = v.select(0, static_cast<std::int64_t>(i)) * partial(f, names[i], options);
          total = total.defined() ? total + term : term;
        }
        return total;
      },
      detail::OperatorKind::Leaf, 1);
}

Operator material(Function velocity, std::string_view time, pluss::StackArray<std::string_view> variables,
                  DiffOptions options) {
  const Operator convective = directional(velocity, std::move(variables), options);
  const std::string t(time);
  return detail::leaf(
      "vector_material", extension_capabilities(false),
      [convective, t, options](const Function &f) { return partial(f, t, options) + convective(f); },
      detail::OperatorKind::Leaf, 1);
}

namespace variational {
namespace {

Function total_derivative(const Function &g, const std::string &x, const std::vector<std::string> &jets,
                          const DiffOptions &options) {
  Function result = partial(g, x, options);
  for (std::size_t k = 0; k < jets.size(); ++k) {
    const Function dg = partial(g, jets[k], options);
    if (k + 1 == jets.size()) {
      PLUSS_CHECK(math::detail::FunctionAccess::is_zero(dg),
                  "operators::variational: the Lagrangian reaches the highest listed jet; list more orders "
                  "(twice the highest order L uses)");
      continue;
    }
    result = result + g.arg(jets[k + 1]) * dg;
  }
  return result;
}

} // namespace

Operator functional_derivative(std::string_view independent, pluss::StackArray<std::string_view> jets,
                               DiffOptions options) {
  const std::vector<std::string> names = names_of(std::move(jets));
  PLUSS_CHECK(!independent.empty() && names.size() >= 2,
              "operators::variational: needs an independent variable and at least u and u'");
  const std::string x(independent);
  std::vector<std::string> payload{x};
  payload.insert(payload.end(), names.begin(), names.end());
  Capabilities capabilities = extension_capabilities(true);
  capabilities.linear = false;
  return detail::leaf(
      "variational_functional_derivative", capabilities,
      [x, names, options](const Function &L) {
        PLUSS_CHECK(L.rank() == 0, "operators::variational: the Lagrangian must be scalar-valued");
        Function result;
        for (std::size_t k = 0; k < names.size(); ++k) {
          Function term = partial(L, names[k], options);
          if (math::detail::FunctionAccess::is_zero(term)) continue;
          for (std::size_t j = 0; j < k; ++j) term = total_derivative(term, x, names, options) * -1.0;
          result = result.defined() ? result + term : term;
        }
        return result.defined() ? result : L * 0.0;
      },
      detail::OperatorKind::Leaf, 1, detail::pack_all(payload));
}

Operator euler_lagrange(std::string_view independent, pluss::StackArray<std::string_view> jets, DiffOptions options) {
  return functional_derivative(independent, std::move(jets), options);
}

} // namespace variational

namespace fractional {
namespace {

// the lazy Riemann-Liouville integral of order beta in (0, inf) from `lower`
Function lazy_integral(const Function &f, const std::string &variable, double beta, double lower) {
  Function s;
  const Function lifted = math::detail::FunctionAccess::extend(f, "__fractional_s", s);
  const Function x = lifted.arg(variable);
  s = lifted.arg("__fractional_s");
  const Function span = lower == 0.0 ? x : x - lower;
  const Function at = x - span * s.pow(1.0 / beta);
  const Function moved = math::detail::FunctionAccess::replace(lifted, variable, at);
  const Function inner = operators::integral("__fractional_s", 0.0, 1.0)(moved);
  return inner * span.pow(beta) * (1.0 / std::tgamma(beta + 1.0));
}

Function apply_fractional(const Function &f, const std::string &variable, double alpha, int mode, double lower) {
  PLUSS_CHECK(f.defined() && f.rank() == 0, "operators::fractional: the function must be scalar-valued");
  const double m = std::ceil(alpha);
  if (mode != 0 && alpha == m) {  // integer order: the ordinary derivative
    return operators::differential(variable, static_cast<std::int64_t>(m))(f);
  }
  if (lower == 0.0) {
    const Function closed = math::detail::FunctionAccess::fractional_table(f, variable, alpha, mode);
    if (closed.defined()) return closed;
  }
  if (mode == 0) return lazy_integral(f, variable, alpha, lower);
  const double beta = m - alpha;
  if (mode == 2) return lazy_integral(operators::differential(variable, static_cast<std::int64_t>(m))(f), variable, beta, lower);
  return operators::differential(variable, static_cast<std::int64_t>(m))(lazy_integral(f, variable, beta, lower));
}

Operator make(std::string key, std::string variable, double alpha, int mode, double lower) {
  PLUSS_CHECK(!variable.empty() && std::isfinite(alpha) && alpha > 0.0 && std::isfinite(lower),
              "operators::fractional: needs a variable, alpha > 0 and a finite lower terminal");
  return detail::leaf(
      std::move(key), extension_capabilities(true),
      [variable, alpha, mode, lower](const Function &f) { return apply_fractional(f, variable, alpha, mode, lower); },
      detail::OperatorKind::Leaf, 1,
      detail::pack_all({variable, detail::number(alpha), std::to_string(mode), detail::number(lower)}));
}

} // namespace

Operator integral(std::string_view variable, double alpha, double lower) {
  return make("fractional_integral", std::string(variable), alpha, 0, lower);
}

Operator differential(std::string_view variable, double alpha, Kind kind, double lower) {
  return make("fractional_differential", std::string(variable), alpha, kind == Kind::Caputo ? 2 : 1, lower);
}

Operator restore_fractional(std::string_view key, std::string_view payload) {
  const auto f = detail::unpack_all(payload);
  PLUSS_CHECK(f.size() == 4, "math::io: malformed fractional operator descriptor");
  return make(std::string(key), f[0], detail::real(f[1]), std::stoi(f[2]), detail::real(f[3]));
}

} // namespace fractional

Operator convolution(std::string_view variable, Function kernel) {
  PLUSS_CHECK(!variable.empty() && kernel.defined(), "operators::convolution: needs a variable and a kernel");
  return detail::leaf(
      "convolution", extension_capabilities(false),
      [x = std::string(variable), kernel](const Function &f) {
        Function t;
        const Function lifted = math::detail::FunctionAccess::extend(f, "__convolution_t", t);
        t = lifted.arg("__convolution_t");
        const Function at_t = math::detail::FunctionAccess::replace(lifted, x, t);
        const Function k = math::detail::FunctionAccess::rebind(kernel, lifted);
        const Function k_shifted = math::detail::FunctionAccess::replace(k, x, lifted.arg(x) - t);
        return operators::integral("__convolution_t", -std::numeric_limits<double>::infinity(),
                                   std::numeric_limits<double>::infinity())(at_t * k_shifted);
      },
      detail::OperatorKind::Leaf, 1);
}

Operator restore_extension(std::string_view key, std::string_view payload) {
  const auto f = detail::unpack_all(payload);
  if (key.starts_with("vector_")) {
    PLUSS_CHECK(f.size() >= 4, "math::io: malformed vector operator descriptor");
    DiffOptions options;
    options.nonsmooth = static_cast<math::NonSmooth>(std::stoi(f[0]));
    options.fallback = static_cast<math::Fallback>(std::stoi(f[1]));
    options.max_nodes = static_cast<std::size_t>(std::stoull(f[2]));
    std::vector<std::string_view> views(f.begin() + 3, f.end());
    pluss::StackArray<std::string_view> names{views};
    if (key == "vector_divergence") return divergence(std::move(names), options);
    if (key == "vector_curl") return curl(std::move(names), options);
    return laplacian(std::move(names), options);
  }
  PLUSS_CHECK(key == "variational_functional_derivative" && f.size() >= 3, "math::io: unknown extension operator");
  std::vector<std::string_view> views(f.begin() + 1, f.end());
  return variational::functional_derivative(f[0], pluss::StackArray<std::string_view>{views});
}

} // namespace pluss::subsp::math::operators
