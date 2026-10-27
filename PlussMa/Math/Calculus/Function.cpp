#include "FunctionInternal.h"

#include "pluss/error.h"
#include "pluss/subsp/math/integ.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace pluss::subsp::math::detail {

namespace {

std::atomic<std::uint64_t> next_trace_id{1};
constexpr std::size_t absolute_graph_limit = 1000000;

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
    step.inputs.reserve(order[i]->children().size());
    for (std::size_t child = 0; child < order[i]->children().size(); ++child) {
      const auto found = positions.find(order[i]->children()[child].get());
      PLUSS_CHECK(found != positions.end(),
                  "math::Function: graph plan is not topologically ordered");
      step.inputs.push_back(found->second);
    }
    plan.push_back(std::move(step));
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
  PLUSS_CHECK(output.rank() <= 2,
              "math::Function: outputs are limited to scalar, vector, or "
              "matrix shape");
  return Node::primitive(PrimitiveId::Stack, std::move(children),
                         std::move(output));
}

bool depends_on(const NodePtr &node, std::int64_t variable_id,
                std::unordered_map<const Node *, bool> &memo) {
  if (const auto found = memo.find(node.get()); found != memo.end()) {
    return found->second;
  }
  bool result = node->kind() == NodeKind::Projection &&
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
    if (children.size() == 1) {
      return unary_node(node->primitive_id(), std::move(children[0]));
    }
    return binary_node(node->primitive_id(), std::move(children[0]),
                       std::move(children[1]));
  }
  if (node->kind() == NodeKind::Integral) {
    return Node::integral(std::move(children[0]),
                          node->integral_variable_id(), node->integral_lower(),
                          node->integral_upper());
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
      return binary_node(
          PrimitiveId::Div,
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
          return binary_node(PrimitiveId::Mul, children[0],
                             std::move(*right));
        }
      }
      if (!depends_on(children[1], variable_id_)) {
        std::optional<NodePtr> left = apply(children[0]);
        if (left) {
          return binary_node(PrimitiveId::Mul, std::move(*left),
                             children[1]);
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
        return binary_node(
            PrimitiveId::Div,
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
                 ? std::optional<NodePtr>(unary_node(
                       PrimitiveId::Neg,
                       unary_node(PrimitiveId::Cos, children[0])))
                 : std::nullopt;
    case PrimitiveId::Cos:
      return is_variable(children[0])
                 ? std::optional<NodePtr>(
                       unary_node(PrimitiveId::Sin, children[0]))
                 : std::nullopt;
    case PrimitiveId::Sinh:
      return is_variable(children[0])
                 ? std::optional<NodePtr>(
                       unary_node(PrimitiveId::Cosh, children[0]))
                 : std::nullopt;
    case PrimitiveId::Cosh:
      return is_variable(children[0])
                 ? std::optional<NodePtr>(
                       unary_node(PrimitiveId::Sinh, children[0]))
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
  if (primitive_id == PrimitiveId::Sqrt &&
      input->kind() == NodeKind::Primitive &&
      input->primitive_id() == PrimitiveId::Pow &&
      scalar_is(input->children()[1], 2.0) &&
      known_nonnegative(input->children()[0], trace)) {
    return input->children()[0];
  }
  return unary_node(primitive_id, std::move(input));
}

NodePtr simplify_binary(PrimitiveId primitive_id, NodePtr left, NodePtr right,
                        const TraceState &trace) {
  if (primitive_id == PrimitiveId::Div && left.get() == right.get() &&
      known_nonzero(left, trace)) {
    return unary_node(PrimitiveId::OnesLike, left);
  }
  return binary_node(primitive_id, std::move(left), std::move(right));
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
        result = Node::integral(std::move(children[0]),
                                node->integral_variable_id(),
                                node->integral_lower(), node->integral_upper());
      } else if (node->primitive_id() == PrimitiveId::Stack) {
        result = stack_nodes(std::move(children));
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
  explicit Differentiator(NonSmooth nonsmooth) : nonsmooth_(nonsmooth) {}

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
      result = apply(apply(node, variable_id, order - 1), variable_id, 1);
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
    case PrimitiveId::Abs:
      switch (nonsmooth_) {
      case NonSmooth::Strict:
        return binary_node(PrimitiveId::Mul,
                           unary_node(PrimitiveId::AbsSlopeStrict, u), du);
      case NonSmooth::Piecewise:
        return binary_node(PrimitiveId::Mul,
                           unary_node(PrimitiveId::AbsSlopePiecewise, u), du);
      case NonSmooth::Subgradient:
      case NonSmooth::Autodiff:
        return binary_node(PrimitiveId::Mul,
                           unary_node(PrimitiveId::AbsSlopeZero, u), du);
      }
      PLUSS_FAIL("operators::differential: unknown non-smooth policy");
    case PrimitiveId::ZerosLike:
    case PrimitiveId::OnesLike:
    case PrimitiveId::AbsSlopeStrict:
    case PrimitiveId::AbsSlopeZero:
    case PrimitiveId::AbsSlopePiecewise:
      return unary_node(PrimitiveId::ZerosLike, node);
    case PrimitiveId::Stack:
      PLUSS_FAIL("operators::differential: unreachable stack derivative");
    }
    PLUSS_FAIL("operators::differential: missing primitive derivative rule");
  }

  std::unordered_map<DerivativeKey, NodePtr, DerivativeKeyHash> memo_{};
  NonSmooth nonsmooth_ = NonSmooth::Strict;
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

} // namespace

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
Node::primitive(PrimitiveId primitive_id,
                std::vector<pluss::msharp::intrusive_ptr<Node>> children,
                OutputMetadata output) {
  const PrimitiveSpec &spec = detail::primitive(primitive_id);
  PLUSS_CHECK(spec.variadic ? !children.empty() : children.size() == spec.arity,
              "math::Function: primitive graph arity mismatch");
  return pluss::msharp::intrusive_ptr<Node>(
      pluss::msharp::intrusive_ptr<Node>::adopt,
      new Node(NodeKind::Primitive, 0, -1, 0.0, pluss::Series{}, primitive_id,
               std::move(children), std::move(output)));
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
                           bool depends_on_variables)
    : trace_(std::move(trace)), root_(std::move(root)), sealed_(sealed),
      depends_on_variables_(depends_on_variables) {
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
}

Function FunctionAccess::make(pluss::msharp::intrusive_ptr<TraceState> trace,
                              pluss::msharp::intrusive_ptr<Node> root,
                              bool sealed, bool depends_on_variables) {
  PLUSS_CHECK(
      trace && root,
      "math::Function: internal graph must have trace and root storage");
  return Function(pluss::msharp::make_intrusive<FunctionImpl>(
      std::move(trace), std::move(root), sealed, depends_on_variables));
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
              impl.sealed(), impl.depends_on_variables());
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
      lhs.depends_on_variables() || rhs.depends_on_variables());
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
              impl.sealed(), impl.depends_on_variables());
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
              impl.depends_on_variables());
}

Function FunctionAccess::differentiate(const Function &function,
                                       std::string_view variable,
                                       std::int64_t order, NonSmooth nonsmooth,
                                       std::size_t max_nodes) {
  const FunctionImpl &impl = get(function);
  PLUSS_CHECK(order > 0, "operators::differential: order must be positive");
  const auto found = impl.binding_index().find(std::string(variable));
  PLUSS_CHECK(found != impl.binding_index().end(),
              "operators::differential: unknown variable '" +
                  std::string(variable) + "'");
  Differentiator differentiator(nonsmooth);
  NodePtr root = differentiator.apply(
      impl.root_ptr(), static_cast<std::int64_t>(found->second), order);
  require_graph_limit(root, max_nodes, "operators::differential");
  return make(impl.trace_ptr(), std::move(root), true,
              impl.depends_on_variables());
}

Function FunctionAccess::gradient(const Function &function,
                                  pluss::RangeView<std::string_view> variables,
                                  NonSmooth nonsmooth, std::size_t max_nodes) {
  const FunctionImpl &impl = get(function);
  PLUSS_CHECK(impl.root().output().rank() == 0,
              "operators::gradient: input Function must be scalar-valued");
  const std::vector<std::int64_t> selected =
      resolve_variables(impl, variables, "operators::gradient");
  Differentiator differentiator(nonsmooth);
  std::vector<NodePtr> components;
  components.reserve(selected.size());
  for (const std::int64_t variable : selected) {
    components.push_back(differentiator.apply(impl.root_ptr(), variable, 1));
  }
  NodePtr root = stack_nodes(std::move(components));
  require_graph_limit(root, max_nodes, "operators::gradient");
  return make(impl.trace_ptr(), std::move(root), true,
              impl.depends_on_variables());
}

Function FunctionAccess::jacobian(const Function &function,
                                  pluss::RangeView<std::string_view> variables,
                                  NonSmooth nonsmooth, std::size_t max_nodes) {
  const FunctionImpl &impl = get(function);
  PLUSS_CHECK(impl.root().output().rank() <= 1,
              "operators::jacobian: input Function must be scalar- or "
              "vector-valued");
  const std::vector<std::int64_t> selected =
      resolve_variables(impl, variables, "operators::jacobian");
  Differentiator differentiator(nonsmooth);
  std::vector<NodePtr> columns;
  columns.reserve(selected.size());
  for (const std::int64_t variable : selected) {
    columns.push_back(differentiator.apply(impl.root_ptr(), variable, 1));
  }
  NodePtr root = stack_nodes(std::move(columns));
  require_graph_limit(root, max_nodes, "operators::jacobian");
  return make(impl.trace_ptr(), std::move(root), true,
              impl.depends_on_variables());
}

Function FunctionAccess::hessian(const Function &function,
                                 pluss::RangeView<std::string_view> variables,
                                 NonSmooth nonsmooth, std::size_t max_nodes) {
  const FunctionImpl &impl = get(function);
  PLUSS_CHECK(impl.root().output().rank() == 0,
              "operators::hessian: input Function must be scalar-valued");
  const std::vector<std::int64_t> selected =
      resolve_variables(impl, variables, "operators::hessian");
  Differentiator differentiator(nonsmooth);
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
              impl.depends_on_variables());
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
  NodePtr root = integrator.apply(impl.root_ptr()).value_or(
      Node::integral(impl.root_ptr(), variable_id, 0.0));
  return make(impl.trace_ptr(), std::move(root), true,
              impl.depends_on_variables());
}

Function FunctionAccess::integral(const Function &function,
                                  std::string_view variable, double lower,
                                  std::optional<double> upper) {
  const FunctionImpl &impl = get(function);
  PLUSS_CHECK(std::isfinite(lower),
              "operators::integral: lower bound must be finite");
  PLUSS_CHECK(!upper || std::isfinite(*upper),
              "operators::integral: upper bound must be finite");
  const auto found = impl.binding_index().find(std::string(variable));
  PLUSS_CHECK(found != impl.binding_index().end(),
              "operators::integral: unknown variable '" +
                  std::string(variable) + "'");
  const auto variable_id = static_cast<std::int64_t>(found->second);
  Integrator integrator(impl.trace().id(), variable_id);
  NodePtr root;
  if (std::optional<NodePtr> primitive = integrator.apply(impl.root_ptr())) {
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
  return make(impl.trace_ptr(), std::move(root), true,
              impl.depends_on_variables());
}

pluss::Series
FunctionAccess::evaluate(const Function &function,
                         pluss::RangeView<pluss::Series> arguments) {
  const FunctionImpl &impl = get(function);
  PLUSS_CHECK(
      impl.sealed(),
      "math::Function: an unsealed trace projection cannot be evaluated");
  PLUSS_CHECK(arguments.size() == impl.trace().variables().size(),
              "math::Function: ordered argument count " +
                  std::to_string(arguments.size()) +
                  " does not match function arity " +
                  std::to_string(impl.trace().variables().size()));

  const pluss::Series *like = arguments.empty() ? nullptr : &arguments[0];
  for (const pluss::Series &argument : arguments) {
    PLUSS_CHECK(argument.defined(),
                "math::Function: arguments must be defined Series values");
    PLUSS_CHECK(!like || argument.platform() == like->platform(),
                "math::Function: all arguments must use the same platform");
  }

  std::vector<pluss::Series> values;
  values.reserve(impl.plan().size());
  for (const ExecutionStep &step : impl.plan()) {
    const Node &node = *step.node;
    switch (node.kind()) {
    case NodeKind::Projection:
      values.push_back(arguments[static_cast<std::size_t>(node.variable_id())]);
      break;
    case NodeKind::ScalarConstant:
      values.push_back(
          like ? pluss::codem::make_scalar_like(*like, node.scalar_value())
               : pluss::Series::full({}, node.scalar_value(),
                                     pluss::ScalarType::Float64, pluss::pCPU));
      break;
    case NodeKind::SeriesConstant: {
      pluss::Series value = node.series_value().clone();
      if (like && value.platform() != like->platform()) {
        value = value.to(like->platform());
      }
      values.push_back(std::move(value));
      break;
    }
    case NodeKind::Integral: {
      const std::int64_t variable_id = node.integral_variable_id();
      PLUSS_CHECK(variable_id >= 0 &&
                      static_cast<std::size_t>(variable_id) < arguments.size(),
                  "math::Function: integral variable binding is out of range");
      double upper = 0.0;
      if (node.integral_upper()) {
        upper = *node.integral_upper();
      } else {
        PLUSS_CHECK(arguments[static_cast<std::size_t>(variable_id)].numel() ==
                        1,
                    "math::Function: unresolved anchored integrals require "
                    "a scalar upper-bound argument");
        upper = arguments[static_cast<std::size_t>(variable_id)]
                    .to(pluss::pCPU, pluss::ScalarType::Float64)
                    .item<double>();
      }
      std::vector<pluss::Series> bound(arguments.begin(), arguments.end());
      for (std::size_t i = 0; i < bound.size(); ++i) {
        if (i != static_cast<std::size_t>(variable_id) && bound[i].dim() > 0) {
          bound[i] = bound[i].unsqueeze(-1);
        }
      }
      const Function integrand =
          make(impl.trace_ptr(), node.children()[0], true, true);
      integ::QuadraOptions options;
      options.dtype = arguments[static_cast<std::size_t>(variable_id)].dtype();
      options.platform =
          arguments[static_cast<std::size_t>(variable_id)].platform();
      const integ::QuadraResult result = integ::quadra(
          [integrand, bound = std::move(bound),
           variable_id](const pluss::Series &nodes) mutable {
            bound[static_cast<std::size_t>(variable_id)] = nodes;
            return FunctionAccess::evaluate(integrand, bound);
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
      std::vector<pluss::Series> inputs;
      inputs.reserve(step.inputs.size());
      for (const std::size_t input : step.inputs) {
        inputs.push_back(values[input]);
      }
      pluss::Series value = spec.evaluate(inputs);
      PLUSS_CHECK(value.defined(), "math::Function: primitive '" +
                                       std::string(spec.name) +
                                       "' returned an undefined Series");
      values.push_back(std::move(value));
      break;
    }
    }
  }
  PLUSS_CHECK(!values.empty(),
              "math::Function: compiled execution plan is empty");
  return values.back();
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
      detail::FunctionAccess::get(*this).trace().variables().size());
}

pluss::RangeView<std::string_view> Function::variables() const {
  return detail::FunctionAccess::get(*this).trace().variables().data();
}

std::int64_t Function::rank() const {
  return detail::FunctionAccess::get(*this).root().output().rank();
}

pluss::RangeView<std::int64_t> Function::shape() const {
  return pluss::RangeView<std::int64_t>(
      detail::FunctionAccess::get(*this).root().output().shape);
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

pluss::Series Function::operator()(pluss::RangeView<Binding> bindings) const {
  const detail::FunctionImpl &impl = detail::FunctionAccess::get(*this);
  std::vector<pluss::Series> ordered(impl.trace().variables().size());
  std::vector<bool> seen(ordered.size(), false);

  for (const auto &[name, value] : bindings) {
    const auto found = impl.binding_index().find(std::string(name));
    PLUSS_CHECK(found != impl.binding_index().end(),
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
                             std::string(impl.trace().variables().get(
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
Function Function::operator-() const {
  return detail::FunctionAccess::unary(detail::PrimitiveId::Neg, *this);
}

Function function(double constant) {
  return detail::FunctionAccess::make(
      detail::empty_trace(), detail::Node::scalar(constant), true, false);
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

} // namespace pluss::subsp::math
