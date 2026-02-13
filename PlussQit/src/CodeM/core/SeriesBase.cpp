#include "CodeM/core/SeriesBase.h"
#include "CodeM/core/Errors.h"

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
std::byte* SeriesBase::raw_data() { return impl_->raw_data(); }
const std::byte* SeriesBase::raw_data() const { return impl_->raw_data(); }

} // namespace pluss::codem
