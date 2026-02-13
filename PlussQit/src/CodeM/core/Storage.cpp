#include "CodeM/core/Storage.h"
#include "CodeM/core/Errors.h"

#include <cstdlib>

namespace pluss::codem {

void Storage::AlignedDeleter::operator()(std::byte* p) const noexcept { std::free(p); }

static std::size_t round_up(std::size_t x, std::size_t a) { return (x + (a - 1)) / a * a; }

Storage::Storage(std::size_t nbytes, std::size_t alignment)
  : nbytes_(nbytes), alignment_(alignment) {
  if (nbytes == 0) return;
  if (alignment == 0) alignment = 64;
  const std::size_t alloc_size = round_up(nbytes, alignment);

  void* p = std::aligned_alloc(alignment, alloc_size);
  if (!p) throw_error("Storage: aligned_alloc failed");
  data_ = std::shared_ptr<std::byte>(reinterpret_cast<std::byte*>(p), AlignedDeleter{});
}

Storage::Storage(std::shared_ptr<std::byte> external, std::size_t nbytes, std::size_t alignment)
  : data_(std::move(external)), nbytes_(nbytes), alignment_(alignment) {}

} // namespace pluss::codem
