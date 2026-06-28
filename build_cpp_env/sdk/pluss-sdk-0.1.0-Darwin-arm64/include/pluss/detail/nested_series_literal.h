#pragma once

#include <cstring>
#include <cstdint>
#include <initializer_list>
#include <optional>
#include <type_traits>
#include <vector>

#include "pluss/codem/codem.h"

namespace pluss::detail {

enum class LiteralLeafKind : std::uint8_t {
  Bool,
  UInt8,
  Int,
  Float32,
  Float64
};

struct NestedSeriesLiteral {
  enum class NodeKind : std::uint8_t { Scalar, List };

  NodeKind kind{NodeKind::List};
  double scalar{0.0};
  LiteralLeafKind leaf_kind{LiteralLeafKind::Int};
  std::vector<NestedSeriesLiteral> children;

  NestedSeriesLiteral() = default;

  NestedSeriesLiteral(bool v)
      : kind(NodeKind::Scalar),
        scalar(v ? 1.0 : 0.0),
        leaf_kind(LiteralLeafKind::Bool) {}

  template <class T, std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, bool>, int> = 0>
  NestedSeriesLiteral(T v)
      : kind(NodeKind::Scalar),
        scalar(static_cast<double>(v)),
        leaf_kind(std::is_same_v<std::remove_cv_t<T>, std::uint8_t> ? LiteralLeafKind::UInt8 : LiteralLeafKind::Int) {}

  template <class T, std::enable_if_t<std::is_floating_point_v<T>, int> = 0>
  NestedSeriesLiteral(T v)
      : kind(NodeKind::Scalar),
        scalar(static_cast<double>(v)),
        leaf_kind(std::is_same_v<std::remove_cv_t<T>, float> ? LiteralLeafKind::Float32 : LiteralLeafKind::Float64) {}

  NestedSeriesLiteral(std::initializer_list<NestedSeriesLiteral> list)
      : kind(NodeKind::List),
        scalar(0.0),
        leaf_kind(LiteralLeafKind::Int) {
    children.reserve(list.size());
    for (const auto& elem : list) children.emplace_back(elem);
  }
};

struct LiteralStats {
  bool has_bool = false;
  bool has_uint8 = false;
  bool has_int = false;
  bool has_float32 = false;
  bool has_float64 = false;
};

inline void collect_literal_stats(const NestedSeriesLiteral& node, LiteralStats& stats) {
  if (node.kind == NestedSeriesLiteral::NodeKind::Scalar) {
    switch (node.leaf_kind) {
      case LiteralLeafKind::Bool: stats.has_bool = true; break;
      case LiteralLeafKind::UInt8: stats.has_uint8 = true; break;
      case LiteralLeafKind::Int: stats.has_int = true; break;
      case LiteralLeafKind::Float32: stats.has_float32 = true; break;
      case LiteralLeafKind::Float64: stats.has_float64 = true; break;
    }
    return;
  }
  for (const auto& child : node.children) collect_literal_stats(child, stats);
}

inline void infer_shape_and_flatten(const NestedSeriesLiteral& node,
                                    std::size_t depth,
                                    std::vector<std::int64_t>& sizes,
                                    std::vector<double>& flat) {
  if (node.kind == NestedSeriesLiteral::NodeKind::Scalar) {
    flat.push_back(node.scalar);
    return;
  }

  const auto n = static_cast<std::int64_t>(node.children.size());
  if (sizes.size() == depth) sizes.push_back(n);
  else if (sizes[depth] != n) {
    pluss::codem::throw_error("pluss::value: ragged nested initializer_list is not supported");
  }

  bool has_scalar = false;
  bool has_list = false;
  for (const auto& child : node.children) {
    has_scalar = has_scalar || (child.kind == NestedSeriesLiteral::NodeKind::Scalar);
    has_list = has_list || (child.kind == NestedSeriesLiteral::NodeKind::List);
  }
  if (has_scalar && has_list) {
    pluss::codem::throw_error("pluss::value: ragged nested initializer_list is not supported");
  }

  for (const auto& child : node.children) infer_shape_and_flatten(child, depth + 1, sizes, flat);
}

inline pluss::codem::ScalarType deduce_literal_dtype(const NestedSeriesLiteral& node) {
  LiteralStats stats;
  collect_literal_stats(node, stats);

  if (stats.has_float64) return pluss::codem::ScalarType::Float64;
  if (stats.has_float32) return pluss::codem::ScalarType::Float32;
  if (stats.has_int) return pluss::codem::ScalarType::Int64;
  if (stats.has_uint8) return pluss::codem::ScalarType::UInt8;
  return pluss::codem::ScalarType::Bool;
}

inline pluss::codem::Series to_series(const NestedSeriesLiteral& literal,
                                      std::optional<pluss::codem::ScalarType> dtype = std::nullopt) {
  std::vector<std::int64_t> sizes;
  std::vector<double> flat;
  infer_shape_and_flatten(literal, 0, sizes, flat);

  if (sizes.size() > 7) {
    pluss::codem::throw_error("pluss::value: supports up to 7D nested initializer_list");
  }

  const auto out_dtype = dtype.has_value() ? *dtype : deduce_literal_dtype(literal);
  pluss::codem::Series out = pluss::codem::Series::empty(sizes, out_dtype);
  pluss::codem::Series tmp = pluss::codem::Series::empty(sizes, pluss::codem::ScalarType::Float64);
  std::memcpy(
      tmp.raw_mutable_data(),
      flat.data(),
      sizeof(double) * static_cast<std::size_t>(tmp.numel()));
  out.copy_(tmp.to(out_dtype), false);
  return out;
}

} // namespace pluss::detail
