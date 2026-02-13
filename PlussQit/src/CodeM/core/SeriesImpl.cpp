#include "CodeM/core/SeriesImpl.h"
#include "CodeM/core/ViewInvariants.h"
#include "CodeM/core/Errors.h"

#include <cstddef>

namespace pluss::codem {

std::int64_t compute_numel(std::span<const std::int64_t> sizes) {
  if (sizes.empty()) return 1;
  std::int64_t n = 1;
  for (auto s : sizes) {
    if (s < 0) throw_error("compute_numel: negative size");
    n *= s;
  }
  return n;
}

std::vector<std::int64_t> contiguous_strides(std::span<const std::int64_t> sizes) {
  std::vector<std::int64_t> st(sizes.size(), 1);
  std::int64_t running = 1;
  for (std::int64_t i = static_cast<std::int64_t>(sizes.size()) - 1; i >= 0; --i) {
    st[static_cast<std::size_t>(i)] = running;
    running *= sizes[static_cast<std::size_t>(i)];
  }
  return st;
}

SeriesImpl::SeriesImpl(Storage storage,
                       std::int64_t storage_offset_elems,
                       std::vector<std::int64_t> sizes,
                       std::vector<std::int64_t> strides,
                       ScalarType st) : storage_(std::move(storage)),
    storage_offset_elems_(storage_offset_elems),
    sizes_(std::move(sizes)),
    strides_(std::move(strides)),
    st_(st) {
  if (sizes_.size() != strides_.size()) throw_error("SeriesImpl: sizes/strides rank mismatch");
  for (auto s: sizes_) if (s < 0) throw_error("SeriesImpl: negative size");

  if (!storage_.defined()) throw_error("SeriesImpl: undefined storage");
  if (storage_offset_elems_ < 0) throw_error("SeriesImpl: negative storage offset");
  if (sizes_.size() != strides_.size()) throw_error("SeriesImpl: rank mismatch");
  if (sizes_.size() > 7) throw_error("SeriesImpl: max 7 dimensions supported");
}

std::int64_t SeriesImpl::numel() const noexcept { return compute_numel(sizes_); }

bool SeriesImpl::is_contiguous() const noexcept {
  const auto cont = contiguous_strides(sizes_);
  if (cont.size() != strides_.size()) return false;
  for (std::size_t i = 0; i < cont.size(); ++i) if (cont[i] != strides_[i]) return false;
  return true;
}

std::byte* SeriesImpl::raw_data() noexcept {
  const auto dt = dtype_of(st_);
  return storage_.data() + static_cast<std::ptrdiff_t>(storage_offset_elems_) * static_cast<std::ptrdiff_t>(dt.itemsize);
}

const std::byte* SeriesImpl::raw_data() const noexcept {
  const auto dt = dtype_of(st_);
  return storage_.data() + static_cast<std::ptrdiff_t>(storage_offset_elems_) * static_cast<std::ptrdiff_t>(dt.itemsize);
}

} // namespace pluss::codem
