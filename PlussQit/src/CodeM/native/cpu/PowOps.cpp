#include "CodeM/native/cpu/PowOps.h"
#include "CodeM/core/Errors.h"
#include "CodeM/core/SeriesIterator.h"
#include "CodeM/core/Half.h"
#include <cmath>
#include <cstdint>

namespace pluss::codem::native::cpu {

template <class T>
static void pow_loop(SeriesIterator& it) {
  auto* outp = reinterpret_cast<T*>(const_cast<std::byte*>(it.out().raw_data()));
  const auto* ap = reinterpret_cast<const T*>(it.a().raw_data());
  const auto* bp = reinterpret_cast<const T*>(it.b().raw_data());
  it.for_each_index([&](std::int64_t lin, std::int64_t ao, std::int64_t bo, std::int64_t){
    outp[lin] = (T)std::pow((double)ap[ao], (double)bp[bo]);
  });
}

template <class HalfT, class ToF32, class FromF32>
static void pow_half_loop(SeriesIterator& it, ToF32&& to, FromF32&& from) {
  auto* outp = reinterpret_cast<HalfT*>(const_cast<std::byte*>(it.out().raw_data()));
  const auto* ap = reinterpret_cast<const HalfT*>(it.a().raw_data());
  const auto* bp = reinterpret_cast<const HalfT*>(it.b().raw_data());
  it.for_each_index([&](std::int64_t lin, std::int64_t ao, std::int64_t bo, std::int64_t){
    outp[lin] = from(std::pow(to(ap[ao]), to(bp[bo])));
  });
}

void pow(const Series& a, const Series& b, const Series& out) {
  SeriesIterator it = SeriesIterator::binary(a,b,out);
  switch (a.scalar_type()) {
    case ScalarType::Float64: pow_loop<double>(it); return;
    case ScalarType::Float32: pow_loop<float>(it); return;
    case ScalarType::Float16: pow_half_loop<Float16>(it, float16_to_float, float16_from_float); return;
    case ScalarType::BFloat16: pow_half_loop<BFloat16>(it, bfloat16_to_float, bfloat16_from_float); return;
    case ScalarType::Int64: pow_loop<std::int64_t>(it); return;
    case ScalarType::Int32: pow_loop<std::int32_t>(it); return;
    default: break;
  }
  throw_error("pow: dtype not implemented");
}

} // namespace
