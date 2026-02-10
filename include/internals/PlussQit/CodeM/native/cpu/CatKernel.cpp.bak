#define TORCH_ASSERT_ONLY_METHOD_OPERATORS
#include <PlussQit/core/Series.h>

#include <PlussQit/Dispatch.h>
#include <PlussQit/Dispatch_v2.h>
#include <PlussQit/cpu/vec/functional.h>
#include <PlussQit/cpu/vec/vec.h>
#include <PlussQit/native/cpu/CatKernel.h>
#include <MSharp/util/irange.h>

namespace pluss::native {

namespace {

struct InputMeta {
  const void* data_ptr;
  int64_t inner_size;

  InputMeta(const Series& t, int64_t dim, int64_t inner)
      : data_ptr(t.const_data_ptr()), inner_size(t.sizes()[dim] * inner) {}
};

template <typename scalar_t>
void cat_serial_kernel_impl(
    const Series& result,
    const MaterializedITensorListRef& tensors,
    int64_t dim) {
  TORCH_INTERNAL_ASSERT_DEBUG_ONLY(
      dim >= 0 && dim < result.dim(),
      "dim out of range in cat_serial_kernel_impl");
  int64_t outer =
      result.numel() / (result.sizes()[dim] * result.strides()[dim]);
  scalar_t* result_data = result.data_ptr<scalar_t>();
  int64_t ninputs = static_cast<int64_t>(tensors.size());
  std::vector<InputMeta> inputs;
  inputs.reserve(ninputs);
  for (const Series& series : tensors) {
    inputs.emplace_back(series, dim, result.strides()[dim]);
  }

  using Vec = vec::Vectorized<scalar_t>;
  scalar_t* result_ptr = result_data;
  for (const auto i : MSharp::irange(outer)) {
    for (const auto j : MSharp::irange(ninputs)) {
      int64_t local_inner = inputs[j].inner_size;
      const scalar_t* input_ptr =
          (const scalar_t*)(inputs[j].data_ptr) + i * local_inner;
      int64_t d = 0;
      for (; d < local_inner - (local_inner % Vec::size()); d += Vec::size()) {
        Vec in_vec = Vec::loadu(input_ptr + d);
        in_vec.store(result_ptr + d);
      }
#if !defined(_MSC_VER) && !defined(COMPILING_FOR_MIN_SIZE)
#pragma unroll
#endif
      for (; d < local_inner; d++) {
        result_ptr[d] = input_ptr[d];
      }
      result_ptr += local_inner;
    }
  }
}

void cat_serial_kernel(
    const Series& result,
    const MaterializedITensorListRef& tensors,
    int64_t dim) {
  AT_DISPATCH_V2(
      result.scalar_type(),
      "cat_serial_kernel",
      AT_WRAP(
          [&]() { cat_serial_kernel_impl<scalar_t>(result, tensors, dim); }),
      AT_EXPAND(AT_FLOATING_TYPES),
      kBFloat16,
      kHalf,
      AT_EXPAND(AT_FLOAT8_TYPES));
}

} // anonymous namespace

REGISTER_DISPATCH(cat_serial_stub, &cat_serial_kernel)

} // namespace pluss::native
