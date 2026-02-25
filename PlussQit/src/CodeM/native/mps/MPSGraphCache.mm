#include "CodeM/native/mps/MPSGraphCache.h"

#include <cstring>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace pluss::codem::native::mps {
namespace {

struct GraphCacheKey {
  std::string op_signature{};
  ScalarType dtype{ScalarType::Float32};
  std::vector<std::int64_t> shape{};
  std::vector<std::int64_t> aux{};

  bool operator==(const GraphCacheKey& other) const noexcept {
    return op_signature == other.op_signature && dtype == other.dtype && shape == other.shape && aux == other.aux;
  }
};

struct GraphCacheKeyHash {
  std::size_t operator()(const GraphCacheKey& key) const noexcept {
    std::size_t h = std::hash<std::string>{}(key.op_signature);
    h ^= (static_cast<std::size_t>(key.dtype) + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2));
    for (const auto v : key.shape) {
      const std::size_t x = std::hash<std::int64_t>{}(v);
      h ^= (x + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2));
    }
    for (const auto v : key.aux) {
      const std::size_t x = std::hash<std::int64_t>{}(v);
      h ^= (x + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2));
    }
    return h;
  }
};

struct GraphCacheValue {
  std::vector<std::byte> payload{};
};

struct GraphCacheState {
  std::unordered_map<GraphCacheKey, GraphCacheValue, GraphCacheKeyHash> map{};
  std::uint64_t hits{0};
  std::uint64_t misses{0};
};

std::mutex& cache_mu_() {
  static std::mutex mu;
  return mu;
}

GraphCacheState& cache_() {
  static GraphCacheState c;
  return c;
}

GraphCacheKey materialize_(const GraphCacheKeyView& key) {
  GraphCacheKey out;
  out.op_signature = std::string(key.op_signature);
  out.dtype = key.dtype;
  out.shape.assign(key.shape.begin(), key.shape.end());
  out.aux.assign(key.aux.begin(), key.aux.end());
  return out;
}

} // namespace

bool graph_cache_lookup(const GraphCacheKeyView& key, void* out_payload, std::size_t payload_size) {
  if (out_payload == nullptr || payload_size == 0) {
    return false;
  }

  std::scoped_lock lock(cache_mu_());
  auto& c = cache_();
  auto it = c.map.find(materialize_(key));
  if (it == c.map.end()) {
    ++c.misses;
    return false;
  }
  if (it->second.payload.size() != payload_size) {
    ++c.misses;
    return false;
  }

  std::memcpy(out_payload, it->second.payload.data(), payload_size);
  ++c.hits;
  return true;
}

void graph_cache_store(const GraphCacheKeyView& key, const void* payload, std::size_t payload_size) {
  if (payload == nullptr || payload_size == 0) {
    return;
  }

  GraphCacheValue value;
  value.payload.resize(payload_size);
  std::memcpy(value.payload.data(), payload, payload_size);

  std::scoped_lock lock(cache_mu_());
  cache_().map[materialize_(key)] = std::move(value);
}

GraphCacheStats graph_cache_stats() noexcept {
  std::scoped_lock lock(cache_mu_());
  const auto& c = cache_();
  return GraphCacheStats{c.map.size(), c.hits, c.misses};
}

void clear_graph_cache() noexcept {
  std::scoped_lock lock(cache_mu_());
  auto& c = cache_();
  c.map.clear();
  c.hits = 0;
  c.misses = 0;
}

} // namespace pluss::codem::native::mps
