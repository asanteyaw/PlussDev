#define TORCH_ASSERT_ONLY_METHOD_OPERATORS
#include <PlussQit/native/ReduceAllOps.h>
#include <PlussQit/native/Resize.h>

#include <PlussQit/core/Series.h>

#ifndef AT_PER_OPERATOR_HEADERS
#include <PlussQit/Functions.h>
#include <PlussQit/NativeFunctions.h>
#else
#include <PlussQit/ops/_aminmax_native.h>
#include <PlussQit/ops/aminmax.h>
#include <PlussQit/ops/empty.h>
#include <PlussQit/ops/max.h>
#include <PlussQit/ops/max_native.h>
#include <PlussQit/ops/min.h>
#include <PlussQit/ops/min_native.h>
#endif

namespace pluss::native {

DEFINE_DISPATCH(min_all_stub);
DEFINE_DISPATCH(max_all_stub);

Series min(const Series &self) {
  TORCH_CHECK(self.numel() > 0,
              "min(): Expected reduction dim to be specified for input.numel() == 0. Specify the reduction dim with the 'dim' argument.");
  Series result = pluss::empty({}, self.options());
  min_all_stub(self.device().type(), result, self.contiguous());
  return result;
}

Series& min_unary_out(const Series &self, Series& out) {
  // First check if the devices match (CPU vs GPU)
  TORCH_CHECK(self.device() == out.device());

  TORCH_CHECK(canCast(
      typeMetaToScalarType(self.dtype()),
      typeMetaToScalarType(out.dtype())));

  pluss::native::resize_output(out, {});

  min_all_stub(self.device().type(), out, self.contiguous());
  return out;
}

Series max(const Series &self) {
  TORCH_CHECK(self.numel() > 0,
              "max(): Expected reduction dim to be specified for input.numel() == 0. Specify the reduction dim with the 'dim' argument.");
  Series result = pluss::empty({}, self.options());
  max_all_stub(self.device().type(), result, self.contiguous());
  return result;
}

Series& max_unary_out(const Series &self, Series& out) {
  // First check if the devices match (CPU vs GPU)
  TORCH_CHECK(self.device() == out.device());

  TORCH_CHECK(canCast(
      typeMetaToScalarType(self.dtype()),
      typeMetaToScalarType(out.dtype())));

  pluss::native::resize_output(out, {});

  max_all_stub(self.device().type(), out, self.contiguous());
  return out;
}

// DEPRECATED: Use pluss::aminmax instead
std::tuple<Series, Series> _aminmax_all(const Series &self) {
  TORCH_WARN_ONCE("_aminmax is deprecated as of PyTorch 1.11 and will be removed in a future release. Use aminmax instead."
                  " This warning will only appear once per process.");
  return pluss::aminmax(self);
}

} // namespace pluss::native
