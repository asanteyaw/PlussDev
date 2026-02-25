#include "CodeM/native/mps/MPSGraphCache.h"

namespace pluss::codem::native::mps {

bool graph_cache_lookup(const GraphCacheKeyView& /*key*/, void* /*out_payload*/, std::size_t /*payload_size*/) {
  return false;
}

void graph_cache_store(const GraphCacheKeyView& /*key*/, const void* /*payload*/, std::size_t /*payload_size*/) {}

GraphCacheStats graph_cache_stats() noexcept {
  return {};
}

void clear_graph_cache() noexcept {}

} // namespace pluss::codem::native::mps
