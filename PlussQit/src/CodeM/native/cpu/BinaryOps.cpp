#include <cstdint>

#include "CodeM/core/Errors.h"
#include "CodeM/core/SeriesIterator.h"
#include "CodeM/core/ScalarType.h"
#include "CodeM/core/Half.h"

namespace pluss::codem::native::cpu {

template <class T, class Fn>
static void binary_loop(SeriesIterator& it, Fn&& fn) {
  auto* outp = reinterpret_cast<T*>(const_cast<std::byte*>(it.out().raw_data()));
  const auto* ap = reinterpret_cast<const T*>(it.a().raw_data());
  const auto* bp = reinterpret_cast<const T*>(it.b().raw_data());
  it.for_each_index([&](std::int64_t lin, std::int64_t ao, std::int64_t bo, std::int64_t){
    outp[lin] = fn(ap[ao], bp[bo]);
  });
}

template <class HalfT, class ToF32, class FromF32, class Fn>
static void binary_half_loop(SeriesIterator& it, Fn&& fn, ToF32&& to, FromF32&& from) {
  auto* outp = reinterpret_cast<HalfT*>(const_cast<std::byte*>(it.out().raw_data()));
  const auto* ap = reinterpret_cast<const HalfT*>(it.a().raw_data());
  const auto* bp = reinterpret_cast<const HalfT*>(it.b().raw_data());
  it.for_each_index([&](std::int64_t lin, std::int64_t ao, std::int64_t bo, std::int64_t){
    outp[lin] = from(fn(to(ap[ao]), to(bp[bo])));
  });
}

// Legacy "binary ops" used by generated wrappers for add/sub/mul/div etc are now in Arithmetic.cpp.
// This file remains for older ops; we keep it small and Torch-independent.

void add(const Series& a, const Series& b, const Series& out_) {
  auto& out = const_cast<Series&>(out_);
  SeriesIterator it = SeriesIterator::binary(a,b,out);
  switch (a.scalar_type()) {
    case ScalarType::Float64: binary_loop<double>(it, [](double x,double y){ return x+y; }); return;
    case ScalarType::Float32: binary_loop<float>(it, [](float x,float y){ return x+y; }); return;
    case ScalarType::Float16: binary_half_loop<Float16>(it, [](float x,float y){ return x+y; }, float16_to_float, float16_from_float); return;
    case ScalarType::BFloat16: binary_half_loop<BFloat16>(it, [](float x,float y){ return x+y; }, bfloat16_to_float, bfloat16_from_float); return;
    default: throw_error("BinaryOps::add: dtype not implemented");
  }
}

} // namespace pluss::codem::native::cpu
