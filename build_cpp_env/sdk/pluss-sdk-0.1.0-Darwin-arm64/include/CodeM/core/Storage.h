#pragma once
#include "pluss/export.h"
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>

namespace pluss::codem {

enum class StorageLocation : std::uint8_t {
  Host = 0,
  CudaDevice = 1,
};

// Shareable storage handle. Host storage is directly addressable; device storage
// must be accessed through backend copy/kernel APIs.
class CODEM_API Storage final {
public:
  Storage() = default;
  explicit Storage(std::size_t nbytes, std::size_t alignment = 64);
  static Storage cuda_device(std::size_t nbytes);

  // Adopt externally-managed memory (kept alive via shared_ptr).
  Storage(std::shared_ptr<std::byte> external, std::size_t nbytes, std::size_t alignment = 1);

  std::byte* data() noexcept { return data_.get(); }
  const std::byte* data() const noexcept { return data_.get(); }
  std::size_t nbytes() const noexcept { return nbytes_; }
  std::size_t alignment() const noexcept { return alignment_; }
  StorageLocation location() const noexcept { return location_; }
  bool host_accessible() const noexcept { return location_ == StorageLocation::Host; }
  bool defined() const noexcept { return static_cast<bool>(data_); }
  std::shared_ptr<std::byte> handle() const noexcept { return data_; }
  std::int64_t version() const noexcept;
  void bump_version() const noexcept;

  // The allocation pool reads PLUSS_CODEM_STORAGE_POOL once and caches it (it sits on the allocation hot
  // path of every Series op); call this after changing the variable at run time.
  static void reload_pool_env() noexcept;

private:
  struct VersionState {
    std::atomic<std::int64_t> value{0};
  };

  struct AlignedDeleter {
    std::size_t alloc_size{0};
    std::size_t alignment{0};
    void operator()(std::byte* p) const noexcept;
  };

  std::shared_ptr<std::byte> data_{nullptr};
  std::shared_ptr<VersionState> version_state_{nullptr};
  std::size_t nbytes_{0};
  std::size_t alignment_{64};
  StorageLocation location_{StorageLocation::Host};
};

} // namespace pluss::codem
