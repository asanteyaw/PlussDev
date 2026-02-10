#pragma once

#include <PlussQit/native/DispatchStub.h>
#include <MSharp/macros/Macros.h>
#include <MSharp/util/BFloat16.h>
#include <MSharp/util/Half.h>

namespace pluss::native {
#if !defined(C10_MOBILE)
using fp16_gemv_fn = void(*)(int, int, float, const Half*, int, const Half*, int, float, Half*, int);
DECLARE_DISPATCH(fp16_gemv_fn, fp16_gemv_trans_stub)

using bf16_gemv_fn = void(*)(int, int, BFloat16, const BFloat16*, int, const BFloat16*, int, BFloat16, BFloat16*, int);
DECLARE_DISPATCH(bf16_gemv_fn, bf16_gemv_trans_stub)

inline namespace CPU_CAPABILITY {
float fp16_dot_with_fp32_arith(const Half* vec1, const Half* vec2, int64_t len);
float bf16_dot_with_fp32_arith(const BFloat16* vec1, const BFloat16* vec2, int64_t len);
} // inline namespace CPU_CAPABILITY
#endif // !defined(C10_MOBILE)
} // namespace pluss::native
