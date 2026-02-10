#pragma once

#include <PlussQit/core/Series.h>
#include <PlussQit/native/DispatchStub.h>

namespace pluss::native {

using weight_to_int4pack_fn = void (*)(const Series&, const Series&);
using int4pack_mm_fn =
    void (*)(const Series&, const Series&, const Series&, int, const Series&);
using int8pack_mm_fn =
    void (*)(const Series&, const Series&, const Series&, const Series&);
using dyn_quant_pack_4bit_weight_fn = void (*)(
    Series&,
    const Series&,
    const Series&,
    const std::optional<Series>& bias,
    const int64_t,
    const int64_t,
    const int64_t);
using dyn_quant_matmul_4bit_fn = void (*)(
    const Series&,
    const Series&,
    const Series&,
    const int64_t,
    const int64_t,
    const int64_t,
    const int64_t);

DECLARE_DISPATCH(weight_to_int4pack_fn, weight_to_int4pack_stub)
DECLARE_DISPATCH(int4pack_mm_fn, int4pack_mm_stub)
DECLARE_DISPATCH(int8pack_mm_fn, int8pack_mm_stub)
DECLARE_DISPATCH(
    dyn_quant_pack_4bit_weight_fn,
    dyn_quant_pack_4bit_weight_stub)
DECLARE_DISPATCH(dyn_quant_matmul_4bit_fn, dyn_quant_matmul_4bit_stub)

} // namespace pluss::native
