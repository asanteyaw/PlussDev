#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>

namespace pluss::codem {

// CPU-only storage (shareable). Later: allocator interface, external memory, mmap, etc.
class Storage final {
public:
  Storage() = default;
  explicit Storage(std::size_t nbytes, std::size_t alignment = 64);

  // Adopt externally-managed memory (kept alive via shared_ptr).
  Storage(std::shared_ptr<std::byte> external, std::size_t nbytes, std::size_t alignment = 1);

  std::byte* data() noexcept { return data_.get(); }
  const std::byte* data() const noexcept { return data_.get(); }
  std::size_t nbytes() const noexcept { return nbytes_; }
  bool defined() const noexcept { return static_cast<bool>(data_); }
  std::shared_ptr<std::byte> handle() const noexcept { return data_; }

private:
  struct AlignedDeleter { void operator()(std::byte* p) const noexcept; };

  std::shared_ptr<std::byte> data_{nullptr};
  std::size_t nbytes_{0};
  std::size_t alignment_{64};
};

} // namespace pluss::codem
