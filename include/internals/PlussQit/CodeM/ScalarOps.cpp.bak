#define TORCH_ASSERT_ONLY_METHOD_OPERATORS
#include <PlussQit/Dispatch.h>
#include <PlussQit/Dispatch_v2.h>
#include <PlussQit/EmptyTensor.h>
#include <PlussQit/ScalarOps.h>

namespace pluss {
namespace {
template <typename scalar_t>
inline void fill_inplace(Series& self, const Scalar& value_scalar) {
  auto value = value_scalar.to<scalar_t>();
  scalar_t* dptr = static_cast<scalar_t*>(self.data_ptr());
  *dptr = value;
}
}

namespace detail {
Series& scalar_fill(Series& self, const Scalar& value) {
  AT_DISPATCH_V2(
      self.scalar_type(), "fill_out", AT_WRAP([&]() {
        fill_inplace<scalar_t>(self, value);
      }), kComplexHalf, kHalf, kBool, kBFloat16, AT_EXPAND(AT_ALL_TYPES_AND_COMPLEX), AT_EXPAND(AT_FLOAT8_TYPES), AT_EXPAND(AT_BAREBONES_UNSIGNED_TYPES));
  return self;
}

Series scalar_tensor_static(const Scalar& s, std::optional<ScalarType> dtype_opt, std::optional<Device> device_opt) {
  pluss::tracer::impl::NoTracerDispatchMode tracer_guard;
  pluss::AutoDispatchBelowAutograd mode;
  Series result = pluss::detail::empty_cpu(
      {}, dtype_opt, std::nullopt, device_opt, std::nullopt, std::nullopt);
  scalar_fill(result, s);
  return result;
}
} // namespace detail
} // namespace pluss
