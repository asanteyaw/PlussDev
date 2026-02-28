#pragma once
#include "pluss/export.h"
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>

namespace pluss::codem {

// CPU-only storage (shareable). Later: allocator interface, external memory, mmap, etc.
class CODEM_API Storage final {
public:
  Storage() = default;
  explicit Storage(std::size_t nbytes, std::size_t alignment = 64);

  // Adopt externally-managed memory (kept alive via shared_ptr).
  Storage(std::shared_ptr<std::byte> external, std::size_t nbytes, std::size_t alignment = 1);

  std::byte* data() noexcept { return data_.get(); }
  const std::byte* data() const noexcept { return data_.get(); }
  std::size_t nbytes() const noexcept { return nbytes_; }
  std::size_t alignment() const noexcept { return alignment_; }
  bool defined() const noexcept { return static_cast<bool>(data_); }
  std::shared_ptr<std::byte> handle() const noexcept { return data_; }
  std::int64_t version() const noexcept;
  void bump_version() const noexcept;

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
};

} // namespace pluss::codem
