#include <atomic>
#include "CodeM/core/Storage.h"
#include "CodeM/core/Errors.h"
#include "CodeM/native/cuda/CUDARuntime.h"

#include <cctype>
#include <cstdlib>
#include <unordered_map>
#include <utility>
#include <vector>

#if defined(_WIN32)
#include <malloc.h>
#endif

namespace pluss::codem {

namespace {

struct CacheKey {
  std::size_t alloc_size{0};
  std::size_t alignment{0};
  bool operator==(const CacheKey& other) const noexcept {
    return alloc_size == other.alloc_size && alignment == other.alignment;
  }
};

struct CacheKeyHash {
  std::size_t operator()(const CacheKey& k) const noexcept {
    const auto h1 = std::hash<std::size_t>{}(k.alloc_size);
    const auto h2 = std::hash<std::size_t>{}(k.alignment);
    return h1 ^ (h2 + 0x9e3779b97f4a7c15ULL + (h1 << 6) + (h1 >> 2));
  }
};

struct ThreadLocalPool {
  std::unordered_map<CacheKey, std::vector<void*>, CacheKeyHash> blocks;
  std::size_t cached_bytes{0};
};

static bool ieq_token_(const char* s, const char* lit) {
  if (!s || !lit) return false;
  while (*s && *lit) {
    const auto a = static_cast<unsigned char>(*s);
    const auto b = static_cast<unsigned char>(*lit);
    if (std::tolower(a) != std::tolower(b)) return false;
    ++s;
    ++lit;
  }
  return *s == '\0' && *lit == '\0';
}

static bool pool_disabled_from_env_() {
  const char* raw = std::getenv("PLUSS_CODEM_STORAGE_POOL");
  if (!raw) return false;
  while (*raw && std::isspace(static_cast<unsigned char>(*raw))) ++raw;
  if (*raw == '\0') return false;
  if (ieq_token_(raw, "0")) return true;
  if (ieq_token_(raw, "off")) return true;
  if (ieq_token_(raw, "no")) return true;
  if (ieq_token_(raw, "false")) return true;
  if (ieq_token_(raw, "disable")) return true;
  if (ieq_token_(raw, "disabled")) return true;
  return false;
}

static std::atomic<int> g_pool_state{-1};  // -1 unknown, 0 disabled, 1 enabled

static bool storage_pool_compute_() {
#if defined(__has_feature)
#if __has_feature(address_sanitizer)
  return false;
#endif
#endif
#if defined(__SANITIZE_ADDRESS__)
  return false;
#endif
  return !pool_disabled_from_env_();
}

static bool storage_pool_enabled_() {
  int s = g_pool_state.load(std::memory_order_relaxed);
  if (s < 0) {
    s = storage_pool_compute_() ? 1 : 0;
    g_pool_state.store(s, std::memory_order_relaxed);
  }
  return s == 1;
}

inline ThreadLocalPool& tls_pool_() {
  // Keep pool alive for process lifetime to avoid static destruction ordering issues.
  thread_local auto* pool = new ThreadLocalPool();
  return *pool;
}

constexpr std::size_t kPerThreadBytesCap = std::size_t{128} * 1024 * 1024;
constexpr std::size_t kPerKeyBlockCap = 16;

static void* aligned_allocate_(std::size_t size, std::size_t alignment) {
#if defined(_WIN32)
  return ::_aligned_malloc(size, alignment);
#else
  return std::aligned_alloc(alignment, size);
#endif
}

static void aligned_free_(void* p) noexcept {
#if defined(_WIN32)
  ::_aligned_free(p);
#else
  std::free(p);
#endif
}

static void* pool_take_(std::size_t alloc_size, std::size_t alignment) {
  if (!storage_pool_enabled_()) return nullptr;
  auto& pool = tls_pool_();
  const CacheKey key{alloc_size, alignment};
  auto it = pool.blocks.find(key);
  if (it == pool.blocks.end() || it->second.empty()) return nullptr;
  void* p = it->second.back();
  it->second.pop_back();
  if (it->second.empty()) pool.blocks.erase(it);
  pool.cached_bytes -= alloc_size;
  return p;
}

static void pool_put_(void* p, std::size_t alloc_size, std::size_t alignment) noexcept {
  if (!p) return;
  if (!storage_pool_enabled_()) {
    aligned_free_(p);
    return;
  }
  if (alloc_size == 0 || alignment == 0) {
    aligned_free_(p);
    return;
  }
  if (alloc_size > kPerThreadBytesCap / 2) {
    aligned_free_(p);
    return;
  }
  auto& pool = tls_pool_();
  const CacheKey key{alloc_size, alignment};
  auto& bucket = pool.blocks[key];
  if (bucket.size() >= kPerKeyBlockCap || pool.cached_bytes + alloc_size > kPerThreadBytesCap) {
    aligned_free_(p);
    return;
  }
  bucket.push_back(p);
  pool.cached_bytes += alloc_size;
}

} // namespace

void Storage::reload_pool_env() noexcept { g_pool_state.store(-1, std::memory_order_relaxed); }

void Storage::AlignedDeleter::operator()(std::byte* p) const noexcept {
  pool_put_(p, alloc_size, alignment);
}

static std::size_t round_up(std::size_t x, std::size_t a) { return (x + (a - 1)) / a * a; }

Storage::Storage(std::size_t nbytes, std::size_t alignment)
  : version_state_(std::make_shared<VersionState>()), nbytes_(nbytes), alignment_(alignment) {
  if (nbytes == 0) return;
  if (alignment == 0) alignment = 64;
  alignment_ = alignment;
  const std::size_t alloc_size = round_up(nbytes, alignment);

  void* p = pool_take_(alloc_size, alignment);
  if (!p) p = aligned_allocate_(alloc_size, alignment);
  if (!p) throw_error("Storage: aligned_alloc failed");
  data_ = std::shared_ptr<std::byte>(reinterpret_cast<std::byte*>(p), AlignedDeleter{alloc_size, alignment});
}

Storage Storage::cuda_device(std::size_t nbytes) {
  Storage storage;
  storage.version_state_ = std::make_shared<VersionState>();
  storage.nbytes_ = nbytes;
  storage.alignment_ = 1;
  storage.location_ = StorageLocation::CudaDevice;
  if (nbytes == 0) {
    return storage;
  }
  void* p = nullptr;
  if (!native::cuda::allocate_device(&p, nbytes) || p == nullptr) {
    throw_error("Storage::cuda_device: cuda device allocation failed");
  }
  storage.data_ = std::shared_ptr<std::byte>(
      reinterpret_cast<std::byte*>(p),
      [](std::byte* q) noexcept {
        (void)native::cuda::free_device(q);
      });
  return storage;
}

Storage::Storage(std::shared_ptr<std::byte> external, std::size_t nbytes, std::size_t alignment)
  : data_(std::move(external)),
    version_state_(std::make_shared<VersionState>()),
    nbytes_(nbytes),
    alignment_(alignment) {}

std::int64_t Storage::version() const noexcept {
  return version_state_ ? version_state_->value.load(std::memory_order_relaxed) : 0;
}

void Storage::bump_version() const noexcept {
  if (version_state_) {
    version_state_->value.fetch_add(1, std::memory_order_relaxed);
  }
}

} // namespace pluss::codem
