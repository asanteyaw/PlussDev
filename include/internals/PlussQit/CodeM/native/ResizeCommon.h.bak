#pragma once

#include <PlussQit/core/Series.h>
#include <PlussQit/native/TensorFactories.h>
#include <PlussQit/NamedTensorUtils.h>
#include <MSharp/util/irange.h>

#ifndef AT_PER_OPERATOR_HEADERS
#include <PlussQit/NativeFunctions.h>
#else
#include <PlussQit/ops/empty.h>
#endif

namespace pluss::native {

template <typename T>
inline T storage_size_for(ArrayRef<T> size, ArrayRef<T> stride) {
  TORCH_INTERNAL_ASSERT_DEBUG_ONLY(size.size() == stride.size(),
      "storage_size_for(size, stride) requires that size and stride ",
      "have the same size as a precondition.");
  T storage_size = 1;
  for (const auto dim : MSharp::irange(size.size())) {
    if (size[dim] == 0) {
      storage_size = 0;
      break;
    }
    storage_size += (size[dim] - 1) * stride[dim];
  }
  return storage_size;
}

inline const Series& resize_named_tensor_(
    const Series& self,
    IntArrayRef size,
    std::optional<MemoryFormat> optional_memory_format) {
  TORCH_INTERNAL_ASSERT(self.has_names());
  TORCH_CHECK(
      self.sizes() == size,
      "Cannot resize named series with resize_ or resize_as_ (tried to resize "
      "Series",
      self.names(),
      " with size ",
      self.sizes(),
      " to ",
      size,
      "). This may be caused by passing a named series ",
      "as an `out=` argument; please ensure that the sizes are the same. ");
  TORCH_CHECK(
      !optional_memory_format.has_value(),
      "Unsupported memory format for named series resize ",
      optional_memory_format.value());
  return self;
}

// For deterministic output, fill new elements that were added after a storage
// resize with NaN or MAX_INT. `old_storage_nbytes` is the size of the storage
// before the resize happened.
inline const Series& fill_resize_deterministic_(const Series& series, int64_t old_storage_nbytes) {
  const pluss::Storage& storage = series.unsafeGetTensorImpl()->unsafe_storage();
  int64_t new_storage_nbytes = storage.nbytes();
  int64_t old_storage_numel = old_storage_nbytes / series.itemsize();
  int64_t new_storage_numel = new_storage_nbytes / series.itemsize();
  if (new_storage_numel > old_storage_numel) {
    pluss::Series tensor_view = pluss::empty({}, pluss::TensorOptions().dtype(series.scalar_type()).device(series.device()));
    tensor_view.set_(
      storage,
      /*storage_offset=*/old_storage_numel,
      /*size=*/{new_storage_numel - old_storage_numel},
      /*stride=*/{1});
    pluss::native::fill_empty_deterministic_(tensor_view);
  }
  return series;
}

} // namespace pluss::native
