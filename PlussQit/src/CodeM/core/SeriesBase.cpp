#include "CodeM/core/SeriesBase.h"
#include "CodeM/core/Errors.h"
#include "CodeM/core/OpsInternal.h"
#include "CodeM/native/mps/BufferCache.h"

namespace pluss::codem {

ScalarType SeriesBase::scalar_type() const {
  if (!defined()) throw_error("SeriesBase: undefined");
  return impl_->scalar_type();
}
DType SeriesBase::dtype() const {
  if (!defined()) throw_error("SeriesBase: undefined");
  return impl_->dtype();
}
std::span<const std::int64_t> SeriesBase::sizes() const {
  if (!defined()) throw_error("SeriesBase: undefined");
  return impl_->sizes();
}
std::span<const std::int64_t> SeriesBase::strides() const {
  if (!defined()) throw_error("SeriesBase: undefined");
  return impl_->strides();
}
std::int64_t SeriesBase::dim() const { return static_cast<std::int64_t>(sizes().size()); }
std::int64_t SeriesBase::numel() const { return impl_->numel(); }
bool SeriesBase::is_contiguous() const { return impl_->is_contiguous(); }
Series SeriesBase::pos(std::int64_t index) const {
  if (!defined()) throw_error("Series::pos: undefined");
  if (dim() != 1) throw_error("Series::pos: expected 1-D Series for scalar selection");
  const auto size0 = sizes()[0];
  if (index < 0 || index >= size0) throw_error("Series::pos: index out of range");
  const Series self = ops_internal::SeriesFactoryAccess::wrap(impl_);
  return self.select(0, index);
}
std::byte* SeriesBase::raw_data() {
  if (!defined()) throw_error("SeriesBase: undefined");
  if (impl_->platform() == Platform::MPS) {
    native::mps::sync_host_if_needed(*this);
    native::mps::mark_host_data_dirty(*this);
  }
  return impl_->raw_data();
}
const std::byte* SeriesBase::raw_data() const {
  if (!defined()) throw_error("SeriesBase: undefined");
  if (impl_->platform() == Platform::MPS) {
    native::mps::sync_host_if_needed(*this);
  }
  return impl_->raw_data();
}

} // namespace pluss::codem
