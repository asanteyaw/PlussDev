#pragma once

#include <cstdint>
#include <initializer_list>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "CodeM/core/Errors.h"

namespace pluss::detail {

struct NestedTimelineLiteral {
  enum class NodeKind : std::uint8_t { Scalar, List };

  NodeKind kind{NodeKind::List};
  std::string scalar;
  std::vector<NestedTimelineLiteral> children;

  NestedTimelineLiteral() = default;

  NestedTimelineLiteral(const char* v)
      : kind(NodeKind::Scalar),
        scalar(v ? v : "") {}

  NestedTimelineLiteral(std::string_view v)
      : kind(NodeKind::Scalar),
        scalar(v) {}

  NestedTimelineLiteral(const std::string& v)
      : kind(NodeKind::Scalar),
        scalar(v) {}

  NestedTimelineLiteral(std::string&& v)
      : kind(NodeKind::Scalar),
        scalar(std::move(v)) {}

  NestedTimelineLiteral(std::initializer_list<NestedTimelineLiteral> list)
      : kind(NodeKind::List) {
    children.reserve(list.size());
    for (const auto& elem : list) children.emplace_back(elem);
  }
};

struct FlattenedTimelineLiteral {
  std::vector<std::int64_t> sizes;
  std::vector<std::string> flat;
};

inline void infer_shape_and_flatten(const NestedTimelineLiteral& node,
                                    std::size_t depth,
                                    std::int64_t max_rank,
                                    std::string_view where,
                                    std::vector<std::int64_t>& sizes,
                                    std::vector<std::string>& flat) {
  if (node.kind == NestedTimelineLiteral::NodeKind::Scalar) {
    flat.push_back(node.scalar);
    return;
  }

  CODEM_CHECK(depth < static_cast<std::size_t>(max_rank),
              std::string(where) + ": supports up to " + std::to_string(max_rank) + "D nested initializer_list");

  const auto n = static_cast<std::int64_t>(node.children.size());
  if (sizes.size() == depth) sizes.push_back(n);
  else if (sizes[depth] != n) {
    CODEM_FAIL(std::string(where) + ": ragged nested initializer_list is not supported");
  }

  bool has_scalar = false;
  bool has_list = false;
  for (const auto& child : node.children) {
    has_scalar = has_scalar || (child.kind == NestedTimelineLiteral::NodeKind::Scalar);
    has_list = has_list || (child.kind == NestedTimelineLiteral::NodeKind::List);
  }
  if (has_scalar && has_list) {
    CODEM_FAIL(std::string(where) + ": ragged nested initializer_list is not supported");
  }

  for (const auto& child : node.children) {
    infer_shape_and_flatten(child, depth + 1, max_rank, where, sizes, flat);
  }
}

inline FlattenedTimelineLiteral flatten_timeline_literal(
    std::initializer_list<NestedTimelineLiteral> list,
    std::int64_t max_rank,
    std::string_view where) {
  FlattenedTimelineLiteral out;
  const NestedTimelineLiteral root(list);
  infer_shape_and_flatten(root, 0, max_rank, where, out.sizes, out.flat);
  return out;
}

struct NestedInt64Literal {
  enum class NodeKind : std::uint8_t { Scalar, List };

  NodeKind kind{NodeKind::List};
  std::int64_t scalar{0};
  std::vector<NestedInt64Literal> children;

  NestedInt64Literal() = default;

  template <class T, std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<std::remove_cv_t<T>, bool>, int> = 0>
  NestedInt64Literal(T v)
      : kind(NodeKind::Scalar),
        scalar(static_cast<std::int64_t>(v)) {}

  NestedInt64Literal(std::initializer_list<NestedInt64Literal> list)
      : kind(NodeKind::List) {
    children.reserve(list.size());
    for (const auto& elem : list) children.emplace_back(elem);
  }
};

struct FlattenedInt64Literal {
  std::vector<std::int64_t> sizes;
  std::vector<std::int64_t> flat;
};

inline void infer_shape_and_flatten(const NestedInt64Literal& node,
                                    std::size_t depth,
                                    std::int64_t max_rank,
                                    std::string_view where,
                                    std::vector<std::int64_t>& sizes,
                                    std::vector<std::int64_t>& flat) {
  if (node.kind == NestedInt64Literal::NodeKind::Scalar) {
    flat.push_back(node.scalar);
    return;
  }

  CODEM_CHECK(depth < static_cast<std::size_t>(max_rank),
              std::string(where) + ": supports up to " + std::to_string(max_rank) + "D nested initializer_list");

  const auto n = static_cast<std::int64_t>(node.children.size());
  if (sizes.size() == depth) sizes.push_back(n);
  else if (sizes[depth] != n) {
    CODEM_FAIL(std::string(where) + ": ragged nested initializer_list is not supported");
  }

  bool has_scalar = false;
  bool has_list = false;
  for (const auto& child : node.children) {
    has_scalar = has_scalar || (child.kind == NestedInt64Literal::NodeKind::Scalar);
    has_list = has_list || (child.kind == NestedInt64Literal::NodeKind::List);
  }
  if (has_scalar && has_list) {
    CODEM_FAIL(std::string(where) + ": ragged nested initializer_list is not supported");
  }

  for (const auto& child : node.children) {
    infer_shape_and_flatten(child, depth + 1, max_rank, where, sizes, flat);
  }
}

inline FlattenedInt64Literal flatten_int64_literal(
    std::initializer_list<NestedInt64Literal> list,
    std::int64_t max_rank,
    std::string_view where) {
  FlattenedInt64Literal out;
  const NestedInt64Literal root(list);
  infer_shape_and_flatten(root, 0, max_rank, where, out.sizes, out.flat);
  return out;
}

} // namespace pluss::detail
