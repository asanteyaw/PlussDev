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
pluss::msharp::RangeView::Int SeriesBase::sizes() const {
  if (!defined()) throw_error("SeriesBase: undefined");
  return pluss::msharp::RangeView::Int{impl_->sizes()};
}
std::int64_t SeriesBase::size(std::int64_t dim) const {
  if (!defined()) throw_error("SeriesBase: undefined");
  auto nd = this->dim();
  if (dim < 0) dim += nd;
  if (dim < 0 || dim >= nd) throw_error("SeriesBase::size: dim out of range");
  return sizes()[static_cast<std::size_t>(dim)];
}
pluss::msharp::RangeView::Int SeriesBase::strides() const {
  if (!defined()) throw_error("SeriesBase: undefined");
  return pluss::msharp::RangeView::Int{impl_->strides()};
}
std::int64_t SeriesBase::stride(std::int64_t dim) const {
  if (!defined()) throw_error("SeriesBase: undefined");
  const auto nd = this->dim();
  if (dim < 0) dim += nd;
  if (dim < 0 || dim >= nd) throw_error("Series::stride: dim out of range");
  return strides()[static_cast<std::size_t>(dim)];
}
std::int64_t SeriesBase::dim() const { return static_cast<std::int64_t>(sizes().size()); }
std::int64_t SeriesBase::numel() const {
  if (!defined()) throw_error("SeriesBase: undefined");
  return impl_->numel();
}
bool SeriesBase::is_contiguous() const {
  if (!defined()) throw_error("SeriesBase: undefined");
  return impl_->is_contiguous();
}
bool SeriesBase::is_signed() const {
  const auto st = scalar_type();
  switch (st) {
    case ScalarType::Int8:
    case ScalarType::Int16:
    case ScalarType::Int32:
    case ScalarType::Int64:
    case ScalarType::Float16:
    case ScalarType::BFloat16:
    case ScalarType::Float32:
    case ScalarType::Float64:
    case ScalarType::Float8E4M3FN:
    case ScalarType::Float8E5M2:
    case ScalarType::Complex64:
    case ScalarType::Complex128:
      return true;
    default:
      return false;
  }
}
Series SeriesBase::pos(std::int64_t index) const {
  if (!defined()) throw_error("Series::pos: undefined");
  if (dim() != 1) throw_error("Series::pos: expected 1-D Series for scalar selection");
  const auto size0 = sizes()[0];
  if (index < 0) index += size0;
  if (index < 0 || index >= size0) throw_error("Series::pos: index out of range");
  const Series self = ops_internal::SeriesFactoryAccess::wrap(impl_);
  return self.select(0, index);
}
Series SeriesBase::pos(std::int64_t row, std::int64_t col) const {
  if (!defined()) throw_error("Series::pos: undefined");
  if (dim() != 2) throw_error("Series::pos: expected 2-D Series for scalar selection");
  const auto nrows = sizes()[0];
  const auto ncols = sizes()[1];
  if (row < 0) row += nrows;
  if (col < 0) col += ncols;
  if (row < 0 || row >= nrows || col < 0 || col >= ncols) {
    throw_error("Series::pos: index out of range");
  }
  const Series self = ops_internal::SeriesFactoryAccess::wrap(impl_);
  return self.select(0, row).select(0, col);
}
Series SeriesBase::pos(std::int64_t i, std::int64_t j, std::int64_t k) const {
  if (!defined()) throw_error("Series::pos: undefined");
  if (dim() != 3) throw_error("Series::pos: expected 3-D Series for scalar selection");
  const auto n0 = sizes()[0];
  const auto n1 = sizes()[1];
  const auto n2 = sizes()[2];
  if (i < 0) i += n0;
  if (j < 0) j += n1;
  if (k < 0) k += n2;
  if (i < 0 || i >= n0 || j < 0 || j >= n1 || k < 0 || k >= n2) {
    throw_error("Series::pos: index out of range");
  }
  const Series self = ops_internal::SeriesFactoryAccess::wrap(impl_);
  return self.select(0, i).select(0, j).select(0, k);
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
