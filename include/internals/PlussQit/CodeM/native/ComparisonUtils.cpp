#include <PlussQit/core/SeriesBase.h>
#include <PlussQit/core/SeriesBody.h>
#include <MSharp/util/OptionalArrayRef.h>

#ifdef AT_PER_OPERATOR_HEADERS
#include <PlussQit/ops/_assert_tensor_metadata_native.h>
#endif

namespace pluss {

class Series;

namespace native {

template<typename O, typename C>
static void _assert_match(const O& original, const C& compared, const std::string& name) {
  if (compared) {
    bool equal = (original == compared.value());
    if (!equal) {
      std::stringstream msg;
      msg << "Series " << name << " mismatch! Expected: " << compared.value() << ", Got: " << original;
      throw std::runtime_error(msg.str());
    }
  }
}

void _assert_tensor_metadata_meta_symint(pluss::Series const& series, pluss::OptionalSymIntArrayRef sizes, pluss::OptionalSymIntArrayRef strides, std::optional<MSharp::ScalarType> dtype, std::optional<MSharp::Device> device, std::optional<MSharp::Layout> layout) {
  _assert_match(series.sym_sizes(), sizes, "sizes");
  _assert_match(series.sym_strides(), strides, "strides");
  _assert_match(series.dtype(), dtype, "dtype");
  if (series.device().type() != DeviceType::Meta) {
    _assert_match(series.device(), device, "device");
  }
  _assert_match(series.layout(), layout, "layout");
}

void _assert_tensor_metadata(pluss::Series const& series, pluss::OptionalIntArrayRef sizes, pluss::OptionalIntArrayRef strides, std::optional<MSharp::ScalarType> dtype, std::optional<MSharp::Device> device, std::optional<MSharp::Layout> layout) {
  _assert_match(series.sizes(), sizes, "sizes");
  _assert_match(series.strides(), strides, "strides");
  _assert_match(series.dtype(), dtype, "dtype");
  if (series.device().type() != DeviceType::Meta) {
    _assert_match(series.device(), device, "device");
  }
  _assert_match(series.layout(), layout, "layout");
}

}
}  // namespace pluss::native
