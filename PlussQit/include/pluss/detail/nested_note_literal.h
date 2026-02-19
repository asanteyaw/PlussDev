#pragma once

#include <cstdint>
#include <initializer_list>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "CodeM/core/Errors.h"

namespace pluss::detail {

struct NestedNoteLiteral {
  enum class NodeKind : std::uint8_t { Scalar, List };

  NodeKind kind{NodeKind::List};
  std::string scalar;
  std::vector<NestedNoteLiteral> children;

  NestedNoteLiteral() = default;

  NestedNoteLiteral(const char* v)
      : kind(NodeKind::Scalar),
        scalar(v ? v : "") {}

  NestedNoteLiteral(std::string_view v)
      : kind(NodeKind::Scalar),
        scalar(v) {}

  NestedNoteLiteral(const std::string& v)
      : kind(NodeKind::Scalar),
        scalar(v) {}

  NestedNoteLiteral(std::string&& v)
      : kind(NodeKind::Scalar),
        scalar(std::move(v)) {}

#if defined(__cpp_char8_t)
  NestedNoteLiteral(const char8_t* v)
      : kind(NodeKind::Scalar),
        scalar(v ? reinterpret_cast<const char*>(v) : "") {}

  NestedNoteLiteral(std::u8string_view v)
      : kind(NodeKind::Scalar),
        scalar(reinterpret_cast<const char*>(v.data()), v.size()) {}

  NestedNoteLiteral(const std::u8string& v)
      : kind(NodeKind::Scalar),
        scalar(reinterpret_cast<const char*>(v.data()), v.size()) {}

  NestedNoteLiteral(std::u8string&& v)
      : kind(NodeKind::Scalar),
        scalar(reinterpret_cast<const char*>(v.data()), v.size()) {}
#endif

  NestedNoteLiteral(std::initializer_list<NestedNoteLiteral> list)
      : kind(NodeKind::List) {
    children.reserve(list.size());
    for (const auto& elem : list) children.emplace_back(elem);
  }
};

struct FlattenedNoteLiteral {
  std::vector<std::int64_t> sizes;
  std::vector<std::string> flat;
};

inline void infer_shape_and_flatten(const NestedNoteLiteral& node,
                                    std::size_t depth,
                                    std::int64_t max_rank,
                                    std::string_view where,
                                    std::vector<std::int64_t>& sizes,
                                    std::vector<std::string>& flat) {
  if (node.kind == NestedNoteLiteral::NodeKind::Scalar) {
    flat.push_back(node.scalar);
    return;
  }

  PLUSS_CHECK(depth < static_cast<std::size_t>(max_rank),
              std::string(where) + ": supports up to " + std::to_string(max_rank) + "D nested initializer_list");

  const auto n = static_cast<std::int64_t>(node.children.size());
  if (sizes.size() == depth) sizes.push_back(n);
  else if (sizes[depth] != n) {
    PLUSS_CHECK(false, std::string(where) + ": ragged nested initializer_list is not supported");
  }

  bool has_scalar = false;
  bool has_list = false;
  for (const auto& child : node.children) {
    has_scalar = has_scalar || (child.kind == NestedNoteLiteral::NodeKind::Scalar);
    has_list = has_list || (child.kind == NestedNoteLiteral::NodeKind::List);
  }
  if (has_scalar && has_list) {
    PLUSS_CHECK(false, std::string(where) + ": ragged nested initializer_list is not supported");
  }

  for (const auto& child : node.children) {
    infer_shape_and_flatten(child, depth + 1, max_rank, where, sizes, flat);
  }
}

inline FlattenedNoteLiteral flatten_note_literal(
    std::initializer_list<NestedNoteLiteral> list,
    std::int64_t max_rank,
    std::string_view where) {
  FlattenedNoteLiteral out;
  const NestedNoteLiteral root(list);
  infer_shape_and_flatten(root, 0, max_rank, where, out.sizes, out.flat);
  return out;
}

} // namespace pluss::detail
