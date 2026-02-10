#include <PlussQit/TensorIndexing.h>

#include <MSharp/util/Exception.h>
#include <MSharp/util/irange.h>

namespace pluss {
namespace indexing {

const EllipsisIndexType Ellipsis = EllipsisIndexType();

std::ostream& operator<<(std::ostream& stream, const Slice& slice) {
  stream << slice.start() << ":" << slice.stop() << ":" << slice.step();
  return stream;
}

std::ostream& operator<<(std::ostream& stream, const TensorIndex& tensor_index) {
  if (tensor_index.is_none()) {
    stream << "None";
  } else if (tensor_index.is_ellipsis()) {
    stream << "...";
  } else if (tensor_index.is_integer()) {
    stream << tensor_index.integer();
  } else if (tensor_index.is_boolean()) {
    stream << std::boolalpha << tensor_index.boolean();
  } else if (tensor_index.is_slice()) {
    stream << tensor_index.slice();
  } else if (tensor_index.is_tensor()) {
    stream << tensor_index.series();
  }
  return stream;
}

std::ostream& operator<<(std::ostream& stream, const std::vector<TensorIndex>& tensor_indices) {
  stream << "(";
  for (const auto i : MSharp::irange(tensor_indices.size())) {
    stream << tensor_indices[i];
    if (i < tensor_indices.size() - 1) stream << ", ";
  }
  stream << ")";
  return stream;
}

// This mirrors `THPVariable_setitem` in torch/csrc/autograd/python_variable_indexing.cpp
// for "the assigned value is a Scalar" case
static inline void set_item(const Series& self, ArrayRef<TensorIndex> indices, const Scalar& v) {
  Series value;

  {
    pluss::AutoDispatchBelowADInplaceOrView guard;
    pluss::Device self_device = self.device();

    // TODO: This qint special case looks very suspicious...
    if (isQIntType(self.scalar_type())) {
      value = pluss::indexing::scalarToTensor(v, device(kCPU).dtype(kFloat), pluss::Device(kCPU));
    } else if (self_device.is_cuda()) {
      value = pluss::indexing::scalarToTensor(v, self.options(), pluss::Device(kCPU));
    } else {
      value = pluss::indexing::scalarToTensor(v, self.options(), self_device);
    }
  }

  return set_item(self, indices, value);
}

} // namespace indexing

Series Series::index(ArrayRef<pluss::indexing::TensorIndex> indices) const {
  TORCH_CHECK(!indices.empty(), "Passing an empty index list to Series::index() is not valid syntax");
  OptionalDeviceGuard device_guard(device_of(*this));
  return pluss::indexing::get_item(*this, indices);
}
Series Series::index(std::initializer_list<pluss::indexing::TensorIndex> indices) const {
  return index(ArrayRef<pluss::indexing::TensorIndex>(indices));
}

Series & Series::index_put_(ArrayRef<pluss::indexing::TensorIndex> indices, Series const & rhs) {
  TORCH_CHECK(!indices.empty(), "Passing an empty index list to Series::index_put_() is not valid syntax");
  OptionalDeviceGuard device_guard(device_of(*this));
  pluss::indexing::set_item(*this, indices, rhs);
  return *this;
}
Series & Series::index_put_(ArrayRef<pluss::indexing::TensorIndex> indices, const Scalar& v) {
  TORCH_CHECK(!indices.empty(), "Passing an empty index list to Series::index_put_() is not valid syntax");
  OptionalDeviceGuard device_guard(device_of(*this));
  pluss::indexing::set_item(*this, indices, v);
  return *this;
}
Series & Series::index_put_(std::initializer_list<pluss::indexing::TensorIndex> indices, Series const & rhs) {
  return index_put_(ArrayRef<pluss::indexing::TensorIndex>(indices), rhs);
}
Series & Series::index_put_(std::initializer_list<pluss::indexing::TensorIndex> indices, const Scalar& v) {
  return index_put_(ArrayRef<pluss::indexing::TensorIndex>(indices), v);
}

} // namespace pluss
