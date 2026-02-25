#pragma once

#include "CodeM/core/ScalarType.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace pluss::codem::native::mps {

struct GraphCacheKeyView {
  std::string_view op_signature{};
  ScalarType dtype{ScalarType::Float32};
  std::span<const std::int64_t> shape{};
  std::span<const std::int64_t> aux{};
};

struct GraphCacheStats {
  std::size_t entries{0};
  std::uint64_t hits{0};
  std::uint64_t misses{0};
};

// Lookup/store an opaque payload keyed by op signature + dtype + shape/aux vectors.
bool graph_cache_lookup(const GraphCacheKeyView& key, void* out_payload, std::size_t payload_size);
void graph_cache_store(const GraphCacheKeyView& key, const void* payload, std::size_t payload_size);

GraphCacheStats graph_cache_stats() noexcept;
void clear_graph_cache() noexcept;

} // namespace pluss::codem::native::mps
