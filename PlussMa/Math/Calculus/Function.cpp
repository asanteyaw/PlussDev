#include "FunctionInternal.h"

#include "pluss/error.h"
#include "pluss/subsp/math/integ.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <complex>
#include <limits>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace pluss::subsp::math::detail {

namespace {

std::atomic<std::uint64_t> next_trace_id{1};
constexpr std::size_t absolute_graph_limit = 1000000;
constexpr std::int64_t max_output_rank = 4;

pluss::msharp::intrusive_ptr<TraceState> empty_trace() {
  return pluss::msharp::make_intrusive<TraceState>(
      next_trace_id.fetch_add(1, std::memory_order_relaxed),
      pluss::msharp::Lister::StringView{});
}

void validate_names(const pluss::msharp::Lister::StringView &names) {
  std::unordered_set<std::string_view> seen;
  seen.reserve(names.size());
  for (const std::string_view name : names) {
    PLUSS_CHECK(!name.empty(),
                "math::function: variable names must not be empty");
    PLUSS_CHECK(seen.emplace(name).second,
                "math::function: duplicate variable name '" +
                    std::string(name) + "'");
  }
}

void validate_root(const Node &root, const TraceState &trace) {
  std::vector<const Node *> pending{&root};
  std::unordered_set<const Node *> seen;
  while (!pending.empty()) {
    const Node *node = pending.back();
    pending.pop_back();
    if (!seen.emplace(node).second) {
      continue;
    }
    if (node->kind() == NodeKind::Projection) {
      PLUSS_CHECK(
          node->trace_id() == trace.id(),
          "math::function: callable returned an expression containing a "
          "projection bound to a different function trace");
      PLUSS_CHECK(
          node->variable_id() >= 0 &&
              static_cast<std::size_t>(node->variable_id()) <
                  trace.variables().size(),
          "math::function: projection variable binding is out of range");
    }
    for (const auto &child : node->children()) {
      pending.push_back(child.get());
    }
  }
}

std::vector<std::int64_t> broadcast_shape(const OutputMetadata &left,
                                          const OutputMetadata &right) {
  const std::size_t rank = std::max(left.shape.size(), right.shape.size());
  std::vector<std::int64_t> output(rank, 1);
  for (std::size_t offset = 0; offset < rank; ++offset) {
    const std::int64_t lhs = offset < left.shape.size()
                                 ? left.shape[left.shape.size() - 1 - offset]
                                 : 1;
    const std::int64_t rhs = offset < right.shape.size()
                                 ? right.shape[right.shape.size() - 1 - offset]
                                 : 1;
    PLUSS_CHECK(lhs == rhs || lhs == 1 || rhs == 1,
                "math::Function: output shapes are not broadcast-compatible");
    output[rank - 1 - offset] = std::max(lhs, rhs);
  }
  return output;
}

std::vector<ExecutionStep>
build_plan(const pluss::msharp::intrusive_ptr<Node> &root) {
  std::vector<pluss::msharp::intrusive_ptr<Node>> order;
  std::vector<std::pair<pluss::msharp::intrusive_ptr<Node>, bool>> pending;
  std::unordered_set<const Node *> emitted;
  pending.emplace_back(root, false);

  while (!pending.empty()) {
    auto [node, finish] = std::move(pending.back());
    pending.pop_back();
    if (emitted.contains(node.get())) {
      continue;
    }
    if (!finish) {
      pending.emplace_back(node, true);
      if (node->kind() == NodeKind::Integral) {
        continue;
      }
      const auto &children = node->children();
      for (auto it = children.rbegin(); it != children.rend(); ++it) {
        if (!emitted.contains(it->get())) {
          pending.emplace_back(*it, false);
        }
      }
      continue;
    }
    emitted.emplace(node.get());
    PLUSS_CHECK(emitted.size() <= absolute_graph_limit,
                "math::Function: graph exceeds the absolute node safeguard");
    order.push_back(std::move(node));
  }

  std::unordered_map<const Node *, std::size_t> positions;
  positions.reserve(order.size());
  std::vector<ExecutionStep> plan;
  plan.reserve(order.size());
  for (std::size_t i = 0; i < order.size(); ++i) {
    positions.emplace(order[i].get(), i);
    ExecutionStep step;
    step.node = order[i];
    if (order[i]->kind() == NodeKind::Integral) {
      step.integral_plan = build_plan(order[i]->children().front());
      plan.push_back(std::move(step));
      continue;
    }
    step.inputs.reserve(order[i]->children().size());
    for (std::size_t child = 0; child < order[i]->children().size(); ++child) {
      const auto found = positions.find(order[i]->children()[child].get());
      PLUSS_CHECK(found != positions.end(),
                  "math::Function: graph plan is not topologically ordered");
      step.inputs.push_back(found->second);
    }
    plan.push_back(std::move(step));
  }
  std::vector<std::size_t> last_use(plan.size(), plan.size());
  for (std::size_t step = 0; step < plan.size(); ++step) {
    for (const std::size_t input : plan[step].inputs) {
      last_use[input] = step;
    }
  }
  for (std::size_t input = 0; input < last_use.size(); ++input) {
    if (last_use[input] != plan.size()) {
      plan[last_use[input]].releases.push_back(input);
    }
  }
  return plan;
}

pluss::msharp::intrusive_ptr<TraceState>
merge_trace(const FunctionImpl &left, const FunctionImpl &right) {
  const bool left_claims_trace =
      left.depends_on_variables() || !left.trace().variables().empty();
  const bool right_claims_trace =
      right.depends_on_variables() || !right.trace().variables().empty();
  if (left_claims_trace && right_claims_trace) {
    PLUSS_CHECK(left.trace().id() == right.trace().id(),
                "math::Function: expressions from different variable traces "
                "cannot be combined directly");
  }
  if (left_claims_trace) {
    return left.trace_ptr();
  }
  if (right_claims_trace) {
    return right.trace_ptr();
  }
  return left.trace_ptr();
}

using NodePtr = pluss::msharp::intrusive_ptr<Node>;

bool scalar_is(const NodePtr &node, double value) noexcept {
  return node->kind() == NodeKind::ScalarConstant &&
         node->scalar_value() == value;
}

NodePtr unary_node(PrimitiveId primitive_id, NodePtr input) {
  if (primitive_id == PrimitiveId::Neg && scalar_is(input, 0.0)) {
    return input;
  }
  if (primitive_id == PrimitiveId::Neg &&
      input->kind() == NodeKind::Primitive &&
      input->primitive_id() == PrimitiveId::Neg) {
    return input->children()[0];
  }
  const OutputMetadata output = input->output();
  return Node::primitive(primitive_id, {std::move(input)}, output);
}

NodePtr complex_scalar(double real, double imag) {
  return Node::series(
      pluss::make_series({std::complex<double>{real, imag}}).reshape({}));
}

NodePtr binary_node(PrimitiveId primitive_id, NodePtr left, NodePtr right);

NodePtr safe_divide(NodePtr numerator, NodePtr denominator,
                    NonSmooth nonsmooth) {
  const PrimitiveId primitive =
      nonsmooth == NonSmooth::Strict       ? PrimitiveId::SafeDivStrict
      : nonsmooth == NonSmooth::Piecewise  ? PrimitiveId::SafeDivPiecewise
                                           : PrimitiveId::SafeDivZero;
  return binary_node(primitive, std::move(numerator), std::move(denominator));
}

NodePtr binary_node(PrimitiveId primitive_id, NodePtr left, NodePtr right) {
  if (left->kind() == NodeKind::ScalarConstant &&
      right->kind() == NodeKind::ScalarConstant) {
    const double lhs = left->scalar_value();
    const double rhs = right->scalar_value();
    switch (primitive_id) {
    case PrimitiveId::Add:
      return Node::scalar(lhs + rhs);
    case PrimitiveId::Sub:
      return Node::scalar(lhs - rhs);
    case PrimitiveId::Mul:
      return Node::scalar(lhs * rhs);
    case PrimitiveId::Div:
      if (rhs != 0.0) {
        return Node::scalar(lhs / rhs);
      }
      break;
    case PrimitiveId::Pow: {
      const double value = std::pow(lhs, rhs);
      if (std::isfinite(value)) {
        return Node::scalar(value);
      }
      break;
    }
    default:
      break;
    }
  }
  if (primitive_id == PrimitiveId::Add) {
    if (scalar_is(left, 0.0)) {
      return right;
    }
    if (scalar_is(right, 0.0)) {
      return left;
    }
  } else if (primitive_id == PrimitiveId::Sub) {
    if (scalar_is(right, 0.0)) {
      return left;
    }
    if (left.get() == right.get()) {
      return unary_node(PrimitiveId::ZerosLike, left);
    }
  } else if (primitive_id == PrimitiveId::Mul) {
    if (scalar_is(left, 0.0)) {
      return unary_node(PrimitiveId::ZerosLike, right);
    }
    if (scalar_is(right, 0.0)) {
      return unary_node(PrimitiveId::ZerosLike, left);
    }
    if (scalar_is(left, 1.0)) {
      return right;
    }
    if (scalar_is(right, 1.0)) {
      return left;
    }
  } else if (primitive_id == PrimitiveId::Div) {
    if (scalar_is(right, 1.0)) {
      return left;
    }
  } else if (primitive_id == PrimitiveId::Pow) {
    if (scalar_is(right, 0.0)) {
      return unary_node(PrimitiveId::OnesLike, left);
    }
    if (scalar_is(right, 1.0)) {
      return left;
    }
  }

  OutputMetadata output;
  output.shape = broadcast_shape(left->output(), right->output());
  return Node::primitive(primitive_id, {std::move(left), std::move(right)},
                         std::move(output));
}

NodePtr stack_nodes(std::vector<NodePtr> children) {
  PLUSS_CHECK(!children.empty(),
              "math::Function: structural stack requires at least one input");
  const std::vector<std::int64_t> component_shape =
      children.front()->output().shape;
  for (const NodePtr &child : children) {
    PLUSS_CHECK(child->output().shape == component_shape,
                "math::Function: stacked outputs must have equal shapes");
  }
  OutputMetadata output;
  output.shape = component_shape;
  output.shape.push_back(static_cast<std::int64_t>(children.size()));
  PLUSS_CHECK(output.rank() <= max_output_rank,
              "math::Function: outputs are limited to rank " +
                  std::to_string(max_output_rank) +
                  " (scalar, vector, matrix, and tensors up to rank 4)");
  return Node::primitive(PrimitiveId::Stack, std::move(children),
                         std::move(output));
}

// Tensor primitives: output shape rules. Output axes are the trailing axes of
// a value; `attributes` hold the integer parameters (see PrimitiveRegistry.cpp).
NodePtr tensor_node(PrimitiveId primitive_id, std::vector<NodePtr> children,
                    std::vector<std::int64_t> attributes) {
  OutputMetadata output;
  switch (primitive_id) {
  case PrimitiveId::Transpose: {
    PLUSS_CHECK(attributes.size() == 3,
                "math::Function: malformed transpose attributes");
    output = children[0]->output();
    PLUSS_CHECK(attributes[0] == output.rank() && attributes[1] >= 0 &&
                    attributes[2] >= 0 && attributes[1] < output.rank() &&
                    attributes[2] < output.rank(),
                "math::Function: transpose axes are outside the output shape");
    std::swap(output.shape[static_cast<std::size_t>(attributes[1])],
              output.shape[static_cast<std::size_t>(attributes[2])]);
    break;
  }
  case PrimitiveId::Select: {
    PLUSS_CHECK(attributes.size() == 3,
                "math::Function: malformed select attributes");
    output = children[0]->output();
    PLUSS_CHECK(attributes[0] == output.rank() && attributes[1] >= 0 &&
                    attributes[1] < output.rank() && attributes[2] >= 0 &&
                    attributes[2] <
                        output.shape[static_cast<std::size_t>(attributes[1])],
                "math::Function: select index is outside the output shape");
    output.shape.erase(output.shape.begin() + attributes[1]);
    break;
  }
  case PrimitiveId::Inverse:
  case PrimitiveId::Det:
  case PrimitiveId::Trace: {
    output = children[0]->output();
    PLUSS_CHECK(output.rank() == 2 && output.shape[0] == output.shape[1],
                "math::Function: this operation requires a square matrix "
                "output");
    if (primitive_id != PrimitiveId::Inverse) {
      output.shape.clear();
    }
    break;
  }
  case PrimitiveId::PathIntegral:
  case PrimitiveId::PathCovariation: {
    PLUSS_CHECK(children[0]->output().rank() == 0 &&
                    children[1]->output().rank() == 0,
                "math::Function: path functionals act on scalar-valued "
                "integrands and processes");
    PLUSS_CHECK(attributes.size() ==
                    (primitive_id == PrimitiveId::PathIntegral ? 3U : 2U),
                "math::Function: malformed path functional attributes");
    break; // scalar result: the path axis is consumed
  }
  case PrimitiveId::Contract: {
    PLUSS_CHECK(attributes.size() >= 3,
                "math::Function: malformed contract attributes");
    const std::int64_t rank_a = attributes[0], rank_b = attributes[1],
                       n = attributes[2];
    PLUSS_CHECK(static_cast<std::int64_t>(attributes.size()) == 3 + 2 * n &&
                    children[0]->output().rank() == rank_a &&
                    children[1]->output().rank() == rank_b,
                "math::Function: contraction attributes do not match operand "
                "ranks");
    std::vector<bool> used_a(static_cast<std::size_t>(rank_a), false),
        used_b(static_cast<std::size_t>(rank_b), false);
    for (std::int64_t k = 0; k < n; ++k) {
      const std::int64_t i = attributes[static_cast<std::size_t>(3 + k)];
      const std::int64_t j = attributes[static_cast<std::size_t>(3 + n + k)];
      PLUSS_CHECK(i >= 0 && i < rank_a && j >= 0 && j < rank_b &&
                      !used_a[static_cast<std::size_t>(i)] &&
                      !used_b[static_cast<std::size_t>(j)],
                  "math::Function: contraction axes are invalid or repeated");
      PLUSS_CHECK(children[0]->output().shape[static_cast<std::size_t>(i)] ==
                      children[1]->output().shape[static_cast<std::size_t>(j)],
                  "math::Function: contracted axes have different lengths");
      used_a[static_cast<std::size_t>(i)] = true;
      used_b[static_cast<std::size_t>(j)] = true;
    }
    for (std::int64_t i = 0; i < rank_a; ++i) {
      if (!used_a[static_cast<std::size_t>(i)]) {
        output.shape.push_back(children[0]->output().shape[static_cast<std::size_t>(i)]);
      }
    }
    for (std::int64_t j = 0; j < rank_b; ++j) {
      if (!used_b[static_cast<std::size_t>(j)]) {
        output.shape.push_back(children[1]->output().shape[static_cast<std::size_t>(j)]);
      }
    }
    break;
  }
  default:
    PLUSS_FAIL("math::Function: not a tensor primitive");
  }
  PLUSS_CHECK(output.rank() <= max_output_rank,
              "math::Function: tensor result exceeds the maximum output rank");
  return Node::primitive(primitive_id, std::move(children), std::move(output),
                         std::move(attributes));
}

// matrix product / dot / matrix-vector as a contraction of the last axis of
// `left` with the first axis of `right`.
NodePtr matmul_node(const NodePtr &left, const NodePtr &right) {
  const std::int64_t rank_a = left->output().rank();
  const std::int64_t rank_b = right->output().rank();
  PLUSS_CHECK(rank_a >= 1 && rank_b >= 1,
              "math::Function: matmul requires vector or matrix outputs");
  return tensor_node(PrimitiveId::Contract, {left, right},
                     {rank_a, rank_b, 1, rank_a - 1, 0});
}

bool depends_on(const NodePtr &node, std::int64_t variable_id,
                std::unordered_map<const Node *, bool> &memo) {
  if (const auto found = memo.find(node.get()); found != memo.end()) {
    return found->second;
  }
  bool result = (node->kind() == NodeKind::Projection ||
                 node->kind() == NodeKind::Opaque) &&
                node->variable_id() == variable_id;
  if (node->kind() == NodeKind::Integral &&
      node->integral_variable_id() == variable_id) {
    result = !node->integral_upper().has_value();
  } else if (!result) {
    for (const NodePtr &child : node->children()) {
      if (depends_on(child, variable_id, memo)) {
        result = true;
        break;
      }
    }
  }
  memo.emplace(node.get(), result);
  return result;
}

bool depends_on(const NodePtr &node, std::int64_t variable_id) {
  std::unordered_map<const Node *, bool> memo;
  return depends_on(node, variable_id, memo);
}

NodePtr rebuild_node(const NodePtr &node, std::vector<NodePtr> children) {
  if (node->kind() == NodeKind::Primitive) {
    if (node->primitive_id() == PrimitiveId::Stack) {
      return stack_nodes(std::move(children));
    }
    if (carries_attributes(node->primitive_id())) {
      return tensor_node(node->primitive_id(), std::move(children),
                         node->attributes());
    }
    if (children.size() == 1) {
      return unary_node(node->primitive_id(), std::move(children[0]));
    }
    return binary_node(node->primitive_id(), std::move(children[0]),
                       std::move(children[1]));
  }
  if (node->kind() == NodeKind::Integral) {
    return Node::integral(std::move(children[0]), node->integral_variable_id(),
                          node->integral_lower(), node->integral_upper());
  }
  return node;
}

NodePtr substitute(const NodePtr &node, std::int64_t variable_id,
                   const NodePtr &replacement,
                   std::unordered_map<const Node *, NodePtr> &memo) {
  if (const auto found = memo.find(node.get()); found != memo.end()) {
    return found->second;
  }
  NodePtr result = node;
  if (node->kind() == NodeKind::Projection &&
      node->variable_id() == variable_id) {
    result = replacement;
  } else if (!node->children().empty()) {
    std::vector<NodePtr> children;
    children.reserve(node->children().size());
    for (const NodePtr &child : node->children()) {
      children.push_back(substitute(child, variable_id, replacement, memo));
    }
    result = rebuild_node(node, std::move(children));
  }
  memo.emplace(node.get(), result);
  return result;
}

NodePtr substitute(const NodePtr &node, std::int64_t variable_id,
                   const NodePtr &replacement) {
  std::unordered_map<const Node *, NodePtr> memo;
  return substitute(node, variable_id, replacement, memo);
}

NodePtr rebind(const NodePtr &node, const FunctionImpl &source,
               const FunctionImpl &target,
               std::unordered_map<const Node *, NodePtr> &memo) {
  if (const auto found = memo.find(node.get()); found != memo.end()) {
    return found->second;
  }

  NodePtr result;
  if (node->kind() == NodeKind::Projection ||
      node->kind() == NodeKind::Opaque) {
    const std::string_view name =
        source.trace().variables().get(node->variable_id());
    const auto found = target.binding_index().find(std::string(name));
    PLUSS_CHECK(found != target.binding_index().end(),
                "math::Function: cannot rebind variable '" + std::string(name) +
                    "' onto the target Function");
    const auto variable_id = static_cast<std::int64_t>(found->second);
    result = node->kind() == NodeKind::Projection
                 ? Node::projection(target.trace().id(), variable_id)
                 : Node::opaque(node->opaque_callable(), variable_id,
                                node->opaque_derivative_order());
  } else if (node->kind() == NodeKind::Primitive) {
    std::vector<NodePtr> children;
    children.reserve(node->children().size());
    for (const NodePtr &child : node->children()) {
      children.push_back(rebind(child, source, target, memo));
    }
    result = Node::primitive(node->primitive_id(), std::move(children),
                             node->output(), node->attributes());
  } else if (node->kind() == NodeKind::Integral) {
    const std::string_view name =
        source.trace().variables().get(node->integral_variable_id());
    const auto found = target.binding_index().find(std::string(name));
    PLUSS_CHECK(found != target.binding_index().end(),
                "math::Function: cannot rebind integral variable '" +
                    std::string(name) + "' onto the target Function");
    result = Node::integral(rebind(node->children()[0], source, target, memo),
                            static_cast<std::int64_t>(found->second),
                            node->integral_lower(), node->integral_upper());
  } else {
    result = node;
  }
  memo.emplace(node.get(), result);
  return result;
}

class Integrator final {
public:
  Integrator(std::uint64_t trace_id, std::int64_t variable_id)
      : trace_id_(trace_id), variable_id_(variable_id) {}

  std::optional<NodePtr> apply(const NodePtr &node) {
    if (const auto found = memo_.find(node.get()); found != memo_.end()) {
      return found->second;
    }
    std::optional<NodePtr> result = integrate(node);
    memo_.emplace(node.get(), result);
    return result;
  }

private:
  std::optional<NodePtr> integrate(const NodePtr &node) {
    if (!depends_on(node, variable_id_)) {
      return binary_node(PrimitiveId::Mul, node,
                         Node::projection(trace_id_, variable_id_));
    }
    if (node->kind() == NodeKind::Projection &&
        node->variable_id() == variable_id_) {
      return binary_node(PrimitiveId::Div,
                         binary_node(PrimitiveId::Pow, node, Node::scalar(2.0)),
                         Node::scalar(2.0));
    }
    if (node->kind() != NodeKind::Primitive) {
      return std::nullopt;
    }
    const auto &children = node->children();
    switch (node->primitive_id()) {
    case PrimitiveId::Add:
    case PrimitiveId::Sub: {
      std::optional<NodePtr> left = apply(children[0]);
      std::optional<NodePtr> right = apply(children[1]);
      if (!left || !right) {
        return std::nullopt;
      }
      return binary_node(node->primitive_id(), std::move(*left),
                         std::move(*right));
    }
    case PrimitiveId::Neg: {
      std::optional<NodePtr> value = apply(children[0]);
      return value ? std::optional<NodePtr>(
                         unary_node(PrimitiveId::Neg, std::move(*value)))
                   : std::nullopt;
    }
    case PrimitiveId::Mul: {
      if (!depends_on(children[0], variable_id_)) {
        std::optional<NodePtr> right = apply(children[1]);
        if (right) {
          return binary_node(PrimitiveId::Mul, children[0], std::move(*right));
        }
      }
      if (!depends_on(children[1], variable_id_)) {
        std::optional<NodePtr> left = apply(children[0]);
        if (left) {
          return binary_node(PrimitiveId::Mul, std::move(*left), children[1]);
        }
      }
      return std::nullopt;
    }
    case PrimitiveId::Div: {
      if (!depends_on(children[1], variable_id_)) {
        std::optional<NodePtr> numerator = apply(children[0]);
        if (numerator) {
          return binary_node(PrimitiveId::Div, std::move(*numerator),
                             children[1]);
        }
      }
      if (scalar_is(children[0], 1.0) && is_variable(children[1])) {
        return unary_node(PrimitiveId::Log,
                          unary_node(PrimitiveId::Abs, children[1]));
      }
      return std::nullopt;
    }
    case PrimitiveId::Pow:
      if (is_variable(children[0]) &&
          children[1]->kind() == NodeKind::ScalarConstant) {
        const double exponent = children[1]->scalar_value();
        if (exponent == -1.0) {
          return unary_node(PrimitiveId::Log,
                            unary_node(PrimitiveId::Abs, children[0]));
        }
        return binary_node(PrimitiveId::Div,
                           binary_node(PrimitiveId::Pow, children[0],
                                       Node::scalar(exponent + 1.0)),
                           Node::scalar(exponent + 1.0));
      }
      return std::nullopt;
    case PrimitiveId::Exp:
      return is_variable(children[0]) ? std::optional<NodePtr>(node)
                                      : std::nullopt;
    case PrimitiveId::Expm1:
      if (is_variable(children[0])) {
        return binary_node(PrimitiveId::Sub,
                           unary_node(PrimitiveId::Exp, children[0]),
                           children[0]);
      }
      return std::nullopt;
    case PrimitiveId::Sin:
      return is_variable(children[0])
                 ? std::optional<NodePtr>(
                       unary_node(PrimitiveId::Neg,
                                  unary_node(PrimitiveId::Cos, children[0])))
                 : std::nullopt;
    case PrimitiveId::Cos:
      return is_variable(children[0]) ? std::optional<NodePtr>(unary_node(
                                            PrimitiveId::Sin, children[0]))
                                      : std::nullopt;
    case PrimitiveId::Sinh:
      return is_variable(children[0]) ? std::optional<NodePtr>(unary_node(
                                            PrimitiveId::Cosh, children[0]))
                                      : std::nullopt;
    case PrimitiveId::Cosh:
      return is_variable(children[0]) ? std::optional<NodePtr>(unary_node(
                                            PrimitiveId::Sinh, children[0]))
                                      : std::nullopt;
    default:
      return std::nullopt;
    }
  }

  bool is_variable(const NodePtr &node) const noexcept {
    return node->kind() == NodeKind::Projection &&
           node->variable_id() == variable_id_;
  }

  std::uint64_t trace_id_ = 0;
  std::int64_t variable_id_ = -1;
  std::unordered_map<const Node *, std::optional<NodePtr>> memo_{};
};

bool known_nonzero(const NodePtr &node, const TraceState &trace) {
  if (node->kind() == NodeKind::ScalarConstant) {
    return node->scalar_value() != 0.0;
  }
  if (node->kind() != NodeKind::Projection) {
    return false;
  }
  const Assumption assumption =
      trace.assumptions()[static_cast<std::size_t>(node->variable_id())];
  return assumption == Assumption::NonZero ||
         assumption == Assumption::Positive ||
         assumption == Assumption::Negative;
}

bool known_nonnegative(const NodePtr &node, const TraceState &trace) {
  if (node->kind() == NodeKind::ScalarConstant) {
    return node->scalar_value() >= 0.0;
  }
  if (node->kind() != NodeKind::Projection) {
    return false;
  }
  const Assumption assumption =
      trace.assumptions()[static_cast<std::size_t>(node->variable_id())];
  return assumption == Assumption::Zero ||
         assumption == Assumption::NonNegative ||
         assumption == Assumption::Positive;
}

bool known_nonpositive(const NodePtr &node, const TraceState &trace) {
  if (node->kind() == NodeKind::ScalarConstant) {
    return node->scalar_value() <= 0.0;
  }
  if (node->kind() != NodeKind::Projection) {
    return false;
  }
  const Assumption assumption =
      trace.assumptions()[static_cast<std::size_t>(node->variable_id())];
  return assumption == Assumption::Zero ||
         assumption == Assumption::NonPositive ||
         assumption == Assumption::Negative;
}

Assumption merge_assumption(Assumption current, Assumption incoming) {
  if (current == incoming || incoming == Assumption::Real) {
    return current;
  }
  if (current == Assumption::Real) {
    return incoming;
  }
  if ((current == Assumption::Positive &&
       (incoming == Assumption::NonNegative ||
        incoming == Assumption::NonZero)) ||
      (incoming == Assumption::Positive &&
       (current == Assumption::NonNegative ||
        current == Assumption::NonZero))) {
    return Assumption::Positive;
  }
  if ((current == Assumption::Negative &&
       (incoming == Assumption::NonPositive ||
        incoming == Assumption::NonZero)) ||
      (incoming == Assumption::Negative &&
       (current == Assumption::NonPositive ||
        current == Assumption::NonZero))) {
    return Assumption::Negative;
  }
  if ((current == Assumption::NonNegative && incoming == Assumption::NonZero) ||
      (incoming == Assumption::NonNegative && current == Assumption::NonZero)) {
    return Assumption::Positive;
  }
  if ((current == Assumption::NonPositive && incoming == Assumption::NonZero) ||
      (incoming == Assumption::NonPositive && current == Assumption::NonZero)) {
    return Assumption::Negative;
  }
  if ((current == Assumption::NonNegative &&
       incoming == Assumption::NonPositive) ||
      (incoming == Assumption::NonNegative &&
       current == Assumption::NonPositive)) {
    return Assumption::Zero;
  }
  if (current == Assumption::Zero && (incoming == Assumption::NonNegative ||
                                      incoming == Assumption::NonPositive)) {
    return Assumption::Zero;
  }
  if (incoming == Assumption::Zero && (current == Assumption::NonNegative ||
                                       current == Assumption::NonPositive)) {
    return Assumption::Zero;
  }
  PLUSS_FAIL("math::Function: contradictory variable assumptions");
}

NodePtr simplify_unary(PrimitiveId primitive_id, NodePtr input,
                       const TraceState &trace) {
  if (primitive_id == PrimitiveId::Log &&
      input->kind() == NodeKind::Primitive &&
      input->primitive_id() == PrimitiveId::Exp) {
    return input->children()[0];
  }
  if (primitive_id == PrimitiveId::Abs && known_nonnegative(input, trace)) {
    return input;
  }
  if (primitive_id == PrimitiveId::Abs && known_nonpositive(input, trace)) {
    return unary_node(PrimitiveId::Neg, std::move(input));
  }
  if (primitive_id == PrimitiveId::Sqrt &&
      input->kind() == NodeKind::Primitive &&
      input->primitive_id() == PrimitiveId::Pow &&
      scalar_is(input->children()[1], 2.0) &&
      known_nonnegative(input->children()[0], trace)) {
    return input->children()[0];
  }
  if (primitive_id == PrimitiveId::Sqrt &&
      input->kind() == NodeKind::Primitive &&
      input->primitive_id() == PrimitiveId::Pow &&
      scalar_is(input->children()[1], 2.0) &&
      known_nonpositive(input->children()[0], trace)) {
    return unary_node(PrimitiveId::Neg, input->children()[0]);
  }
  return unary_node(primitive_id, std::move(input));
}

NodePtr simplify_binary(PrimitiveId primitive_id, NodePtr left, NodePtr right,
                        const TraceState &trace) {
  if (primitive_id == PrimitiveId::Div && left.get() == right.get() &&
      known_nonzero(left, trace)) {
    return unary_node(PrimitiveId::OnesLike, left);
  }
  if ((primitive_id == PrimitiveId::Add || primitive_id == PrimitiveId::Sub) &&
      left->kind() == NodeKind::Primitive &&
      right->kind() == NodeKind::Primitive && left.get() != right.get() &&
      left->primitive_id() == PrimitiveId::Mul &&
      right->primitive_id() == PrimitiveId::Mul) {
    if (left->children()[0].get() == right->children()[0].get()) {
      return binary_node(
          PrimitiveId::Mul, left->children()[0],
          binary_node(primitive_id, left->children()[1], right->children()[1]));
    }
    if (left->children()[1].get() == right->children()[1].get()) {
      return binary_node(
          PrimitiveId::Mul,
          binary_node(primitive_id, left->children()[0], right->children()[0]),
          left->children()[1]);
    }
  }
  return binary_node(primitive_id, std::move(left), std::move(right));
}

enum class Parity : std::uint8_t { Unknown, Even, Odd };

Parity parity(const NodePtr &node, std::int64_t variable_id,
              std::unordered_map<const Node *, Parity> &memo) {
  if (const auto found = memo.find(node.get()); found != memo.end()) {
    return found->second;
  }
  Parity result = Parity::Unknown;
  if (node->kind() == NodeKind::Projection) {
    result = node->variable_id() == variable_id ? Parity::Odd : Parity::Even;
  } else if (node->kind() == NodeKind::ScalarConstant ||
             node->kind() == NodeKind::SeriesConstant) {
    result = Parity::Even;
  } else if (node->kind() == NodeKind::Primitive) {
    const auto &children = node->children();
    const Parity first = parity(children[0], variable_id, memo);
    switch (node->primitive_id()) {
    case PrimitiveId::Neg:
    case PrimitiveId::Conj:
    case PrimitiveId::Real:
    case PrimitiveId::Imag:
    case PrimitiveId::AbsSlopeStrict:
    case PrimitiveId::AbsSlopeZero:
    case PrimitiveId::AbsSlopePiecewise:
      result = first;
      break;
    case PrimitiveId::Abs:
    case PrimitiveId::Cos:
    case PrimitiveId::Cosh:
      result = first == Parity::Odd ? Parity::Even : Parity::Unknown;
      break;
    case PrimitiveId::Sin:
    case PrimitiveId::Sinh:
    case PrimitiveId::Tan:
    case PrimitiveId::Tanh:
      result = first == Parity::Odd ? Parity::Odd : Parity::Unknown;
      break;
    case PrimitiveId::Exp:
    case PrimitiveId::Expm1:
      result = first == Parity::Even ? Parity::Even : Parity::Unknown;
      break;
    case PrimitiveId::Add:
    case PrimitiveId::Sub: {
      const Parity second = parity(children[1], variable_id, memo);
      result = first == second ? first : Parity::Unknown;
      break;
    }
    case PrimitiveId::Mul:
    case PrimitiveId::Div:
    case PrimitiveId::SafeDivStrict:
    case PrimitiveId::SafeDivZero:
    case PrimitiveId::SafeDivPiecewise: {
      const Parity second = parity(children[1], variable_id, memo);
      if (first != Parity::Unknown && second != Parity::Unknown) {
        result = first == second ? Parity::Even : Parity::Odd;
      }
      break;
    }
    case PrimitiveId::Pow:
      if (children[1]->kind() == NodeKind::ScalarConstant) {
        const double exponent = children[1]->scalar_value();
        if (std::floor(exponent) == exponent) {
          result = static_cast<std::int64_t>(std::abs(exponent)) % 2 == 0
                       ? Parity::Even
                       : first;
        }
      }
      break;
    case PrimitiveId::Log:
    case PrimitiveId::Log1p:
    case PrimitiveId::Sqrt:
    case PrimitiveId::ZerosLike:
    case PrimitiveId::OnesLike:
    case PrimitiveId::Stack:
    case PrimitiveId::Transpose:
    case PrimitiveId::Contract:
    case PrimitiveId::Inverse:
    case PrimitiveId::Det:
    case PrimitiveId::Trace:
    case PrimitiveId::Select:
    case PrimitiveId::PathIntegral:
    case PrimitiveId::PathCovariation:
    case PrimitiveId::Angle:
      break;
    }
  }
  memo.emplace(node.get(), result);
  return result;
}

Parity parity(const NodePtr &node, std::int64_t variable_id) {
  std::unordered_map<const Node *, Parity> memo;
  return parity(node, variable_id, memo);
}

NodePtr contextual_zero(const NodePtr &node, std::int64_t excluded_variable) {
  std::vector<NodePtr> pending{node};
  while (!pending.empty()) {
    NodePtr current = std::move(pending.back());
    pending.pop_back();
    if (current->kind() == NodeKind::Projection &&
        current->variable_id() != excluded_variable) {
      return unary_node(PrimitiveId::ZerosLike, std::move(current));
    }
    for (const NodePtr &child : current->children()) {
      pending.push_back(child);
    }
  }
  return Node::scalar(0.0);
}

Provenance provenance(const NodePtr &root) {
  std::vector<const Node *> pending{root.get()};
  std::unordered_set<const Node *> seen;
  bool opaque = false;
  bool quadrature = false;
  while (!pending.empty()) {
    const Node *node = pending.back();
    pending.pop_back();
    if (!seen.emplace(node).second) {
      continue;
    }
    if (node->kind() == NodeKind::Opaque) {
      if (node->opaque_derivative_order() > 0) {
        return Provenance::Autodiff;
      }
      opaque = true;
    } else if (node->kind() == NodeKind::Integral) {
      quadrature = true;
    }
    for (const NodePtr &child : node->children()) {
      pending.push_back(child.get());
    }
  }
  if (quadrature) {
    return Provenance::Quadrature;
  }
  return opaque ? Provenance::Opaque : Provenance::Structural;
}

class Simplifier final {
public:
  explicit Simplifier(const TraceState &trace) : trace_(trace) {}

  NodePtr apply(const NodePtr &node) {
    if (const auto found = memo_.find(node.get()); found != memo_.end()) {
      return found->second;
    }
    NodePtr result = node;
    if (node->kind() == NodeKind::Primitive ||
        node->kind() == NodeKind::Integral) {
      std::vector<NodePtr> children;
      children.reserve(node->children().size());
      for (const NodePtr &child : node->children()) {
        children.push_back(apply(child));
      }
      if (node->kind() == NodeKind::Integral) {
        result =
            Node::integral(std::move(children[0]), node->integral_variable_id(),
                           node->integral_lower(), node->integral_upper());
      } else if (node->primitive_id() == PrimitiveId::Stack) {
        result = stack_nodes(std::move(children));
      } else if (carries_attributes(node->primitive_id())) {
        result = tensor_node(node->primitive_id(), std::move(children),
                             node->attributes());
      } else if (children.size() == 1) {
        result = simplify_unary(node->primitive_id(), std::move(children[0]),
                                trace_);
      } else if (children.size() == 2) {
        result = simplify_binary(node->primitive_id(), std::move(children[0]),
                                 std::move(children[1]), trace_);
      }
    }
    memo_.emplace(node.get(), result);
    return result;
  }

private:
  const TraceState &trace_;
  std::unordered_map<const Node *, NodePtr> memo_{};
};

void require_graph_limit(const NodePtr &root, std::size_t max_nodes,
                         std::string_view operation) {
  PLUSS_CHECK(max_nodes > 0,
              std::string(operation) + ": max_nodes must be positive");
  std::vector<const Node *> pending{root.get()};
  std::unordered_set<const Node *> seen;
  seen.reserve(std::min<std::size_t>(max_nodes, 4096));
  while (!pending.empty()) {
    const Node *node = pending.back();
    pending.pop_back();
    if (!seen.emplace(node).second) {
      continue;
    }
    PLUSS_CHECK(seen.size() <= max_nodes,
                std::string(operation) +
                    ": transformed graph exceeds max_nodes safeguard");
    for (const NodePtr &child : node->children()) {
      pending.push_back(child.get());
    }
  }
}

struct DerivativeKey final {
  const Node *node = nullptr;
  std::int64_t variable_id = -1;
  std::int64_t order = 0;

  bool operator==(const DerivativeKey &) const noexcept = default;
};

struct DerivativeKeyHash final {
  std::size_t operator()(const DerivativeKey &key) const noexcept {
    std::size_t seed = std::hash<const Node *>{}(key.node);
    seed ^= std::hash<std::int64_t>{}(key.variable_id) + 0x9e3779b9U +
            (seed << 6U) + (seed >> 2U);
    seed ^= std::hash<std::int64_t>{}(key.order) + 0x9e3779b9U + (seed << 6U) +
            (seed >> 2U);
    return seed;
  }
};

class Differentiator final {
public:
  // `max_nodes` bounds every intermediate derivative order, not only the
  // finished graph, so a request for a very high order is rejected before the
  // exponentially growing graph has been built.
  Differentiator(NonSmooth nonsmooth, Fallback fallback,
                 std::size_t max_nodes = std::numeric_limits<std::size_t>::max())
      : nonsmooth_(nonsmooth), fallback_(fallback), max_nodes_(max_nodes) {}

  NodePtr apply(const NodePtr &node, std::int64_t variable_id,
                std::int64_t order) {
    const DerivativeKey key{node.get(), variable_id, order};
    if (const auto found = memo_.find(key); found != memo_.end()) {
      return found->second;
    }

    NodePtr result;
    if (order == 0) {
      result = node;
    } else if (order > 1) {
      const NodePtr lower = apply(node, variable_id, order - 1);
      require_graph_limit(lower, max_nodes_, "operators::differential");
      result = apply(lower, variable_id, 1);
    } else {
      result = first(node, variable_id);
    }
    memo_.emplace(key, result);
    return result;
  }

private:
  NodePtr first(const NodePtr &node, std::int64_t variable_id) {
    switch (node->kind()) {
    case NodeKind::Projection:
      return unary_node(node->variable_id() == variable_id
                            ? PrimitiveId::OnesLike
                            : PrimitiveId::ZerosLike,
                        node);
    case NodeKind::ScalarConstant:
    case NodeKind::SeriesConstant:
      return unary_node(PrimitiveId::ZerosLike, node);
    case NodeKind::Opaque:
      PLUSS_CHECK(fallback_ == Fallback::Autodiff,
                  "operators::differential: opaque Function requires explicit "
                  "DiffOptions{.fallback = math::Fallback::Autodiff}");
      PLUSS_CHECK(node->variable_id() == variable_id,
                  "operators::differential: opaque Function variable "
                  "binding mismatch");
      return Node::opaque(node->opaque_callable(), variable_id,
                          node->opaque_derivative_order() + 1);
    case NodeKind::Integral:
      if (node->integral_variable_id() == variable_id) {
        if (!node->integral_upper().has_value()) {
          return node->children()[0];
        }
        return unary_node(PrimitiveId::ZerosLike, node);
      }
      return Node::integral(apply(node->children()[0], variable_id, 1),
                            node->integral_variable_id(),
                            node->integral_lower(), node->integral_upper());
    case NodeKind::Primitive:
      break;
    }

    const auto &children = node->children();
    if (node->primitive_id() == PrimitiveId::Stack) {
      std::vector<NodePtr> derivatives;
      derivatives.reserve(children.size());
      for (const NodePtr &child : children) {
        derivatives.push_back(apply(child, variable_id, 1));
      }
      return stack_nodes(std::move(derivatives));
    }
    const NodePtr &u = children[0];
    const NodePtr du = apply(u, variable_id, 1);
    switch (node->primitive_id()) {
    case PrimitiveId::Add:
      return binary_node(PrimitiveId::Add, du,
                         apply(children[1], variable_id, 1));
    case PrimitiveId::Sub:
      return binary_node(PrimitiveId::Sub, du,
                         apply(children[1], variable_id, 1));
    case PrimitiveId::Mul:
      return binary_node(
          PrimitiveId::Add, binary_node(PrimitiveId::Mul, du, children[1]),
          binary_node(PrimitiveId::Mul, u, apply(children[1], variable_id, 1)));
    case PrimitiveId::Div: {
      const NodePtr &v = children[1];
      const NodePtr numerator = binary_node(
          PrimitiveId::Sub, binary_node(PrimitiveId::Mul, du, v),
          binary_node(PrimitiveId::Mul, u, apply(v, variable_id, 1)));
      return binary_node(PrimitiveId::Div, numerator,
                         binary_node(PrimitiveId::Pow, v, Node::scalar(2.0)));
    }
    case PrimitiveId::Pow: {
      const NodePtr &v = children[1];
      const NodePtr dv = apply(v, variable_id, 1);
      if (v->kind() == NodeKind::ScalarConstant) {
        const double exponent = v->scalar_value();
        return binary_node(
            PrimitiveId::Mul,
            binary_node(
                PrimitiveId::Mul, Node::scalar(exponent),
                binary_node(PrimitiveId::Pow, u, Node::scalar(exponent - 1.0))),
            du);
      }
      if (u->kind() == NodeKind::ScalarConstant) {
        return binary_node(
            PrimitiveId::Mul,
            binary_node(PrimitiveId::Mul, node,
                        Node::scalar(std::log(u->scalar_value()))),
            dv);
      }
      const NodePtr logarithmic = binary_node(
          PrimitiveId::Add,
          binary_node(PrimitiveId::Mul, dv, unary_node(PrimitiveId::Log, u)),
          binary_node(PrimitiveId::Mul, v,
                      binary_node(PrimitiveId::Div, du, u)));
      return binary_node(PrimitiveId::Mul, node, logarithmic);
    }
    case PrimitiveId::Neg:
      return unary_node(PrimitiveId::Neg, du);
    case PrimitiveId::Exp:
      return binary_node(PrimitiveId::Mul, node, du);
    case PrimitiveId::Log:
      return binary_node(PrimitiveId::Div, du, u);
    case PrimitiveId::Log1p:
      return binary_node(PrimitiveId::Div, du,
                         binary_node(PrimitiveId::Add, Node::scalar(1.0), u));
    case PrimitiveId::Expm1:
      return binary_node(PrimitiveId::Mul, unary_node(PrimitiveId::Exp, u), du);
    case PrimitiveId::Sqrt:
      return binary_node(
          PrimitiveId::Div, du,
          binary_node(PrimitiveId::Mul, Node::scalar(2.0), node));
    case PrimitiveId::Sin:
      return binary_node(PrimitiveId::Mul, unary_node(PrimitiveId::Cos, u), du);
    case PrimitiveId::Cos:
      return binary_node(
          PrimitiveId::Mul,
          unary_node(PrimitiveId::Neg, unary_node(PrimitiveId::Sin, u)), du);
    case PrimitiveId::Tan:
      return binary_node(PrimitiveId::Div, du,
                         binary_node(PrimitiveId::Pow,
                                     unary_node(PrimitiveId::Cos, u),
                                     Node::scalar(2.0)));
    case PrimitiveId::Sinh:
      return binary_node(PrimitiveId::Mul, unary_node(PrimitiveId::Cosh, u),
                         du);
    case PrimitiveId::Cosh:
      return binary_node(PrimitiveId::Mul, unary_node(PrimitiveId::Sinh, u),
                         du);
    case PrimitiveId::Tanh:
      return binary_node(PrimitiveId::Div, du,
                         binary_node(PrimitiveId::Pow,
                                     unary_node(PrimitiveId::Cosh, u),
                                     Node::scalar(2.0)));
    case PrimitiveId::Conj:
      return unary_node(PrimitiveId::Conj, du);
    case PrimitiveId::Real:
      return unary_node(PrimitiveId::Real, du);
    case PrimitiveId::Imag:
      return unary_node(PrimitiveId::Imag, du);
    case PrimitiveId::Abs: {
      // Directional derivative along a real parameter. This formula is valid
      // for both real and complex-valued Functions.
      const NodePtr numerator = unary_node(
          PrimitiveId::Real,
          binary_node(PrimitiveId::Mul, unary_node(PrimitiveId::Conj, u), du));
      return safe_divide(numerator, node, nonsmooth_);
    }
    case PrimitiveId::Angle: {
      const NodePtr numerator = unary_node(
          PrimitiveId::Imag,
          binary_node(PrimitiveId::Mul, unary_node(PrimitiveId::Conj, u), du));
      const NodePtr denominator =
          binary_node(PrimitiveId::Pow, unary_node(PrimitiveId::Abs, u),
                      Node::scalar(2.0));
      return safe_divide(numerator, denominator, nonsmooth_);
    }
    case PrimitiveId::ZerosLike:
    case PrimitiveId::OnesLike:
    case PrimitiveId::AbsSlopeStrict:
    case PrimitiveId::AbsSlopeZero:
    case PrimitiveId::AbsSlopePiecewise:
      return unary_node(PrimitiveId::ZerosLike, node);
    case PrimitiveId::SafeDivStrict:
    case PrimitiveId::SafeDivZero:
    case PrimitiveId::SafeDivPiecewise: {
      const NodePtr &v = children[1];
      const NodePtr dv = apply(v, variable_id, 1);
      const NodePtr numerator = binary_node(
          PrimitiveId::Sub, binary_node(PrimitiveId::Mul, du, v),
          binary_node(PrimitiveId::Mul, u, dv));
      return safe_divide(
          numerator,
          binary_node(PrimitiveId::Pow, v, Node::scalar(2.0)), nonsmooth_);
    }
    case PrimitiveId::Stack:
      PLUSS_FAIL("operators::differential: unreachable stack derivative");
    case PrimitiveId::Transpose:
    case PrimitiveId::Trace:
    case PrimitiveId::Select:
      return tensor_node(node->primitive_id(), {du}, node->attributes());
    case PrimitiveId::Contract: {
      const NodePtr dv = apply(children[1], variable_id, 1);
      return binary_node(
          PrimitiveId::Add,
          tensor_node(PrimitiveId::Contract, {du, children[1]},
                      node->attributes()),
          tensor_node(PrimitiveId::Contract, {children[0], dv},
                      node->attributes()));
    }
    case PrimitiveId::PathIntegral: {
      // Pathwise exchange d/dv int F dW = int (dF/dv) dW for every variable
      // except the noise itself, whose derivative is the Malliavin derivative.
      PLUSS_CHECK(children[1]->kind() == NodeKind::Projection &&
                      children[1]->variable_id() != variable_id,
                  "operators::differential: a stochastic integral cannot be "
                  "differentiated with respect to its own noise path (that "
                  "is the Malliavin derivative, not an ordinary derivative)");
      return tensor_node(PrimitiveId::PathIntegral, {du, children[1]},
                         node->attributes());
    }
    case PrimitiveId::PathCovariation: {
      const NodePtr dv = apply(children[1], variable_id, 1);
      return binary_node(
          PrimitiveId::Add,
          tensor_node(PrimitiveId::PathCovariation, {du, children[1]},
                      node->attributes()),
          tensor_node(PrimitiveId::PathCovariation, {children[0], dv},
                      node->attributes()));
    }
    case PrimitiveId::Inverse: {
      // d(A^-1) = -A^-1 dA A^-1
      const NodePtr left = matmul_node(node, du);
      return unary_node(PrimitiveId::Neg, matmul_node(left, node));
    }
    case PrimitiveId::Det: {
      // d det(A) = det(A) tr(A^-1 dA)
      const NodePtr inverse = tensor_node(PrimitiveId::Inverse, {u}, {});
      return binary_node(
          PrimitiveId::Mul, node,
          tensor_node(PrimitiveId::Trace, {matmul_node(inverse, du)}, {}));
    }
    }
    PLUSS_FAIL("operators::differential: missing primitive derivative rule");
  }

  std::unordered_map<DerivativeKey, NodePtr, DerivativeKeyHash> memo_{};
  NonSmooth nonsmooth_ = NonSmooth::Strict;
  Fallback fallback_ = Fallback::Reject;
  std::size_t max_nodes_ = std::numeric_limits<std::size_t>::max();
};

struct WirtingerPair final {
  NodePtr z;
  NodePtr conjugate;
};

class WirtingerDifferentiator final {
public:
  WirtingerDifferentiator(std::int64_t variable_id, NonSmooth nonsmooth)
      : variable_id_(variable_id), nonsmooth_(nonsmooth) {}

  WirtingerPair apply(const NodePtr &node) {
    if (const auto found = memo_.find(node.get()); found != memo_.end()) {
      return found->second;
    }
    WirtingerPair result = first(node);
    memo_.emplace(node.get(), result);
    return result;
  }

private:
  WirtingerPair zero(const NodePtr &node) const {
    const NodePtr value = unary_node(PrimitiveId::ZerosLike, node);
    return {value, value};
  }

  NodePtr quotient(NodePtr numerator, NodePtr denominator) const {
    return safe_divide(std::move(numerator), std::move(denominator),
                       nonsmooth_);
  }

  WirtingerPair chain(const NodePtr &factor,
                      const WirtingerPair &inner) const {
    return {binary_node(PrimitiveId::Mul, factor, inner.z),
            binary_node(PrimitiveId::Mul, factor, inner.conjugate)};
  }

  WirtingerPair first(const NodePtr &node) {
    switch (node->kind()) {
    case NodeKind::Projection: {
      const NodePtr one = unary_node(PrimitiveId::OnesLike, node);
      const NodePtr zero_value = unary_node(PrimitiveId::ZerosLike, node);
      return node->variable_id() == variable_id_
                 ? WirtingerPair{one, zero_value}
                 : WirtingerPair{zero_value, zero_value};
    }
    case NodeKind::ScalarConstant:
    case NodeKind::SeriesConstant:
      return zero(node);
    case NodeKind::Opaque:
      PLUSS_FAIL("operators::wirtinger: opaque Functions have no structural "
                 "complex derivative");
    case NodeKind::Integral: {
      if (node->integral_variable_id() == variable_id_) {
        PLUSS_FAIL("operators::wirtinger: integration and differentiation "
                   "cannot use the same complex variable");
      }
      const WirtingerPair inner = apply(node->children()[0]);
      return {Node::integral(inner.z, node->integral_variable_id(),
                             node->integral_lower(), node->integral_upper()),
              Node::integral(inner.conjugate, node->integral_variable_id(),
                             node->integral_lower(), node->integral_upper())};
    }
    case NodeKind::Primitive:
      break;
    }

    const auto &children = node->children();
    if (node->primitive_id() == PrimitiveId::Stack) {
      std::vector<NodePtr> z;
      std::vector<NodePtr> conjugate;
      z.reserve(children.size());
      conjugate.reserve(children.size());
      for (const NodePtr &child : children) {
        const WirtingerPair derivative = apply(child);
        z.push_back(derivative.z);
        conjugate.push_back(derivative.conjugate);
      }
      return {stack_nodes(std::move(z)), stack_nodes(std::move(conjugate))};
    }

    const NodePtr &u = children[0];
    const WirtingerPair du = apply(u);
    switch (node->primitive_id()) {
    case PrimitiveId::Add:
    case PrimitiveId::Sub: {
      const WirtingerPair dv = apply(children[1]);
      return {binary_node(node->primitive_id(), du.z, dv.z),
              binary_node(node->primitive_id(), du.conjugate, dv.conjugate)};
    }
    case PrimitiveId::Mul: {
      const NodePtr &v = children[1];
      const WirtingerPair dv = apply(v);
      return {binary_node(
                  PrimitiveId::Add,
                  binary_node(PrimitiveId::Mul, du.z, v),
                  binary_node(PrimitiveId::Mul, u, dv.z)),
              binary_node(
                  PrimitiveId::Add,
                  binary_node(PrimitiveId::Mul, du.conjugate, v),
                  binary_node(PrimitiveId::Mul, u, dv.conjugate))};
    }
    case PrimitiveId::Div:
    case PrimitiveId::SafeDivStrict:
    case PrimitiveId::SafeDivZero:
    case PrimitiveId::SafeDivPiecewise: {
      const NodePtr &v = children[1];
      const WirtingerPair dv = apply(v);
      const NodePtr denominator =
          binary_node(PrimitiveId::Pow, v, Node::scalar(2.0));
      return {
          quotient(binary_node(
                       PrimitiveId::Sub,
                       binary_node(PrimitiveId::Mul, du.z, v),
                       binary_node(PrimitiveId::Mul, u, dv.z)),
                   denominator),
          quotient(binary_node(
                       PrimitiveId::Sub,
                       binary_node(PrimitiveId::Mul, du.conjugate, v),
                       binary_node(PrimitiveId::Mul, u, dv.conjugate)),
                   denominator)};
    }
    case PrimitiveId::Pow: {
      const NodePtr &v = children[1];
      const WirtingerPair dv = apply(v);
      const auto component = [&](const NodePtr &a, const NodePtr &b) {
        return binary_node(
            PrimitiveId::Mul, node,
            binary_node(
                PrimitiveId::Add,
                binary_node(PrimitiveId::Mul, b,
                            unary_node(PrimitiveId::Log, u)),
                binary_node(PrimitiveId::Mul, v,
                            binary_node(PrimitiveId::Div, a, u))));
      };
      return {component(du.z, dv.z),
              component(du.conjugate, dv.conjugate)};
    }
    case PrimitiveId::Neg:
      return {unary_node(PrimitiveId::Neg, du.z),
              unary_node(PrimitiveId::Neg, du.conjugate)};
    case PrimitiveId::Exp:
      return chain(node, du);
    case PrimitiveId::Log:
      return {binary_node(PrimitiveId::Div, du.z, u),
              binary_node(PrimitiveId::Div, du.conjugate, u)};
    case PrimitiveId::Log1p: {
      const NodePtr denominator =
          binary_node(PrimitiveId::Add, Node::scalar(1.0), u);
      return {binary_node(PrimitiveId::Div, du.z, denominator),
              binary_node(PrimitiveId::Div, du.conjugate, denominator)};
    }
    case PrimitiveId::Expm1:
      return chain(unary_node(PrimitiveId::Exp, u), du);
    case PrimitiveId::Sqrt: {
      const NodePtr denominator = binary_node(
          PrimitiveId::Mul, Node::scalar(2.0), node);
      return {binary_node(PrimitiveId::Div, du.z, denominator),
              binary_node(PrimitiveId::Div, du.conjugate, denominator)};
    }
    case PrimitiveId::Sin:
      return chain(unary_node(PrimitiveId::Cos, u), du);
    case PrimitiveId::Cos:
      return chain(unary_node(
                       PrimitiveId::Neg,
                       unary_node(PrimitiveId::Sin, u)),
                   du);
    case PrimitiveId::Tan: {
      const NodePtr denominator = binary_node(
          PrimitiveId::Pow, unary_node(PrimitiveId::Cos, u),
          Node::scalar(2.0));
      return {binary_node(PrimitiveId::Div, du.z, denominator),
              binary_node(PrimitiveId::Div, du.conjugate, denominator)};
    }
    case PrimitiveId::Sinh:
      return chain(unary_node(PrimitiveId::Cosh, u), du);
    case PrimitiveId::Cosh:
      return chain(unary_node(PrimitiveId::Sinh, u), du);
    case PrimitiveId::Tanh: {
      const NodePtr denominator = binary_node(
          PrimitiveId::Pow, unary_node(PrimitiveId::Cosh, u),
          Node::scalar(2.0));
      return {binary_node(PrimitiveId::Div, du.z, denominator),
              binary_node(PrimitiveId::Div, du.conjugate, denominator)};
    }
    case PrimitiveId::Conj:
      return {unary_node(PrimitiveId::Conj, du.conjugate),
              unary_node(PrimitiveId::Conj, du.z)};
    case PrimitiveId::Real: {
      const NodePtr half = Node::scalar(0.5);
      return {
          binary_node(
              PrimitiveId::Mul, half,
              binary_node(PrimitiveId::Add, du.z,
                          unary_node(PrimitiveId::Conj, du.conjugate))),
          binary_node(
              PrimitiveId::Mul, half,
              binary_node(PrimitiveId::Add, du.conjugate,
                          unary_node(PrimitiveId::Conj, du.z)))};
    }
    case PrimitiveId::Imag: {
      const NodePtr two_i = complex_scalar(0.0, 2.0);
      return {
          binary_node(
              PrimitiveId::Div,
              binary_node(PrimitiveId::Sub, du.z,
                          unary_node(PrimitiveId::Conj, du.conjugate)),
              two_i),
          binary_node(
              PrimitiveId::Div,
              binary_node(PrimitiveId::Sub, du.conjugate,
                          unary_node(PrimitiveId::Conj, du.z)),
              two_i)};
    }
    case PrimitiveId::Abs: {
      const NodePtr conjugated = unary_node(PrimitiveId::Conj, u);
      const NodePtr denominator =
          binary_node(PrimitiveId::Mul, Node::scalar(2.0), node);
      return {
          quotient(binary_node(
                       PrimitiveId::Add,
                       binary_node(PrimitiveId::Mul, conjugated, du.z),
                       binary_node(PrimitiveId::Mul, u,
                                   unary_node(PrimitiveId::Conj,
                                              du.conjugate))),
                   denominator),
          quotient(binary_node(
                       PrimitiveId::Add,
                       binary_node(PrimitiveId::Mul, conjugated,
                                   du.conjugate),
                       binary_node(PrimitiveId::Mul, u,
                                   unary_node(PrimitiveId::Conj, du.z))),
                   denominator)};
    }
    case PrimitiveId::Angle: {
      const NodePtr conjugated = unary_node(PrimitiveId::Conj, u);
      const NodePtr two_i = complex_scalar(0.0, 2.0);
      return {
          binary_node(
              PrimitiveId::Div,
              binary_node(
                  PrimitiveId::Sub,
                  quotient(du.z, u),
                  quotient(unary_node(PrimitiveId::Conj, du.conjugate),
                           conjugated)),
              two_i),
          binary_node(
              PrimitiveId::Div,
              binary_node(
                  PrimitiveId::Sub,
                  quotient(du.conjugate, u),
                  quotient(unary_node(PrimitiveId::Conj, du.z), conjugated)),
              two_i)};
    }
    case PrimitiveId::ZerosLike:
    case PrimitiveId::OnesLike:
      return zero(node);
    case PrimitiveId::AbsSlopeStrict:
    case PrimitiveId::AbsSlopeZero:
    case PrimitiveId::AbsSlopePiecewise:
      PLUSS_FAIL("operators::wirtinger: internal real non-smooth primitive "
                 "cannot enter complex differentiation");
    case PrimitiveId::Transpose:
    case PrimitiveId::Trace:
    case PrimitiveId::Select:
      return {tensor_node(node->primitive_id(), {du.z}, node->attributes()),
              tensor_node(node->primitive_id(), {du.conjugate},
                          node->attributes())};
    case PrimitiveId::Contract: {
      const WirtingerPair dv = apply(children[1]);
      const auto component = [&](const NodePtr &a, const NodePtr &b) {
        return binary_node(
            PrimitiveId::Add,
            tensor_node(PrimitiveId::Contract, {a, children[1]},
                        node->attributes()),
            tensor_node(PrimitiveId::Contract, {children[0], b},
                        node->attributes()));
      };
      return {component(du.z, dv.z),
              component(du.conjugate, dv.conjugate)};
    }
    case PrimitiveId::PathIntegral: {
      PLUSS_CHECK(children[1]->kind() == NodeKind::Projection &&
                      children[1]->variable_id() != variable_id_,
                  "operators::wirtinger: cannot differentiate a stochastic "
                  "integral with respect to its noise path");
      return {tensor_node(PrimitiveId::PathIntegral, {du.z, children[1]},
                          node->attributes()),
              tensor_node(PrimitiveId::PathIntegral,
                          {du.conjugate, children[1]}, node->attributes())};
    }
    case PrimitiveId::PathCovariation: {
      const WirtingerPair dv = apply(children[1]);
      const auto component = [&](const NodePtr &a, const NodePtr &b) {
        return binary_node(
            PrimitiveId::Add,
            tensor_node(PrimitiveId::PathCovariation, {a, children[1]},
                        node->attributes()),
            tensor_node(PrimitiveId::PathCovariation, {children[0], b},
                        node->attributes()));
      };
      return {component(du.z, dv.z),
              component(du.conjugate, dv.conjugate)};
    }
    case PrimitiveId::Inverse: {
      const auto component = [&](const NodePtr &value) {
        return unary_node(
            PrimitiveId::Neg,
            matmul_node(matmul_node(node, value), node));
      };
      return {component(du.z), component(du.conjugate)};
    }
    case PrimitiveId::Det: {
      const NodePtr inverse =
          tensor_node(PrimitiveId::Inverse, {u}, {});
      const auto component = [&](const NodePtr &value) {
        return binary_node(
            PrimitiveId::Mul, node,
            tensor_node(PrimitiveId::Trace,
                        {matmul_node(inverse, value)}, {}));
      };
      return {component(du.z), component(du.conjugate)};
    }
    case PrimitiveId::Stack:
      PLUSS_FAIL("operators::wirtinger: unreachable stack derivative");
    }
    PLUSS_FAIL("operators::wirtinger: missing primitive derivative rule");
  }

  std::int64_t variable_id_ = -1;
  NonSmooth nonsmooth_ = NonSmooth::Strict;
  std::unordered_map<const Node *, WirtingerPair> memo_{};
};

std::vector<std::int64_t>
resolve_variables(const FunctionImpl &function,
                  pluss::RangeView<std::string_view> variables,
                  std::string_view operation) {
  std::vector<std::int64_t> resolved;
  if (variables.empty()) {
    resolved.reserve(function.trace().variables().size());
    for (std::size_t i = 0; i < function.trace().variables().size(); ++i) {
      resolved.push_back(static_cast<std::int64_t>(i));
    }
  } else {
    resolved.reserve(variables.size());
    std::unordered_set<std::int64_t> seen;
    seen.reserve(variables.size());
    for (const std::string_view variable : variables) {
      PLUSS_CHECK(!variable.empty(), std::string(operation) +
                                         ": variable names must not be empty");
      const auto found = function.binding_index().find(std::string(variable));
      PLUSS_CHECK(found != function.binding_index().end(),
                  std::string(operation) + ": unknown variable '" +
                      std::string(variable) + "'");
      const auto id = static_cast<std::int64_t>(found->second);
      PLUSS_CHECK(seen.emplace(id).second, std::string(operation) +
                                               ": duplicate variable '" +
                                               std::string(variable) + "'");
      resolved.push_back(id);
    }
  }
  PLUSS_CHECK(!resolved.empty(),
              std::string(operation) + ": at least one variable is required");
  return resolved;
}

void merge_bound_variables(std::vector<std::int64_t> &target,
                           const std::vector<std::int64_t> &source) {
  for (const std::int64_t variable : source) {
    if (std::find(target.begin(), target.end(), variable) == target.end()) {
      target.push_back(variable);
    }
  }
}

} // namespace

std::uint64_t fresh_trace_id() noexcept {
  return next_trace_id.fetch_add(1, std::memory_order_relaxed);
}

TraceState::TraceState(std::uint64_t id,
                       pluss::msharp::Lister::StringView variables,
                       std::vector<Assumption> assumptions)
    : id_(id), variables_(std::move(variables)),
      assumptions_(std::move(assumptions)) {
  if (assumptions_.empty()) {
    assumptions_.assign(variables_.size(), Assumption::Real);
  }
  PLUSS_CHECK(assumptions_.size() == variables_.size(),
              "math::Function: assumption count must match variable count");
}

pluss::msharp::intrusive_ptr<Node> Node::projection(std::uint64_t trace_id,
                                                    std::int64_t variable_id) {
  return pluss::msharp::intrusive_ptr<Node>(
      pluss::msharp::intrusive_ptr<Node>::adopt,
      new Node(NodeKind::Projection, trace_id, variable_id, 0.0,
               pluss::Series{}, PrimitiveId::Add, {}, OutputMetadata{}));
}

pluss::msharp::intrusive_ptr<Node> Node::scalar(double value) {
  return pluss::msharp::intrusive_ptr<Node>(
      pluss::msharp::intrusive_ptr<Node>::adopt,
      new Node(NodeKind::ScalarConstant, 0, -1, value, pluss::Series{},
               PrimitiveId::Add, {}, OutputMetadata{}));
}

pluss::msharp::intrusive_ptr<Node> Node::series(pluss::Series value) {
  PLUSS_CHECK(value.defined(),
              "math::function: Series constant must be defined");
  OutputMetadata output;
  const auto sizes = value.sizes();
  output.shape.assign(sizes.begin(), sizes.end());
  return pluss::msharp::intrusive_ptr<Node>(
      pluss::msharp::intrusive_ptr<Node>::adopt,
      new Node(NodeKind::SeriesConstant, 0, -1, 0.0, std::move(value),
               PrimitiveId::Add, {}, std::move(output)));
}

pluss::msharp::intrusive_ptr<Node>
Node::opaque(std::function<pluss::Series(const pluss::Series &)> callable,
             std::int64_t variable_id, std::int64_t derivative_order) {
  PLUSS_CHECK(callable, "math::function: opaque Series callable is required");
  PLUSS_CHECK(variable_id >= 0,
              "math::function: opaque variable binding is invalid");
  PLUSS_CHECK(derivative_order >= 0,
              "math::function: opaque derivative order is invalid");
  return pluss::msharp::intrusive_ptr<Node>(
      pluss::msharp::intrusive_ptr<Node>::adopt,
      new Node(NodeKind::Opaque, 0, variable_id, 0.0, pluss::Series{},
               PrimitiveId::Add, {}, OutputMetadata{}, -1, 0.0, std::nullopt,
               std::move(callable), derivative_order));
}

pluss::msharp::intrusive_ptr<Node>
Node::primitive(PrimitiveId primitive_id,
                std::vector<pluss::msharp::intrusive_ptr<Node>> children,
                OutputMetadata output, std::vector<std::int64_t> attributes) {
  const PrimitiveSpec &spec = detail::primitive(primitive_id);
  PLUSS_CHECK(spec.variadic ? !children.empty() : children.size() == spec.arity,
              "math::Function: primitive graph arity mismatch");
  PLUSS_CHECK(carries_attributes(primitive_id) || attributes.empty(),
              "math::Function: only tensor primitives carry attributes");
  auto node = pluss::msharp::intrusive_ptr<Node>(
      pluss::msharp::intrusive_ptr<Node>::adopt,
      new Node(NodeKind::Primitive, 0, -1, 0.0, pluss::Series{}, primitive_id,
               std::move(children), std::move(output)));
  node->attributes_ = std::move(attributes);
  return node;
}

pluss::msharp::intrusive_ptr<Node>
Node::integral(pluss::msharp::intrusive_ptr<Node> integrand,
               std::int64_t variable_id, double lower,
               std::optional<double> upper) {
  PLUSS_CHECK(integrand,
              "math::Function: integral requires an integrand graph");
  PLUSS_CHECK(integrand->output().rank() == 0,
              "math::Function: structural integration currently requires a "
              "scalar-valued integrand");
  return pluss::msharp::intrusive_ptr<Node>(
      pluss::msharp::intrusive_ptr<Node>::adopt,
      new Node(NodeKind::Integral, 0, -1, 0.0, pluss::Series{},
               PrimitiveId::Add, {std::move(integrand)}, OutputMetadata{},
               variable_id, lower, upper));
}

FunctionImpl::FunctionImpl(pluss::msharp::intrusive_ptr<TraceState> trace,
                           pluss::msharp::intrusive_ptr<Node> root, bool sealed,
                           bool depends_on_variables,
                           std::vector<std::int64_t> bound_variables)
    : trace_(std::move(trace)), root_(std::move(root)), sealed_(sealed),
      depends_on_variables_(depends_on_variables),
      bound_variables_(std::move(bound_variables)) {
  if (!sealed_) {
    return;
  }
  validate_root(*root_, *trace_);
  plan_ = build_plan(root_);
  binding_index_.reserve(trace_->variables().size());
  std::size_t index = 0;
  for (const std::string_view name : trace_->variables()) {
    binding_index_.emplace(std::string(name), index++);
  }

  std::vector<bool> free(trace_->variables().size(), false);
  std::vector<bool> fixed_bound(trace_->variables().size(), false);
  for (const std::int64_t variable : bound_variables_) {
    PLUSS_CHECK(variable >= 0 &&
                    static_cast<std::size_t>(variable) < fixed_bound.size(),
                "math::Function: bound variable metadata is out of range");
    fixed_bound[static_cast<std::size_t>(variable)] = true;
  }
  std::vector<bool> bound(trace_->variables().size(), false);
  // A graph is a DAG with heavy sharing (power-series graphs share every
  // coefficient), so each node is visited once per distinct set of integral-
  // bound variables, not once per path to it.
  std::unordered_set<std::string> visited;
  const auto collect = [&](const auto &self, const Node &node) -> void {
    const Node *address = &node;
    std::string key(reinterpret_cast<const char *>(&address), sizeof(address));
    for (const bool b : bound) key.push_back(b ? '1' : '0');
    if (!visited.insert(std::move(key)).second) {
      return;
    }
    if (node.kind() == NodeKind::Projection || node.kind() == NodeKind::Opaque) {
      const std::int64_t variable = node.variable_id();
      if (variable >= 0 && !bound[static_cast<std::size_t>(variable)]) {
        free[static_cast<std::size_t>(variable)] = true;
      }
      return;
    }
    if (node.kind() == NodeKind::Integral) {
      const std::size_t variable =
          static_cast<std::size_t>(node.integral_variable_id());
      if (!node.integral_upper() && !bound[variable]) {
        free[variable] = true;
      } else if (node.integral_upper()) {
        fixed_bound[variable] = true;
      }
      const bool was_bound = bound[variable];
      bound[variable] = true;
      self(self, *node.children().front());
      bound[variable] = was_bound;
      return;
    }
    for (const auto &child : node.children()) {
      self(self, *child);
    }
  };
  collect(collect, *root_);

  std::vector<std::string> names;
  for (std::size_t variable = 0; variable < free.size(); ++variable) {
    if (fixed_bound[variable] && !free[variable]) {
      continue;
    }
    active_binding_index_.emplace(
        std::string(trace_->variables().get(static_cast<std::int64_t>(variable))),
        active_variables_.size());
    active_variables_.push_back(static_cast<std::int64_t>(variable));
    names.emplace_back(
        trace_->variables().get(static_cast<std::int64_t>(variable)));
  }
  active_names_ = pluss::msharp::Lister::StringView(std::move(names));
}

Function FunctionAccess::make(pluss::msharp::intrusive_ptr<TraceState> trace,
                              pluss::msharp::intrusive_ptr<Node> root,
                              bool sealed, bool depends_on_variables,
                              std::vector<std::int64_t> bound_variables) {
  PLUSS_CHECK(
      trace && root,
      "math::Function: internal graph must have trace and root storage");
  return Function(pluss::msharp::make_intrusive<FunctionImpl>(
      std::move(trace), std::move(root), sealed, depends_on_variables,
      std::move(bound_variables)));
}

const FunctionImpl &FunctionAccess::get(const Function &function) {
  PLUSS_CHECK(function.impl_,
              "math::Function: operation requires a defined function");
  return *function.impl_;
}

Function FunctionAccess::unary(PrimitiveId primitive_id,
                               const Function &input) {
  const FunctionImpl &impl = get(input);
  return make(impl.trace_ptr(),
              simplify_unary(primitive_id, impl.root_ptr(), impl.trace()),
              impl.sealed(), impl.depends_on_variables(),
              impl.bound_variables());
}

Function FunctionAccess::binary(PrimitiveId primitive_id, const Function &left,
                                const Function &right) {
  const FunctionImpl &lhs = get(left);
  const FunctionImpl &rhs = get(right);
  pluss::msharp::intrusive_ptr<TraceState> trace = merge_trace(lhs, rhs);
  return make(
      trace,
      simplify_binary(primitive_id, lhs.root_ptr(), rhs.root_ptr(), *trace),
      lhs.sealed() && rhs.sealed(),
      lhs.depends_on_variables() || rhs.depends_on_variables(),
      [&] {
        std::vector<std::int64_t> result = lhs.bound_variables();
        merge_bound_variables(result, rhs.bound_variables());
        return result;
      }());
}

Function FunctionAccess::tensor(PrimitiveId primitive_id,
                                std::vector<Function> inputs,
                                std::vector<std::int64_t> attributes) {
  PLUSS_CHECK(!inputs.empty(), "math::Function: tensor operation needs inputs");
  pluss::msharp::intrusive_ptr<TraceState> trace = get(inputs[0]).trace_ptr();
  bool sealed = true;
  bool depends = false;
  std::vector<std::int64_t> bound_variables;
  std::vector<NodePtr> children;
  children.reserve(inputs.size());
  for (std::size_t i = 0; i < inputs.size(); ++i) {
    const FunctionImpl &impl = get(inputs[i]);
    if (i > 0) {
      trace = merge_trace(get(inputs[0]), impl);
    }
    sealed = sealed && impl.sealed();
    depends = depends || impl.depends_on_variables();
    merge_bound_variables(bound_variables, impl.bound_variables());
    children.push_back(impl.root_ptr());
  }
  if (primitive_id == PrimitiveId::Contract && attributes.size() == 1) {
    // internal marker for a matrix product: derive the attributes from ranks
    attributes = {children[0]->output().rank(), children[1]->output().rank(), 1,
                  children[0]->output().rank() - 1, 0};
  }
  return make(std::move(trace),
              tensor_node(primitive_id, std::move(children),
                          std::move(attributes)),
              sealed, depends, std::move(bound_variables));
}

Function FunctionAccess::compose(const Function &function,
                                 pluss::RangeView<Function> arguments) {
  const FunctionImpl &outer = get(function);
  PLUSS_CHECK(outer.sealed(),
              "math::Function::compose: the Function must be finished");
  // one argument per variable the function still takes: a variable that an integral bound is not one
  PLUSS_CHECK(arguments.size() == outer.active_variables().size(),
              "math::Function::compose: expected one argument per variable");
  PLUSS_CHECK(!arguments.empty(),
              "math::Function::compose: the Function has no variables");
  pluss::msharp::intrusive_ptr<TraceState> trace = get(arguments[0]).trace_ptr();
  bool sealed = true;
  bool depends = false;
  std::vector<NodePtr> roots(outer.trace().variables().size());
  for (std::size_t i = 0; i < arguments.size(); ++i) {
    const FunctionImpl &impl = get(arguments[i]);
    if (i > 0) {
      trace = merge_trace(get(arguments[0]), impl);
    }
    sealed = sealed && impl.sealed();
    depends = depends || impl.depends_on_variables();
    roots[static_cast<std::size_t>(outer.active_variables()[i])] = impl.root_ptr();
  }
  // the execution plan is topologically ordered, so one pass rebuilds the graph
  std::unordered_map<const Node *, NodePtr> by_node;
  for (const ExecutionStep &step : outer.plan()) {
    const Node &node = *step.node;
    NodePtr rebuilt;
    switch (node.kind()) {
    case NodeKind::Projection:
      rebuilt = roots[static_cast<std::size_t>(node.variable_id())];
      PLUSS_CHECK(rebuilt, "math::Function::compose: a bound variable cannot be substituted");
      break;
    case NodeKind::ScalarConstant:
    case NodeKind::SeriesConstant:
      rebuilt = step.node;
      break;
    case NodeKind::Opaque:
    case NodeKind::Integral:
      PLUSS_FAIL("math::Function::compose: opaque callables and unresolved "
                 "integrals cannot be composed structurally");
    case NodeKind::Primitive: {
      std::vector<NodePtr> children;
      children.reserve(node.children().size());
      for (const NodePtr &child : node.children()) {
        children.push_back(by_node.at(child.get()));
      }
      rebuilt = rebuild_node(step.node, std::move(children));
      break;
    }
    }
    by_node.emplace(step.node.get(), rebuilt);
  }
  return make(std::move(trace), by_node.at(outer.root_ptr().get()), sealed,
              depends);
}

namespace {

// c x^n exp(e1 x + e2 x^2) with x-free coefficients; an undefined Function means zero
struct ExpTerm {
  Function coef;
  int n = 0;
  Function e1;
  Function e2;
};

Function add_opt(const Function &a, const Function &b) {
  if (!a.defined()) return b;
  if (!b.defined()) return a;
  return a + b;
}

class TermParser {
public:
  TermParser(const FunctionImpl &impl, std::int64_t variable_id)
      : impl_(impl), variable_id_(variable_id) {}

  std::optional<std::vector<ExpTerm>> parse(const NodePtr &node) {
    if (!depends_on(node, variable_id_)) {
      return std::vector<ExpTerm>{{wrap(node), 0, Function{}, Function{}}};
    }
    if (node->kind() == NodeKind::Projection) {
      return std::vector<ExpTerm>{{pluss::subsp::math::function(1.0), 1, Function{}, Function{}}};
    }
    if (node->kind() != NodeKind::Primitive) return std::nullopt;
    const auto &c = node->children();
    switch (node->primitive_id()) {
    case PrimitiveId::Add:
    case PrimitiveId::Sub: {
      auto a = parse(c[0]);
      auto b = parse(c[1]);
      if (!a || !b) return std::nullopt;
      if (node->primitive_id() == PrimitiveId::Sub) {
        for (ExpTerm &t : *b) t.coef = -t.coef;
      }
      a->insert(a->end(), b->begin(), b->end());
      return a;
    }
    case PrimitiveId::Neg: {
      auto a = parse(c[0]);
      if (!a) return std::nullopt;
      for (ExpTerm &t : *a) t.coef = -t.coef;
      return a;
    }
    case PrimitiveId::Mul: {
      auto a = parse(c[0]);
      auto b = parse(c[1]);
      if (!a || !b) return std::nullopt;
      return product(*a, *b);
    }
    case PrimitiveId::Div:
    case PrimitiveId::SafeDivStrict:
    case PrimitiveId::SafeDivZero:
    case PrimitiveId::SafeDivPiecewise: {
      if (depends_on(c[1], variable_id_)) return std::nullopt;
      auto a = parse(c[0]);
      if (!a) return std::nullopt;
      const Function den = wrap(c[1]);
      for (ExpTerm &t : *a) t.coef = t.coef / den;
      return a;
    }
    case PrimitiveId::Pow: {
      if (c[1]->kind() != NodeKind::ScalarConstant) return std::nullopt;
      const double k = c[1]->scalar_value();
      if (k < 0.0 || k > 8.0 || k != std::floor(k)) return std::nullopt;
      auto base = parse(c[0]);
      if (!base) return std::nullopt;
      std::vector<ExpTerm> result{{pluss::subsp::math::function(1.0), 0, Function{}, Function{}}};
      for (int i = 0; i < static_cast<int>(k); ++i) result = product(result, *base);
      return result;
    }
    case PrimitiveId::Exp: {
      auto q = quadratic(c[0]);
      if (!q) return std::nullopt;
      return std::vector<ExpTerm>{{q->c0.exp(), 0, q->c1, q->c2}};
    }
    case PrimitiveId::Sin:
    case PrimitiveId::Cos: {
      auto q = quadratic(c[0]);
      if (!q || q->c2.defined() || !q->c1.defined()) return std::nullopt;
      const Function i = pluss::subsp::math::function(std::complex<double>(0.0, 1.0));
      const Function plus = (i * q->c0).exp(), minus = (-(i * q->c0)).exp();
      const bool sine = node->primitive_id() == PrimitiveId::Sin;
      const Function k_plus = sine ? plus / (i * 2.0) : plus * 0.5;
      const Function k_minus = sine ? -(minus / (i * 2.0)) : minus * 0.5;
      return std::vector<ExpTerm>{{k_plus, 0, i * q->c1, Function{}}, {k_minus, 0, -(i * q->c1), Function{}}};
    }
    case PrimitiveId::Sinh:
    case PrimitiveId::Cosh: {
      auto q = quadratic(c[0]);
      if (!q || q->c2.defined() || !q->c1.defined()) return std::nullopt;
      const bool sinh = node->primitive_id() == PrimitiveId::Sinh;
      return std::vector<ExpTerm>{{q->c0.exp() * 0.5, 0, q->c1, Function{}},
                                  {sinh ? -(-q->c0).exp() * 0.5 : (-q->c0).exp() * 0.5, 0, -q->c1, Function{}}};
    }
    default:
      return std::nullopt;
    }
  }

private:
  struct Quadratic {
    Function c0, c1, c2;
  };

  Function wrap(const NodePtr &node) const {
    return FunctionAccess::make(impl_.trace_ptr(), node, true, true, impl_.bound_variables());
  }

  std::vector<ExpTerm> product(const std::vector<ExpTerm> &a, const std::vector<ExpTerm> &b) const {
    std::vector<ExpTerm> out;
    for (const ExpTerm &x : a)
      for (const ExpTerm &y : b) {
        out.push_back({x.coef * y.coef, x.n + y.n, add_opt(x.e1, y.e1), add_opt(x.e2, y.e2)});
      }
    return out;
  }

  // the argument of an exponential as c0 + c1 x + c2 x^2
  std::optional<Quadratic> quadratic(const NodePtr &arg) {
    auto terms = parse(arg);
    if (!terms) return std::nullopt;
    Quadratic q;
    for (const ExpTerm &t : *terms) {
      if (t.e1.defined() || t.e2.defined() || t.n > 2) return std::nullopt;
      (t.n == 0 ? q.c0 : t.n == 1 ? q.c1 : q.c2) = add_opt(t.n == 0 ? q.c0 : t.n == 1 ? q.c1 : q.c2, t.coef);
    }
    if (!q.c0.defined()) q.c0 = pluss::subsp::math::function(0.0);
    return q;
  }

  const FunctionImpl &impl_;
  std::int64_t variable_id_;
};

bool has_complex_constant(const NodePtr &node, std::unordered_map<const Node *, bool> &memo) {
  if (const auto found = memo.find(node.get()); found != memo.end()) return found->second;
  bool result = false;
  if (node->kind() == NodeKind::SeriesConstant) {
    const auto type = node->series_value().scalar_type();
    result = type == pluss::ScalarType::Complex64 || type == pluss::ScalarType::Complex128;
  }
  for (const NodePtr &child : node->children()) result = result || has_complex_constant(child, memo);
  memo.emplace(node.get(), result);
  return result;
}

double factorial(int n) {
  double out = 1.0;
  for (int i = 2; i <= n; ++i) out *= i;
  return out;
}

} // namespace

Function FunctionAccess::transform_table(const Function &lifted, std::string_view variable,
                                         std::string_view frequency, int kind, bool ordinary) {
  const FunctionImpl &impl = get(lifted);
  const std::int64_t x_id = variable_index(lifted, variable);
  const std::int64_t w_id = variable_index(lifted, frequency);
  TermParser parser(impl, x_id);
  const auto terms = parser.parse(impl.root_ptr());
  if (!terms || terms->empty()) return Function{};

  std::vector<std::int64_t> bound = impl.bound_variables();
  if (std::find(bound.begin(), bound.end(), x_id) == bound.end()) bound.push_back(x_id);
  const Function w = make(impl.trace_ptr(), Node::projection(impl.trace().id(), w_id), true, true, bound);
  const Function omega = ordinary ? w * (2.0 * 3.14159265358979323846) : w;
  const Function i = pluss::subsp::math::function(std::complex<double>(0.0, 1.0));
  const double pi = 3.14159265358979323846;

  Function result;
  if (kind <= 2) {
    // half line: G(p) = sum c n! / (p - e1)^(n+1), no Gaussian part
    for (const ExpTerm &t : *terms) if (t.e2.defined()) return Function{};
    const auto G = [&](const Function &p) {
      Function sum;
      for (const ExpTerm &t : *terms) {
        const Function shifted = t.e1.defined() ? p - t.e1 : p;
        sum = add_opt(sum, t.coef * factorial(t.n) / shifted.pow(static_cast<double>(t.n + 1)));
      }
      return sum;
    };
    if (kind == 0) {
      result = G(w);
    } else {
      const Function plus = G(i * omega), minus = G(-(i * omega));
      result = kind == 1 ? (plus + minus) * 0.5 : (minus - plus) / (i * 2.0);
    }
  } else {
    // whole line: needs a Gaussian in every term
    for (const ExpTerm &t : *terms) if (!t.e2.defined()) return Function{};
    const double sign = kind == 3 ? -1.0 : 1.0;  // e^{sign i w x}
    for (const ExpTerm &t : *terms) {
      const Function a = -t.e2;
      const Function b = t.e1.defined() ? t.e1 : pluss::subsp::math::function(0.0);
      const Function shift = b + i * omega * sign;
      Function E = (pluss::subsp::math::function(pi) / a).sqrt() * (shift * shift / (a * 4.0)).exp();
      if (t.n > 0) {
        E = differentiate(E, frequency, t.n, NonSmooth::Strict, Fallback::Reject, 100000);
        const Function factor = (i * (-sign) / (ordinary ? 2.0 * pi : 1.0)).pow(static_cast<double>(t.n));
        E = E * factor;
      }
      result = add_opt(result, t.coef * E);
    }
  }
  std::unordered_map<const Node *, bool> memo;
  const bool real_input = !has_complex_constant(impl.root_ptr(), memo);
  // The cosine and sine transforms of a real function are real by construction; the Laplace transform is
  // complex-valued for complex s (the Talbot inversion evaluates it there), so it keeps its imaginary part.
  if (real_input && (kind == 1 || kind == 2)) result = result.real();
  const FunctionImpl &out = get(result);
  return make(out.trace_ptr(), out.root_ptr(), true, true, bound);
}

namespace {
// structurally zero: a zero constant or ZerosLike, closed under negation, sums, and products/quotients with a zero factor
bool zero_node(const NodePtr &node) {
  if (node->kind() == NodeKind::ScalarConstant) return node->scalar_value() == 0.0;
  if (node->kind() != NodeKind::Primitive) return false;
  const auto &c = node->children();
  switch (node->primitive_id()) {
  case PrimitiveId::ZerosLike: return true;
  case PrimitiveId::Neg: return zero_node(c[0]);
  case PrimitiveId::Add:
  case PrimitiveId::Sub: return zero_node(c[0]) && zero_node(c[1]);
  case PrimitiveId::Mul: return zero_node(c[0]) || zero_node(c[1]);
  case PrimitiveId::Div:
  case PrimitiveId::SafeDivStrict:
  case PrimitiveId::SafeDivZero:
  case PrimitiveId::SafeDivPiecewise: return zero_node(c[0]);
  default: return false;
  }
}
} // namespace

bool FunctionAccess::is_zero(const Function &function) {
  return zero_node(get(function).root_ptr());
}

Function FunctionAccess::replace(const Function &function,
                                 std::string_view variable,
                                 const Function &by) {
  const FunctionImpl &impl = get(function);
  const FunctionImpl &other = get(by);
  PLUSS_CHECK(impl.sealed() && other.sealed(),
              "math::Function: replace needs finished Functions");
  PLUSS_CHECK(impl.trace().id() == other.trace().id(),
              "math::Function: replace needs both Functions on one trace");
  const std::int64_t variable_id = variable_index(function, variable);
  NodePtr root = substitute(impl.root_ptr(), variable_id, other.root_ptr());
  std::vector<std::int64_t> bound = impl.bound_variables();
  merge_bound_variables(bound, other.bound_variables());
  return make(impl.trace_ptr(), std::move(root), true,
              impl.depends_on_variables() || other.depends_on_variables(),
              std::move(bound));
}

Function FunctionAccess::fractional_table(const Function &function,
                                          std::string_view variable,
                                          double alpha, int mode) {
  const FunctionImpl &impl = get(function);
  const std::int64_t x_id = variable_index(function, variable);
  TermParser parser(impl, x_id);
  const auto terms = parser.parse(impl.root_ptr());
  if (!terms || terms->empty()) return Function{};
  for (const ExpTerm &t : *terms) if (t.e1.defined() || t.e2.defined()) return Function{};
  const Function x = make(impl.trace_ptr(), Node::projection(impl.trace().id(), x_id), true, true,
                          impl.bound_variables());
  const double m = std::ceil(alpha);
  // 1 / Gamma(z), zero at the poles z = 0, -1, -2, ...
  const auto rgamma = [](double z) {
    return (z <= 0.0 && z == std::floor(z)) ? 0.0 : 1.0 / std::tgamma(z);
  };
  Function result;
  for (const ExpTerm &t : *terms) {
    const double n = static_cast<double>(t.n);
    double factor = 0.0, power = 0.0;
    if (mode == 0) {  // I^alpha x^n = n! / Gamma(n + alpha + 1) x^(n + alpha)
      factor = std::tgamma(n + 1.0) * rgamma(n + alpha + 1.0);
      power = n + alpha;
    } else {          // D^alpha x^n = n! / Gamma(n + 1 - alpha) x^(n - alpha); Caputo kills n < ceil(alpha)
      if (mode == 2 && n < m) continue;
      factor = std::tgamma(n + 1.0) * rgamma(n + 1.0 - alpha);
      power = n - alpha;
    }
    if (factor == 0.0) continue;
    const Function term = t.coef * x.pow(power) * factor;
    result = add_opt(result, term);
  }
  if (!result.defined()) result = x * 0.0;
  return result;
}

Function FunctionAccess::fix(const Function &function,
                             std::string_view variable, double value) {
  const FunctionImpl &impl = get(function);
  PLUSS_CHECK(impl.sealed(),
              "math::Function: only a finished Function can be fixed");
  const std::int64_t variable_id = variable_index(function, variable);
  NodePtr root =
      substitute(impl.root_ptr(), variable_id, Node::scalar(value));
  std::vector<std::int64_t> bound = impl.bound_variables();
  if (std::find(bound.begin(), bound.end(), variable_id) == bound.end()) {
    bound.push_back(variable_id);
  }
  return make(impl.trace_ptr(), std::move(root), true,
              impl.depends_on_variables(), std::move(bound));
}

Function FunctionAccess::extend(const Function &function,
                                std::string_view name, Function &variable) {
  const FunctionImpl &source = get(function);
  PLUSS_CHECK(source.sealed(),
              "math::Function: only a finished Function can be extended");
  std::vector<std::string> owned;
  const auto &variables = source.trace().variables();
  for (std::size_t i = 0; i < variables.size(); ++i) {
    owned.emplace_back(variables.get(static_cast<std::int64_t>(i)));
    PLUSS_CHECK(owned.back() != name,
                "math::Function: variable '" + std::string(name) +
                    "' already exists");
  }
  const std::size_t index = owned.size();
  owned.emplace_back(name);
  std::vector<std::string_view> views(owned.begin(), owned.end());
  Trace trace{pluss::StackArray<std::string_view>{views}};
  auto state = get(trace.arguments()[0]).trace_ptr();
  variable = make(state, Node::projection(state->id(),
                                          static_cast<std::int64_t>(index)),
                  true, true);
  std::unordered_map<const Node *, NodePtr> memo;
  NodePtr root = detail::rebind(source.root_ptr(), source, get(variable), memo);
  return make(std::move(state), std::move(root), true,
              source.depends_on_variables(), source.bound_variables());
}

std::int64_t FunctionAccess::variable_index(const Function &function,
                                            std::string_view variable) {
  const FunctionImpl &impl = get(function);
  const auto found = impl.binding_index().find(std::string(variable));
  PLUSS_CHECK(found != impl.binding_index().end(),
              "math::Function: unknown variable '" + std::string(variable) + "'");
  return static_cast<std::int64_t>(found->second);
}

bool FunctionAccess::contains_opaque(const Function &function) {
  for (const ExecutionStep &step : get(function).plan()) {
    if (step.node->kind() == NodeKind::Opaque) {
      return true;
    }
  }
  return false;
}

namespace {

// Truncated power series (coefficients 0..order) of a graph in the expansion
// variable, built directly from graph nodes (like the derivative engine): an
// analytic graph of O(order^2) nodes per operation, differentiable and
// saveable, with no per-operation plan construction.
using Jet = std::vector<NodePtr>;

NodePtr zero_node() { return Node::scalar(0.0); }
NodePtr one_node() { return Node::scalar(1.0); }
NodePtr n_add(const NodePtr &a, const NodePtr &b) { return binary_node(PrimitiveId::Add, a, b); }
NodePtr n_sub(const NodePtr &a, const NodePtr &b) { return binary_node(PrimitiveId::Sub, a, b); }
NodePtr n_mul(const NodePtr &a, const NodePtr &b) { return binary_node(PrimitiveId::Mul, a, b); }
NodePtr n_div(const NodePtr &a, const NodePtr &b) { return binary_node(PrimitiveId::Div, a, b); }
NodePtr n_muls(const NodePtr &a, double c) { return binary_node(PrimitiveId::Mul, a, Node::scalar(c)); }
NodePtr n_un(PrimitiveId id, const NodePtr &a) { return unary_node(id, a); }

Jet jet_constant(const NodePtr &value, std::int64_t order) {
  Jet jet(static_cast<std::size_t>(order + 1), zero_node());
  jet[0] = value;
  return jet;
}
Jet jet_scale_by(const Jet &a, const NodePtr &factor) {
  Jet out;
  out.reserve(a.size());
  for (const NodePtr &c : a) out.push_back(n_mul(c, factor));
  return out;
}
Jet jet_add(const Jet &a, const Jet &b, bool subtract) {
  Jet out;
  for (std::size_t k = 0; k < a.size(); ++k) out.push_back(subtract ? n_sub(a[k], b[k]) : n_add(a[k], b[k]));
  return out;
}
Jet jet_mul(const Jet &a, const Jet &b) {
  Jet out;
  for (std::size_t k = 0; k < a.size(); ++k) {
    NodePtr sum = n_mul(a[0], b[k]);
    for (std::size_t j = 1; j <= k; ++j) sum = n_add(sum, n_mul(a[j], b[k - j]));
    out.push_back(sum);
  }
  return out;
}
Jet jet_div(const Jet &a, const Jet &b) {
  Jet out;
  for (std::size_t k = 0; k < a.size(); ++k) {
    NodePtr numerator = a[k];
    for (std::size_t j = 1; j <= k; ++j) numerator = n_sub(numerator, n_mul(b[j], out[k - j]));
    out.push_back(n_div(numerator, b[0]));
  }
  return out;
}
// c' = a' c  (exp): k c_k = sum_{j=1..k} j a_j c_{k-j}
Jet jet_exp(const Jet &a) {
  Jet out{n_un(PrimitiveId::Exp, a[0])};
  for (std::size_t k = 1; k < a.size(); ++k) {
    NodePtr sum = n_mul(a[1], out[k - 1]);
    for (std::size_t j = 2; j <= k; ++j) sum = n_add(sum, n_muls(n_mul(a[j], out[k - j]), static_cast<double>(j)));
    out.push_back(n_muls(sum, 1.0 / static_cast<double>(k)));
  }
  return out;
}
// a' = c' a  (log): solved for c_k
Jet jet_log(const Jet &a) {
  Jet out{n_un(PrimitiveId::Log, a[0])};
  for (std::size_t k = 1; k < a.size(); ++k) {
    NodePtr sum = zero_node();
    for (std::size_t j = 1; j < k; ++j) sum = n_add(sum, n_muls(n_mul(out[j], a[k - j]), static_cast<double>(j)));
    out.push_back(n_div(n_sub(a[k], n_muls(sum, 1.0 / static_cast<double>(k))), a[0]));
  }
  return out;
}
Jet jet_sqrt(const Jet &a) {
  Jet out{n_un(PrimitiveId::Sqrt, a[0])};
  for (std::size_t k = 1; k < a.size(); ++k) {
    NodePtr sum = zero_node();
    for (std::size_t j = 1; j < k; ++j) sum = n_add(sum, n_mul(out[j], out[k - j]));
    out.push_back(n_div(n_sub(a[k], sum), n_muls(out[0], 2.0)));
  }
  return out;
}
// constant exponent p: c_k = (1 / (k a_0)) sum_{j=1..k} (p j - (k - j)) a_j c_{k-j}; small integer p by products
Jet jet_pow_constant(const Jet &a, double p, std::int64_t order) {
  if (std::floor(p) == p && p >= 0.0 && p <= 64.0) {
    Jet out = jet_constant(one_node(), order);
    for (std::int64_t i = 0; i < static_cast<std::int64_t>(p); ++i) out = jet_mul(out, a);
    return out;
  }
  Jet out{binary_node(PrimitiveId::Pow, a[0], Node::scalar(p))};
  for (std::size_t k = 1; k < a.size(); ++k) {
    NodePtr sum = zero_node();
    for (std::size_t j = 1; j <= k; ++j)
      sum = n_add(sum, n_muls(n_mul(a[j], out[k - j]), p * static_cast<double>(j) - static_cast<double>(k - j)));
    out.push_back(n_div(sum, n_muls(a[0], static_cast<double>(k))));
  }
  return out;
}
// sine/cosine (or hyperbolic) pair: s' = a' c, c' = -+ a' s
std::pair<Jet, Jet> jet_trig(const Jet &a, bool hyperbolic) {
  Jet s{n_un(hyperbolic ? PrimitiveId::Sinh : PrimitiveId::Sin, a[0])},
      c{n_un(hyperbolic ? PrimitiveId::Cosh : PrimitiveId::Cos, a[0])};
  for (std::size_t k = 1; k < a.size(); ++k) {
    NodePtr ds = zero_node(), dc = zero_node();
    for (std::size_t j = 1; j <= k; ++j) {
      ds = n_add(ds, n_muls(n_mul(a[j], c[k - j]), static_cast<double>(j)));
      dc = n_add(dc, n_muls(n_mul(a[j], s[k - j]), static_cast<double>(j)));
    }
    s.push_back(n_muls(ds, 1.0 / static_cast<double>(k)));
    c.push_back(n_muls(dc, (hyperbolic ? 1.0 : -1.0) / static_cast<double>(k)));
  }
  return {s, c};
}
// t' = (1 + sign t^2) a'  (tan: +, tanh: -); q = 1 + sign t^2
Jet jet_tan(const Jet &a, bool hyperbolic) {
  const double sign = hyperbolic ? -1.0 : 1.0;
  Jet t{n_un(hyperbolic ? PrimitiveId::Tanh : PrimitiveId::Tan, a[0])};
  Jet q;
  q.push_back(n_add(one_node(), n_muls(n_mul(t[0], t[0]), sign)));
  for (std::size_t k = 1; k < a.size(); ++k) {
    NodePtr sum = zero_node();
    for (std::size_t j = 1; j <= k; ++j) sum = n_add(sum, n_muls(n_mul(a[j], q[k - j]), static_cast<double>(j)));
    t.push_back(n_muls(sum, 1.0 / static_cast<double>(k)));
    NodePtr square = zero_node();
    for (std::size_t j = 0; j <= k; ++j) square = n_add(square, n_mul(t[j], t[k - j]));
    q.push_back(n_muls(square, sign));
  }
  return t;
}

Jet jet_of_step(const Node &node, const std::vector<const Jet *> &in, std::int64_t order) {
  switch (node.primitive_id()) {
  case PrimitiveId::Add: return jet_add(*in[0], *in[1], false);
  case PrimitiveId::Sub: return jet_add(*in[0], *in[1], true);
  case PrimitiveId::Mul: return jet_mul(*in[0], *in[1]);
  case PrimitiveId::Div: return jet_div(*in[0], *in[1]);
  case PrimitiveId::Neg: {
    Jet out;
    for (const NodePtr &c : *in[0]) out.push_back(n_un(PrimitiveId::Neg, c));
    return out;
  }
  case PrimitiveId::Exp: return jet_exp(*in[0]);
  case PrimitiveId::Expm1: {
    Jet out = jet_exp(*in[0]);
    out[0] = n_sub(out[0], one_node());
    return out;
  }
  case PrimitiveId::Log: return jet_log(*in[0]);
  case PrimitiveId::Log1p: {
    Jet shifted = *in[0];
    shifted[0] = n_add(shifted[0], one_node());
    return jet_log(shifted);
  }
  case PrimitiveId::Sqrt: return jet_sqrt(*in[0]);
  case PrimitiveId::Sin: return jet_trig(*in[0], false).first;
  case PrimitiveId::Cos: return jet_trig(*in[0], false).second;
  case PrimitiveId::Sinh: return jet_trig(*in[0], true).first;
  case PrimitiveId::Cosh: return jet_trig(*in[0], true).second;
  case PrimitiveId::Tan: return jet_tan(*in[0], false);
  case PrimitiveId::Tanh: return jet_tan(*in[0], true);
  case PrimitiveId::Abs: return jet_scale_by(*in[0], n_div((*in[0])[0], n_un(PrimitiveId::Abs, (*in[0])[0])));
  case PrimitiveId::Pow:
    if (node.children()[1]->kind() == NodeKind::ScalarConstant) {
      return jet_pow_constant(*in[0], node.children()[1]->scalar_value(), order);
    }
    return jet_exp(jet_mul(*in[1], jet_log(*in[0])));
  case PrimitiveId::ZerosLike:
  case PrimitiveId::AbsSlopeStrict:
  case PrimitiveId::AbsSlopeZero:
  case PrimitiveId::AbsSlopePiecewise:
    return jet_constant(zero_node(), order);
  case PrimitiveId::OnesLike:
    return jet_constant(one_node(), order);
  default:
    PLUSS_FAIL("math::Function: power-series expansion supports scalar "
               "elementwise Functions only (no tensor operations, integrals "
               "or opaque callables)");
  }
}

} // namespace

bool FunctionAccess::supports_power_series(const Function &function) {
  const FunctionImpl &impl = get(function);
  if (impl.root().output().rank() != 0) return false;
  for (const ExecutionStep &step : impl.plan()) {
    const Node &node = *step.node;
    if (node.kind() == NodeKind::Opaque || node.kind() == NodeKind::Integral) return false;
    if (node.kind() != NodeKind::Primitive) continue;
    switch (node.primitive_id()) {
    case PrimitiveId::Add: case PrimitiveId::Sub: case PrimitiveId::Mul: case PrimitiveId::Div:
    case PrimitiveId::Neg: case PrimitiveId::Exp: case PrimitiveId::Expm1: case PrimitiveId::Log:
    case PrimitiveId::Log1p: case PrimitiveId::Sqrt: case PrimitiveId::Sin: case PrimitiveId::Cos:
    case PrimitiveId::Sinh: case PrimitiveId::Cosh: case PrimitiveId::Tan: case PrimitiveId::Tanh:
    case PrimitiveId::Abs: case PrimitiveId::Pow: case PrimitiveId::ZerosLike:
    case PrimitiveId::OnesLike: case PrimitiveId::AbsSlopeStrict: case PrimitiveId::AbsSlopeZero:
    case PrimitiveId::AbsSlopePiecewise:
      break;
    default:
      return false;
    }
  }
  return true;
}

Function FunctionAccess::power_series_coefficients(const Function &function,
                                                   std::string_view variable,
                                                   double center,
                                                   std::int64_t order) {
  PLUSS_CHECK(order >= 0 && order <= 256,
              "operators::expansion: order must lie in [0, 256]");
  const FunctionImpl &impl = get(function);
  PLUSS_CHECK(supports_power_series(function),
              "operators::expansion: the power series of a Function needs a "
              "scalar elementwise Function (no tensor operations, integrals "
              "or opaque callables)");
  const std::int64_t variable_id = variable_index(function, variable);
  std::vector<Jet> jets;
  jets.reserve(impl.plan().size());
  for (const ExecutionStep &step : impl.plan()) {
    const Node &node = *step.node;
    switch (node.kind()) {
    case NodeKind::Projection:
      if (node.variable_id() == variable_id) {
        Jet jet = jet_constant(Node::scalar(center), order);
        if (order >= 1) jet[1] = one_node();
        jets.push_back(std::move(jet));
      } else {
        jets.push_back(jet_constant(step.node, order));
      }
      break;
    case NodeKind::ScalarConstant:
    case NodeKind::SeriesConstant:
      jets.push_back(jet_constant(step.node, order));
      break;
    case NodeKind::Primitive: {
      std::vector<const Jet *> in;
      for (const std::size_t input : step.inputs) in.push_back(&jets[input]);
      jets.push_back(jet_of_step(node, in, order));
      break;
    }
    case NodeKind::Opaque:
    case NodeKind::Integral:
      PLUSS_FAIL("operators::expansion: power-series expansion does not "
                 "support opaque callables or integrals");
    }
  }
  // the expansion variable is replaced by its centre: the coefficients depend on the others only
  std::vector<std::int64_t> bound = impl.bound_variables();
  merge_bound_variables(bound, {variable_id});
  return make(impl.trace_ptr(), stack_nodes(std::move(jets.back())), true, true,
              std::move(bound));
}

Function FunctionAccess::stack(pluss::RangeView<Function> inputs) {
  PLUSS_CHECK(!inputs.empty(), "math::stack: at least one component is required");
  pluss::msharp::intrusive_ptr<TraceState> trace = get(inputs[0]).trace_ptr();
  bool sealed = true;
  bool depends = false;
  std::vector<std::int64_t> bound_variables;
  std::vector<NodePtr> children;
  children.reserve(inputs.size());
  for (std::size_t i = 0; i < inputs.size(); ++i) {
    const FunctionImpl &impl = get(inputs[i]);
    if (i > 0) {
      trace = merge_trace(get(inputs[0]), impl);
    }
    sealed = sealed && impl.sealed();
    depends = depends || impl.depends_on_variables();
    merge_bound_variables(bound_variables, impl.bound_variables());
    children.push_back(impl.root_ptr());
  }
  return make(std::move(trace), stack_nodes(std::move(children)), sealed,
              depends, std::move(bound_variables));
}

Function FunctionAccess::argument(const Function &function,
                                  std::string_view variable) {
  const FunctionImpl &impl = get(function);
  const auto found = impl.binding_index().find(std::string(variable));
  PLUSS_CHECK(found != impl.binding_index().end(),
              "math::Function: unknown variable '" + std::string(variable) +
                  "'");
  return make(impl.trace_ptr(),
              Node::projection(impl.trace().id(),
                               static_cast<std::int64_t>(found->second)),
              true, true);
}

Function FunctionAccess::assume(const Function &function,
                                std::string_view variable,
                                Assumption assumption) {
  const FunctionImpl &impl = get(function);
  const auto found = impl.binding_index().find(std::string(variable));
  PLUSS_CHECK(found != impl.binding_index().end(),
              "math::Function::assume: unknown variable '" +
                  std::string(variable) + "'");
  std::vector<Assumption> assumptions = impl.trace().assumptions();
  assumptions[found->second] =
      merge_assumption(assumptions[found->second], assumption);
  auto trace = pluss::msharp::make_intrusive<TraceState>(
      impl.trace().id(), impl.trace().variables(), std::move(assumptions));
  Simplifier simplifier(*trace);
  return make(std::move(trace), simplifier.apply(impl.root_ptr()),
              impl.sealed(), impl.depends_on_variables(),
              impl.bound_variables());
}

Function FunctionAccess::shift(const Function &function,
                               std::string_view variable, double amount) {
  const FunctionImpl &impl = get(function);
  const auto found = impl.binding_index().find(std::string(variable));
  PLUSS_CHECK(found != impl.binding_index().end(),
              "operators::shift: unknown variable '" + std::string(variable) +
                  "'");

  const std::int64_t variable_id = static_cast<std::int64_t>(found->second);
  std::vector<pluss::msharp::intrusive_ptr<Node>> replacements;
  replacements.reserve(impl.plan().size());
  for (const ExecutionStep &step : impl.plan()) {
    const Node &node = *step.node;
    if (node.kind() == NodeKind::Projection &&
        node.variable_id() == variable_id) {
      replacements.push_back(Node::primitive(
          PrimitiveId::Add, {step.node, Node::scalar(amount)}, node.output()));
      continue;
    }
    if (node.kind() == NodeKind::Opaque && node.variable_id() == variable_id) {
      const auto callable = node.opaque_callable();
      replacements.push_back(Node::opaque(
          [callable, amount](const pluss::Series &value) {
            return callable(value.add(amount));
          },
          variable_id, node.opaque_derivative_order()));
      continue;
    }
    if (node.kind() != NodeKind::Primitive) {
      replacements.push_back(step.node);
      continue;
    }

    std::vector<pluss::msharp::intrusive_ptr<Node>> children;
    children.reserve(step.inputs.size());
    for (const std::size_t input : step.inputs) {
      children.push_back(replacements[input]);
    }
    replacements.push_back(Node::primitive(node.primitive_id(),
                                           std::move(children), node.output()));
  }
  PLUSS_CHECK(!replacements.empty(),
              "operators::shift: function execution plan is empty");
  return make(impl.trace_ptr(), std::move(replacements.back()), true,
              impl.depends_on_variables(), impl.bound_variables());
}

Function FunctionAccess::rebind(const Function &function,
                                const Function &target) {
  const FunctionImpl &source_impl = get(function);
  const FunctionImpl &target_impl = get(target);
  std::unordered_map<const Node *, NodePtr> memo;
  NodePtr root =
      detail::rebind(source_impl.root_ptr(), source_impl, target_impl, memo);
  return make(target_impl.trace_ptr(), std::move(root), true,
              source_impl.depends_on_variables(),
              target_impl.bound_variables());
}

Function FunctionAccess::differentiate(const Function &function,
                                       std::string_view variable,
                                       std::int64_t order, NonSmooth nonsmooth,
                                       Fallback fallback,
                                       std::size_t max_nodes) {
  const FunctionImpl &impl = get(function);
  PLUSS_CHECK(order > 0, "operators::differential: order must be positive");
  const auto found = impl.binding_index().find(std::string(variable));
  PLUSS_CHECK(found != impl.binding_index().end(),
              "operators::differential: unknown variable '" +
                  std::string(variable) + "'");
  Differentiator differentiator(nonsmooth, fallback, max_nodes);
  NodePtr root = differentiator.apply(
      impl.root_ptr(), static_cast<std::int64_t>(found->second), order);
  require_graph_limit(root, max_nodes, "operators::differential");
  return make(impl.trace_ptr(), std::move(root), true,
              impl.depends_on_variables(), impl.bound_variables());
}

Function FunctionAccess::wirtinger(const Function &function,
                                   std::string_view variable,
                                   std::int64_t order, bool conjugate,
                                   NonSmooth nonsmooth,
                                   std::size_t max_nodes) {
  const FunctionImpl &impl = get(function);
  PLUSS_CHECK(order > 0, "operators::wirtinger: order must be positive");
  const auto found = impl.binding_index().find(std::string(variable));
  PLUSS_CHECK(found != impl.binding_index().end(),
              "operators::wirtinger: unknown variable '" +
                  std::string(variable) + "'");
  const std::int64_t variable_id =
      static_cast<std::int64_t>(found->second);
  NodePtr root = impl.root_ptr();
  for (std::int64_t current = 0; current < order; ++current) {
    WirtingerDifferentiator differentiator(variable_id, nonsmooth);
    const WirtingerPair pair = differentiator.apply(root);
    root = conjugate ? pair.conjugate : pair.z;
    require_graph_limit(root, max_nodes,
                        conjugate ? "operators::conjugate_wirtinger"
                                  : "operators::wirtinger");
  }
  return make(impl.trace_ptr(), std::move(root), true,
              impl.depends_on_variables(), impl.bound_variables());
}

Function FunctionAccess::gradient(const Function &function,
                                  pluss::RangeView<std::string_view> variables,
                                  NonSmooth nonsmooth, Fallback fallback,
                                  std::size_t max_nodes) {
  const FunctionImpl &impl = get(function);
  PLUSS_CHECK(impl.root().output().rank() == 0,
              "operators::gradient: input Function must be scalar-valued");
  const std::vector<std::int64_t> selected =
      resolve_variables(impl, variables, "operators::gradient");
  Differentiator differentiator(nonsmooth, fallback);
  std::vector<NodePtr> components;
  components.reserve(selected.size());
  for (const std::int64_t variable : selected) {
    components.push_back(differentiator.apply(impl.root_ptr(), variable, 1));
  }
  NodePtr root = stack_nodes(std::move(components));
  require_graph_limit(root, max_nodes, "operators::gradient");
  return make(impl.trace_ptr(), std::move(root), true,
              impl.depends_on_variables(), impl.bound_variables());
}

Function FunctionAccess::jacobian(const Function &function,
                                  pluss::RangeView<std::string_view> variables,
                                  NonSmooth nonsmooth, Fallback fallback,
                                  std::size_t max_nodes) {
  const FunctionImpl &impl = get(function);
  PLUSS_CHECK(impl.root().output().rank() <= 1,
              "operators::jacobian: input Function must be scalar- or "
              "vector-valued");
  const std::vector<std::int64_t> selected =
      resolve_variables(impl, variables, "operators::jacobian");
  Differentiator differentiator(nonsmooth, fallback);
  std::vector<NodePtr> columns;
  columns.reserve(selected.size());
  for (const std::int64_t variable : selected) {
    columns.push_back(differentiator.apply(impl.root_ptr(), variable, 1));
  }
  NodePtr root = stack_nodes(std::move(columns));
  require_graph_limit(root, max_nodes, "operators::jacobian");
  return make(impl.trace_ptr(), std::move(root), true,
              impl.depends_on_variables(), impl.bound_variables());
}

Function FunctionAccess::hessian(const Function &function,
                                 pluss::RangeView<std::string_view> variables,
                                 NonSmooth nonsmooth, Fallback fallback,
                                 std::size_t max_nodes) {
  const FunctionImpl &impl = get(function);
  PLUSS_CHECK(impl.root().output().rank() == 0,
              "operators::hessian: input Function must be scalar-valued");
  const std::vector<std::int64_t> selected =
      resolve_variables(impl, variables, "operators::hessian");
  Differentiator differentiator(nonsmooth, fallback);
  std::vector<NodePtr> rows;
  rows.reserve(selected.size());
  for (const std::int64_t row_variable : selected) {
    const NodePtr first =
        differentiator.apply(impl.root_ptr(), row_variable, 1);
    std::vector<NodePtr> row;
    row.reserve(selected.size());
    for (const std::int64_t column_variable : selected) {
      row.push_back(differentiator.apply(first, column_variable, 1));
    }
    rows.push_back(stack_nodes(std::move(row)));
  }
  NodePtr root = stack_nodes(std::move(rows));
  require_graph_limit(root, max_nodes, "operators::hessian");
  return make(impl.trace_ptr(), std::move(root), true,
              impl.depends_on_variables(), impl.bound_variables());
}

Function FunctionAccess::antiderivative(const Function &function,
                                        std::string_view variable) {
  const FunctionImpl &impl = get(function);
  const auto found = impl.binding_index().find(std::string(variable));
  PLUSS_CHECK(found != impl.binding_index().end(),
              "operators::antiderivative: unknown variable '" +
                  std::string(variable) + "'");
  const auto variable_id = static_cast<std::int64_t>(found->second);
  Integrator integrator(impl.trace().id(), variable_id);
  NodePtr root =
      integrator.apply(impl.root_ptr())
          .value_or(Node::integral(impl.root_ptr(), variable_id, 0.0));
  return make(impl.trace_ptr(), std::move(root), true,
              impl.depends_on_variables(), impl.bound_variables());
}

Function FunctionAccess::integral(const Function &function,
                                  std::string_view variable, double lower,
                                  std::optional<double> upper) {
  const FunctionImpl &impl = get(function);
  // Bounds may be infinite (a half line or the whole line); an infinite range has no
  // closed-form primitive to substitute into, so it stays an unresolved integral that
  // quadrature evaluates lazily.
  PLUSS_CHECK(!std::isnan(lower) && lower < std::numeric_limits<double>::infinity(),
              "operators::integral: lower bound must be a number below +infinity");
  PLUSS_CHECK(!upper || (!std::isnan(*upper) && *upper > -std::numeric_limits<double>::infinity()),
              "operators::integral: upper bound must be a number above -infinity");
  const bool infinite_range = std::isinf(lower) || (upper && std::isinf(*upper));
  PLUSS_CHECK(!infinite_range || upper,
              "operators::integral: an infinite bound needs both limits");
  const auto found = impl.binding_index().find(std::string(variable));
  PLUSS_CHECK(found != impl.binding_index().end(),
              "operators::integral: unknown variable '" +
                  std::string(variable) + "'");
  const auto variable_id = static_cast<std::int64_t>(found->second);
  Integrator integrator(impl.trace().id(), variable_id);
  NodePtr root;
  if (upper && lower == -*upper &&
      parity(impl.root_ptr(), variable_id) == Parity::Odd) {
    root = contextual_zero(impl.root_ptr(), variable_id);
  } else if (infinite_range) {
    root = Node::integral(impl.root_ptr(), variable_id, lower, upper);
  } else if (std::optional<NodePtr> primitive =
                 integrator.apply(impl.root_ptr())) {
    const NodePtr at_lower =
        substitute(*primitive, variable_id, Node::scalar(lower));
    if (upper) {
      const NodePtr at_upper =
          substitute(*primitive, variable_id, Node::scalar(*upper));
      root = binary_node(PrimitiveId::Sub, at_upper, at_lower);
    } else {
      root = binary_node(PrimitiveId::Sub, *primitive, at_lower);
    }
  } else {
    root = Node::integral(impl.root_ptr(), variable_id, lower, upper);
  }
  std::vector<std::int64_t> bound_variables = impl.bound_variables();
  if (upper && std::find(bound_variables.begin(), bound_variables.end(),
                         variable_id) == bound_variables.end()) {
    bound_variables.push_back(variable_id);
  }
  return make(impl.trace_ptr(), std::move(root), true,
              impl.depends_on_variables(), std::move(bound_variables));
}

pluss::Series
FunctionAccess::evaluate(const Function &function,
                         pluss::RangeView<pluss::Series> arguments) {
  const FunctionImpl &impl = get(function);
  PLUSS_CHECK(
      impl.sealed(),
      "math::Function: an unsealed trace projection cannot be evaluated");
  PLUSS_CHECK(arguments.size() == impl.active_variables().size(),
              "math::Function: ordered argument count " +
                  std::to_string(arguments.size()) +
                  " does not match function arity " +
                  std::to_string(impl.active_variables().size()));

  const pluss::Series *like = arguments.empty() ? nullptr : &arguments[0];
  for (const pluss::Series &argument : arguments) {
    PLUSS_CHECK(argument.defined(),
                "math::Function: arguments must be defined Series values");
    PLUSS_CHECK(!like || argument.platform() == like->platform(),
                "math::Function: all arguments must use the same platform");
  }

  std::vector<std::pair<double, pluss::Series>> scalar_cache;
  scalar_cache.reserve(std::min<std::size_t>(impl.plan().size(), 8));
  const auto scalar_value = [&](double value) -> pluss::Series {
    const auto found = std::find_if(
        scalar_cache.begin(), scalar_cache.end(),
        [value](const auto &entry) { return entry.first == value; });
    if (found != scalar_cache.end()) {
      return found->second;
    }
    pluss::Series scalar =
        like ? pluss::codem::make_scalar_like(*like, value)
             : pluss::Series::full({}, value, pluss::ScalarType::Float64,
                                   pluss::pCPU);
    scalar_cache.emplace_back(value, scalar);
    return scalar;
  };

  std::vector<pluss::Series> expanded(impl.trace().variables().size(),
                                      scalar_value(0.0));
  for (std::size_t i = 0; i < arguments.size(); ++i) {
    expanded[static_cast<std::size_t>(impl.active_variables()[i])] =
        arguments[i];
  }
  arguments = pluss::RangeView<pluss::Series>(expanded);

  const auto evaluate_plan = [&](const auto &self,
                                 const std::vector<ExecutionStep> &plan,
                                 pluss::RangeView<pluss::Series> bindings)
      -> pluss::Series {
  std::vector<pluss::Series> values;
  values.reserve(plan.size());
  std::array<pluss::Series, 2> fixed_inputs{};
  std::vector<pluss::Series> variadic_inputs;
  for (std::size_t step_index = 0; step_index < plan.size(); ++step_index) {
    const ExecutionStep &step = plan[step_index];
    const Node &node = *step.node;
    switch (node.kind()) {
    case NodeKind::Projection:
      values.push_back(bindings[static_cast<std::size_t>(node.variable_id())]);
      break;
    case NodeKind::ScalarConstant:
      values.push_back(scalar_value(node.scalar_value()));
      break;
    case NodeKind::SeriesConstant: {
      // Internal constants are immutable operands, so their Series payload can
      // be shared safely. Clone only when the constant itself escapes as the
      // Function result, preserving value semantics for subsequent calls.
      pluss::Series value =
          step_index + 1 == plan.size() ? node.series_value().clone()
                                        : node.series_value();
      if (like && value.platform() != like->platform()) {
        value = value.to(like->platform());
      }
      values.push_back(std::move(value));
      break;
    }
    case NodeKind::Opaque: {
      const pluss::Series &argument =
          bindings[static_cast<std::size_t>(node.variable_id())];
      // A derivative of an opaque callable is pointwise, like every
      // structural derivative: array arguments get d fn_i / d x_i with the
      // shape of the argument, not the full derivative tensor.
      pluss::Series value =
          node.opaque_derivative_order() == 0
              ? node.opaque_callable()(argument)
          : argument.dim() == 0
              ? argument.calc.diff(node.opaque_callable(),
                                   node.opaque_derivative_order())
              : argument
                    .calc({.order = node.opaque_derivative_order(),
                           .elementwise = true})
                    .diff(node.opaque_callable());
      PLUSS_CHECK(value.defined(),
                  "math::Function: opaque callable returned an undefined "
                  "Series");
      values.push_back(std::move(value));
      break;
    }
    case NodeKind::Integral: {
      const std::int64_t variable_id = node.integral_variable_id();
      PLUSS_CHECK(variable_id >= 0 &&
                      static_cast<std::size_t>(variable_id) < bindings.size(),
                  "math::Function: integral variable binding is out of range");
      double upper = 0.0;
      if (node.integral_upper()) {
        upper = *node.integral_upper();
      } else {
        PLUSS_CHECK(bindings[static_cast<std::size_t>(variable_id)].numel() ==
                        1,
                    "math::Function: unresolved anchored integrals require "
                    "a scalar upper-bound argument");
        upper = bindings[static_cast<std::size_t>(variable_id)]
                    .to(pluss::pCPU, pluss::ScalarType::Float64)
                    .host<double>()
                    .front();
      }
      std::vector<pluss::Series> bound(bindings.begin(), bindings.end());
      for (std::size_t i = 0; i < bound.size(); ++i) {
        if (i != static_cast<std::size_t>(variable_id) && bound[i].dim() > 0) {
          bound[i] = bound[i].unsqueeze(-1);
        }
      }
      integ::QuadraOptions options;
      // An open rule: the integrand is never evaluated at the end points, so integrable end-point
      // singularities and removable 0/0 end points (the declared null points of a measure) are safe.
      options.rule = integ::QuadraRule::GaussLegendre;
      options.dtype =
          bindings[static_cast<std::size_t>(variable_id)].scalar_type();
      options.platform =
          bindings[static_cast<std::size_t>(variable_id)].platform();
      if (options.dtype == pluss::ScalarType::Float16 ||
          options.dtype == pluss::ScalarType::BFloat16 ||
          options.dtype == pluss::ScalarType::Float8E4M3FN ||
          options.dtype == pluss::ScalarType::Float8E5M2) {
        options.tolerance = 1e-2;
      } else if (options.dtype == pluss::ScalarType::Float32 ||
                 options.dtype == pluss::ScalarType::Complex64) {
        options.tolerance = 1e-4;
      }
      const integ::QuadraResult result = integ::quadra(
          [&self, integral_plan = &step.integral_plan,
           bound = std::move(bound),
           variable_id](const pluss::Series &nodes) mutable {
            bound[static_cast<std::size_t>(variable_id)] = nodes;
            return self(self, *integral_plan,
                        pluss::RangeView<pluss::Series>(bound));
          },
          node.integral_lower(), upper, options);
      PLUSS_CHECK(result.converged,
                  "math::Function: unresolved integral quadrature did not "
                  "converge; error estimate=" +
                      std::to_string(result.error_estimate));
      values.push_back(result.value);
      break;
    }
    case NodeKind::Primitive: {
      const PrimitiveSpec &spec = primitive(node.primitive_id());
      pluss::RangeView<pluss::Series> inputs;
      if (step.inputs.size() <= fixed_inputs.size()) {
        for (std::size_t i = 0; i < step.inputs.size(); ++i) {
          fixed_inputs[i] = values[step.inputs[i]];
        }
        inputs = pluss::RangeView<pluss::Series>(fixed_inputs.data(),
                                                 step.inputs.size());
      } else {
        variadic_inputs.clear();
        variadic_inputs.reserve(step.inputs.size());
        for (const std::size_t input : step.inputs) {
          variadic_inputs.push_back(values[input]);
        }
        inputs = pluss::RangeView<pluss::Series>(variadic_inputs);
      }
      pluss::Series value =
          spec.evaluate_with_attributes != nullptr
              ? spec.evaluate_with_attributes(inputs, node.attributes())
              : spec.evaluate(inputs);
      PLUSS_CHECK(value.defined(), "math::Function: primitive '" +
                                       std::string(spec.name) +
                                       "' returned an undefined Series");
      values.push_back(std::move(value));
      break;
    }
    }
    // The MPS cache currently records the last writer but not subsequent
    // readers. Destroying an input immediately after encoding its final read
    // therefore drains the queue conservatively. Reclaim CPU slots now; MPS
    // liveness reuse must wait for native read-fence tracking.
    if (!like || like->platform() == pluss::pCPU) {
      for (const std::size_t release : step.releases) {
        values[release] = pluss::Series{};
      }
    }
  }
  PLUSS_CHECK(!values.empty(),
              "math::Function: compiled execution plan is empty");
  return values.back();
  };
  return evaluate_plan(evaluate_plan, impl.plan(), arguments);
}

Trace::Trace(pluss::StackArray<std::string_view> variables) {
  pluss::msharp::Lister::StringView owned = std::move(variables).lister();
  validate_names(owned);
  state_ = pluss::msharp::make_intrusive<TraceState>(
      next_trace_id.fetch_add(1, std::memory_order_relaxed), std::move(owned));
  arguments_.reserve(state_->variables().size());
  for (std::size_t i = 0; i < state_->variables().size(); ++i) {
    arguments_.push_back(FunctionAccess::make(
        state_, Node::projection(state_->id(), static_cast<std::int64_t>(i)),
        false, true));
  }
}

Trace::Trace(Trace &&) noexcept = default;
Trace &Trace::operator=(Trace &&) noexcept = default;
Trace::~Trace() = default;

std::span<const Function> Trace::arguments() const noexcept {
  return arguments_;
}

void Trace::require_arity(std::size_t callable_arity) const {
  PLUSS_CHECK(
      callable_arity == arguments_.size(),
      "math::function: variable count " + std::to_string(arguments_.size()) +
          " does not match callable arity " + std::to_string(callable_arity));
}

Function Trace::finish(Function output) && {
  PLUSS_CHECK(output.defined(),
              "math::function: callable returned an undefined Function");
  const FunctionImpl &output_impl = FunctionAccess::get(output);
  validate_root(output_impl.root(), *state_);
  if (output_impl.trace().id() == state_->id()) {
    state_ = output_impl.trace_ptr();
  }
  return FunctionAccess::make(std::move(state_), output_impl.root_ptr(), true,
                              output_impl.depends_on_variables());
}

Function opaque(pluss::StackArray<std::string_view> variables,
                std::function<pluss::Series(const pluss::Series &)> callable) {
  pluss::msharp::Lister::StringView owned = std::move(variables).lister();
  validate_names(owned);
  PLUSS_CHECK(owned.size() == 1,
              "math::function: an opaque Series callable requires exactly "
              "one variable");
  auto trace = pluss::msharp::make_intrusive<TraceState>(
      next_trace_id.fetch_add(1, std::memory_order_relaxed), std::move(owned));
  return FunctionAccess::make(std::move(trace),
                              Node::opaque(std::move(callable), 0), true, true);
}

} // namespace pluss::subsp::math::detail

namespace pluss::subsp::math {

Function::Function() noexcept = default;
Function::Function(const Function &) noexcept = default;
Function::Function(Function &&) noexcept = default;
Function &Function::operator=(const Function &) noexcept = default;
Function &Function::operator=(Function &&) noexcept = default;
Function::~Function() = default;

Function::Function(
    pluss::msharp::intrusive_ptr<detail::FunctionImpl> impl) noexcept
    : impl_(std::move(impl)) {}

bool Function::defined() const noexcept { return static_cast<bool>(impl_); }

std::int64_t Function::arity() const {
  return static_cast<std::int64_t>(
      detail::FunctionAccess::get(*this).active_variables().size());
}

pluss::RangeView<std::string_view> Function::variables() const {
  return detail::FunctionAccess::get(*this).active_names().data();
}

std::int64_t Function::rank() const {
  return detail::FunctionAccess::get(*this).root().output().rank();
}

pluss::RangeView<std::int64_t> Function::shape() const {
  return pluss::RangeView<std::int64_t>(
      detail::FunctionAccess::get(*this).root().output().shape);
}

Provenance Function::provenance() const {
  return detail::provenance(detail::FunctionAccess::get(*this).root_ptr());
}

Function Function::arg(std::string_view variable) const {
  return detail::FunctionAccess::argument(*this, variable);
}

Function Function::assume(std::string_view variable, Assumption value) const {
  return detail::FunctionAccess::assume(*this, variable, value);
}

Assumption Function::assumption(std::string_view variable) const {
  const detail::FunctionImpl &impl = detail::FunctionAccess::get(*this);
  const auto found = impl.binding_index().find(std::string(variable));
  PLUSS_CHECK(found != impl.binding_index().end(),
              "math::Function::assumption: unknown variable '" +
                  std::string(variable) + "'");
  return impl.trace().assumptions()[found->second];
}

pluss::Series Function::operator()() const {
  return detail::FunctionAccess::evaluate(*this, {});
}

pluss::Series Function::operator()(double argument) const {
  return (*this)({pluss::make_series(argument)});
}

pluss::Series Function::operator()(float argument) const {
  return (*this)({pluss::make_series(argument)});
}

pluss::Series Function::operator()(int argument) const {
  return (*this)({pluss::make_series(argument)});
}

pluss::Series Function::operator()(std::int64_t argument) const {
  return (*this)({pluss::make_series(argument)});
}

pluss::Series Function::operator()(std::complex<double> argument) const {
  return (*this)({pluss::make_series({argument}).reshape({})});
}

pluss::Series Function::operator()(std::complex<float> argument) const {
  return (*this)({pluss::make_series({argument}).reshape({})});
}

pluss::Series Function::operator()(pluss::RangeView<Binding> bindings) const {
  const detail::FunctionImpl &impl = detail::FunctionAccess::get(*this);
  std::vector<pluss::Series> ordered(impl.active_variables().size());
  std::vector<bool> seen(ordered.size(), false);

  for (const auto &[name, value] : bindings) {
    const auto found = impl.active_binding_index().find(std::string(name));
    PLUSS_CHECK(found != impl.active_binding_index().end(),
                "math::Function: unknown variable binding '" +
                    std::string(name) + "'");
    PLUSS_CHECK(!seen[found->second],
                "math::Function: duplicate variable binding '" +
                    std::string(name) + "'");
    PLUSS_CHECK(value.defined(), "math::Function: binding '" +
                                     std::string(name) +
                                     "' must contain a defined Series");
    ordered[found->second] = value;
    seen[found->second] = true;
  }

  for (std::size_t i = 0; i < seen.size(); ++i) {
    PLUSS_CHECK(seen[i], "math::Function: missing variable binding '" +
                             std::string(impl.active_names().get(
                                 static_cast<std::int64_t>(i))) +
                             "'");
  }
  return detail::FunctionAccess::evaluate(*this, ordered);
}

pluss::Series
Function::operator()(std::initializer_list<Binding> bindings) const {
  return (*this)(pluss::RangeView<Binding>(bindings));
}

pluss::Series
Function::operator()(pluss::RangeView<pluss::Series> arguments) const {
  return detail::FunctionAccess::evaluate(*this, arguments);
}

pluss::Series
Function::operator()(std::initializer_list<pluss::Series> arguments) const {
  return (*this)(pluss::RangeView<pluss::Series>(arguments));
}

Function Function::add(const Function &rhs) const {
  return detail::FunctionAccess::binary(detail::PrimitiveId::Add, *this, rhs);
}
Function Function::add(double rhs) const { return add(function(rhs)); }
Function Function::sub(const Function &rhs) const {
  return detail::FunctionAccess::binary(detail::PrimitiveId::Sub, *this, rhs);
}
Function Function::sub(double rhs) const { return sub(function(rhs)); }
Function Function::mul(const Function &rhs) const {
  return detail::FunctionAccess::binary(detail::PrimitiveId::Mul, *this, rhs);
}
Function Function::mul(double rhs) const { return mul(function(rhs)); }
Function Function::div(const Function &rhs) const {
  return detail::FunctionAccess::binary(detail::PrimitiveId::Div, *this, rhs);
}
Function Function::div(double rhs) const { return div(function(rhs)); }
Function Function::pow(const Function &rhs) const {
  return detail::FunctionAccess::binary(detail::PrimitiveId::Pow, *this, rhs);
}
Function Function::pow(double rhs) const { return pow(function(rhs)); }

Function Function::exp() const {
  return detail::FunctionAccess::unary(detail::PrimitiveId::Exp, *this);
}
Function Function::log() const {
  return detail::FunctionAccess::unary(detail::PrimitiveId::Log, *this);
}
Function Function::log1p() const {
  return detail::FunctionAccess::unary(detail::PrimitiveId::Log1p, *this);
}
Function Function::expm1() const {
  return detail::FunctionAccess::unary(detail::PrimitiveId::Expm1, *this);
}
Function Function::sqrt() const {
  return detail::FunctionAccess::unary(detail::PrimitiveId::Sqrt, *this);
}
Function Function::sin() const {
  return detail::FunctionAccess::unary(detail::PrimitiveId::Sin, *this);
}
Function Function::cos() const {
  return detail::FunctionAccess::unary(detail::PrimitiveId::Cos, *this);
}
Function Function::tan() const {
  return detail::FunctionAccess::unary(detail::PrimitiveId::Tan, *this);
}
Function Function::sinh() const {
  return detail::FunctionAccess::unary(detail::PrimitiveId::Sinh, *this);
}
Function Function::cosh() const {
  return detail::FunctionAccess::unary(detail::PrimitiveId::Cosh, *this);
}
Function Function::tanh() const {
  return detail::FunctionAccess::unary(detail::PrimitiveId::Tanh, *this);
}
Function Function::abs() const {
  return detail::FunctionAccess::unary(detail::PrimitiveId::Abs, *this);
}
Function Function::conj() const {
  return detail::FunctionAccess::unary(detail::PrimitiveId::Conj, *this);
}
Function Function::real() const {
  return detail::FunctionAccess::unary(detail::PrimitiveId::Real, *this);
}
Function Function::imag() const {
  return detail::FunctionAccess::unary(detail::PrimitiveId::Imag, *this);
}
Function Function::angle() const {
  return detail::FunctionAccess::unary(detail::PrimitiveId::Angle, *this);
}
Function Function::matmul(const Function &rhs) const {
  return detail::FunctionAccess::tensor(detail::PrimitiveId::Contract,
                                        {*this, rhs}, {0});
}
Function Function::contract(const Function &rhs,
                            pluss::RangeView<std::int64_t> lhs_axes,
                            pluss::RangeView<std::int64_t> rhs_axes) const {
  PLUSS_CHECK(lhs_axes.size() == rhs_axes.size(),
              "math::Function::contract: both operands need the same number "
              "of contracted axes");
  std::vector<std::int64_t> attributes{rank(), rhs.rank(),
                                       static_cast<std::int64_t>(lhs_axes.size())};
  for (const std::int64_t axis : lhs_axes) attributes.push_back(axis);
  for (const std::int64_t axis : rhs_axes) attributes.push_back(axis);
  return detail::FunctionAccess::tensor(detail::PrimitiveId::Contract,
                                        {*this, rhs}, std::move(attributes));
}
Function Function::contract(const Function &rhs,
                            std::initializer_list<std::int64_t> lhs_axes,
                            std::initializer_list<std::int64_t> rhs_axes) const {
  return contract(rhs, pluss::RangeView<std::int64_t>(lhs_axes),
                  pluss::RangeView<std::int64_t>(rhs_axes));
}
Function Function::transpose() const {
  PLUSS_CHECK(rank() >= 2,
              "math::Function::transpose: needs at least two output axes");
  return transpose(rank() - 2, rank() - 1);
}
Function Function::transpose(std::int64_t axis0, std::int64_t axis1) const {
  return detail::FunctionAccess::tensor(detail::PrimitiveId::Transpose, {*this},
                                        {rank(), axis0, axis1});
}
Function Function::select(std::int64_t axis, std::int64_t index) const {
  return detail::FunctionAccess::tensor(detail::PrimitiveId::Select, {*this},
                                        {rank(), axis, index});
}
Function Function::compose(pluss::RangeView<Function> arguments) const {
  return detail::FunctionAccess::compose(*this, arguments);
}
Function Function::compose(std::initializer_list<Function> arguments) const {
  return compose(pluss::RangeView<Function>(arguments));
}
Function Function::inverse() const {
  return detail::FunctionAccess::tensor(detail::PrimitiveId::Inverse, {*this}, {});
}
Function Function::det() const {
  return detail::FunctionAccess::tensor(detail::PrimitiveId::Det, {*this}, {});
}
Function Function::trace() const {
  return detail::FunctionAccess::tensor(detail::PrimitiveId::Trace, {*this}, {});
}

Function stack(pluss::RangeView<Function> components) {
  return detail::FunctionAccess::stack(components);
}
Function stack(std::initializer_list<Function> components) {
  return detail::FunctionAccess::stack(pluss::RangeView<Function>(components));
}

Function Function::operator-() const {
  return detail::FunctionAccess::unary(detail::PrimitiveId::Neg, *this);
}

Function function(double constant) {
  return detail::FunctionAccess::make(
      detail::empty_trace(), detail::Node::scalar(constant), true, false);
}

Function function(std::complex<float> constant) {
  return function(pluss::series::complex(
      pluss::make_series(constant.real()), pluss::make_series(constant.imag()),
      pluss::ScalarType::Complex64));
}

Function function(std::complex<double> constant) {
  return function(pluss::series::complex(
      pluss::make_series(constant.real()), pluss::make_series(constant.imag()),
      pluss::ScalarType::Complex128));
}

Function function(const pluss::Series &constant) {
  PLUSS_CHECK(constant.defined(),
              "math::function: Series constant must be defined");
  return detail::FunctionAccess::make(detail::empty_trace(),
                                      detail::Node::series(constant.clone()),
                                      true, false);
}

Function operator+(const Function &lhs, const Function &rhs) {
  return lhs.add(rhs);
}
Function operator+(const Function &lhs, double rhs) { return lhs.add(rhs); }
Function operator+(double lhs, const Function &rhs) { return rhs.add(lhs); }
Function operator-(const Function &lhs, const Function &rhs) {
  return lhs.sub(rhs);
}
Function operator-(const Function &lhs, double rhs) { return lhs.sub(rhs); }
Function operator-(double lhs, const Function &rhs) {
  return function(lhs).sub(rhs);
}
Function operator*(const Function &lhs, const Function &rhs) {
  return lhs.mul(rhs);
}
Function operator*(const Function &lhs, double rhs) { return lhs.mul(rhs); }
Function operator*(double lhs, const Function &rhs) { return rhs.mul(lhs); }
Function operator/(const Function &lhs, const Function &rhs) {
  return lhs.div(rhs);
}
Function operator/(const Function &lhs, double rhs) { return lhs.div(rhs); }
Function operator/(double lhs, const Function &rhs) {
  return function(lhs).div(rhs);
}
Function operator+(const Function &lhs, std::complex<double> rhs) {
  return lhs + function(rhs);
}
Function operator+(std::complex<double> lhs, const Function &rhs) {
  return function(lhs) + rhs;
}
Function operator-(const Function &lhs, std::complex<double> rhs) {
  return lhs - function(rhs);
}
Function operator-(std::complex<double> lhs, const Function &rhs) {
  return function(lhs) - rhs;
}
Function operator*(const Function &lhs, std::complex<double> rhs) {
  return lhs * function(rhs);
}
Function operator*(std::complex<double> lhs, const Function &rhs) {
  return function(lhs) * rhs;
}
Function operator/(const Function &lhs, std::complex<double> rhs) {
  return lhs / function(rhs);
}
Function operator/(std::complex<double> lhs, const Function &rhs) {
  return function(lhs) / rhs;
}

} // namespace pluss::subsp::math
