#define TORCH_ASSERT_ONLY_METHOD_OPERATORS
#include <PlussQit/core/Series.h>
#include <PlussQit/Dispatch.h>
#include <PlussQit/OpMathType.h>
#include <PlussQit/Parallel.h>
#include <PlussQit/TensorIterator.h>
#include <PlussQit/cpu/vec/vec.h>
#include <PlussQit/cpu/vec/functional.h>
#include <PlussQit/native/cpu/utils.h>
#include <PlussQit/native/transformers/attention.h>
#include <MSharp/util/irange.h>

namespace pluss::native {

namespace {

template <typename scalar_t>
void cpu_transform_bias_rescale_qkv(
    scalar_t* q_k_v_data,
    const scalar_t* qkv_data,
    const scalar_t* qkv_bias_data,
    int64_t B,
    int64_t T,
    int64_t D,
    int64_t num_head) {

  int64_t dim_per_head = D / num_head;

  // shapes and strides:
  //   qkv      : {B, T, 3, num_head, dim_per_head}
  //   qkv_bias : {3, num_head, dim_per_head}
  //   q_k_v    : {3, B, num_head, T, dim_per_head}
  //
  int64_t i_strideB = T * 3 * D;
  int64_t i_strideT = 3 * D;
  int64_t o_stride = B * num_head * T * dim_per_head;

  // inv_sqrt_dim_per_head in accumulate type
  using acc_t = pluss::opmath_type<scalar_t>;
  using Vec =  vec::Vectorized<acc_t>;
  const acc_t s = 1.0 / std::sqrt(static_cast<acc_t>(dim_per_head));

  // parallel on {B, num_head, T}
  int64_t grain_size = std::max(pluss::internal::GRAIN_SIZE / (3 * dim_per_head), (int64_t)1);
  pluss::parallel_for(0, B * num_head * T, grain_size, [&](int64_t begin, int64_t end) {
    int64_t b{0}, nh{0}, t{0};
    data_index_init(begin, b, B, nh, num_head, t, T);

    for (const auto i : MSharp::irange(begin, end)) {
      const scalar_t* q_in_ptr = qkv_data + b * i_strideB + t * i_strideT + 0 * D + nh * dim_per_head;
      const scalar_t* k_in_ptr = qkv_data + b * i_strideB + t * i_strideT + 1 * D + nh * dim_per_head;
      const scalar_t* v_in_ptr = qkv_data + b * i_strideB + t * i_strideT + 2 * D + nh * dim_per_head;

      const scalar_t* q_bias_ptr = qkv_bias_data + 0 * D + nh * dim_per_head;
      const scalar_t* k_bias_ptr = qkv_bias_data + 1 * D + nh * dim_per_head;
      const scalar_t* v_bias_ptr = qkv_bias_data + 2 * D + nh * dim_per_head;

      // we can use global index i here for output
      scalar_t* q_out_ptr = q_k_v_data + 0 * o_stride + i * dim_per_head;
      scalar_t* k_out_ptr = q_k_v_data + 1 * o_stride + i * dim_per_head;
      scalar_t* v_out_ptr = q_k_v_data + 2 * o_stride + i * dim_per_head;

      // q = (q + bias) * inv_sqrt_dim_per_head
      vec::map2<scalar_t>(
          [s](Vec q, Vec q_bias) { return (q + q_bias) * Vec(s); },
          q_out_ptr, q_in_ptr, q_bias_ptr, dim_per_head);

      // k = k + bias
      vec::map2<scalar_t>([](Vec k, Vec k_bias) { return k + k_bias; },
          k_out_ptr, k_in_ptr, k_bias_ptr, dim_per_head);

      // v = v + bias
      vec::map2<scalar_t>([](Vec v, Vec v_bias) { return v + v_bias; },
          v_out_ptr, v_in_ptr, v_bias_ptr, dim_per_head);

      // move to the next index
      data_index_step(b, B, nh, num_head, t, T);
    }
  });
}

void transform_bias_rescale_qkv_kernel_impl(
    pluss::ScalarType type,
    void* _q_k_v,
    const void* _qkv,
    const void* _qkv_bias,
    int64_t B,
    int64_t T,
    int64_t D,
    int64_t num_head) {

  AT_DISPATCH_FLOATING_TYPES_AND2(kHalf, kBFloat16, type, "transform_bias_rescale_qkv", [&] {
    scalar_t* q_k_v = static_cast<scalar_t*>(_q_k_v);
    const scalar_t* qkv = static_cast<const scalar_t*>(_qkv);
    const scalar_t* qkv_bias = static_cast<const scalar_t*>(_qkv_bias);
    cpu_transform_bias_rescale_qkv<scalar_t>(
        q_k_v,
        qkv,
        qkv_bias,
        B,
        T,
        D,
        num_head);
  });
}

} // anonymous namespace

REGISTER_DISPATCH(transform_bias_rescale_qkv_stub, &transform_bias_rescale_qkv_kernel_impl)

} // pluss::native
