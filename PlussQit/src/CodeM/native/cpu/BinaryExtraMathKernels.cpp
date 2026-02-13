#include "CodeM/native/cpu/BinaryExtraMathKernels.h"

#include "CodeM/core/Errors.h"
#include "CodeM/core/Half.h"
#include "CodeM/core/SeriesIterator.h"

#include <cmath>
#include <cstdint>

namespace pluss::codem::native::cpu {

template <class T, class F>
static void binary_apply(const Series& a, const Series& b, const Series& out_, F fn) {
  auto& out = const_cast<Series&>(out_);
  SeriesIterator it = SeriesIterator::binary(a, b, out);
  const auto* ap = reinterpret_cast<const T*>(a.raw_data());
  const auto* bp = reinterpret_cast<const T*>(b.raw_data());
  auto* op = reinterpret_cast<T*>(out.raw_data());
  it.for_each_index([&](std::int64_t lin, std::int64_t ao, std::int64_t bo, std::int64_t){
    op[lin] = fn(ap[ao], bp[bo]);
  });
}

template <class HalfT, class F>
static void binary_half(const Series& a, const Series& b, const Series& out_, F fn,
                        float(*to_f)(HalfT), HalfT(*from_f)(float)) {
  auto& out = const_cast<Series&>(out_);
  SeriesIterator it = SeriesIterator::binary(a, b, out);
  const auto* ap = reinterpret_cast<const HalfT*>(a.raw_data());
  const auto* bp = reinterpret_cast<const HalfT*>(b.raw_data());
  auto* op = reinterpret_cast<HalfT*>(out.raw_data());
  it.for_each_index([&](std::int64_t lin, std::int64_t ao, std::int64_t bo, std::int64_t){
    const float x = to_f(ap[ao]);
    const float y = to_f(bp[bo]);
    op[lin] = from_f(fn(x,y));
  });
}

void atan2(const Series& a, const Series& b, const Series& out) {
  switch (a.scalar_type()) {
    case ScalarType::Float64: binary_apply<double>(a,b,out,[](double x,double y){ return std::atan2(x,y); }); return;
    case ScalarType::Float32: binary_apply<float>(a,b,out,[](float x,float y){ return std::atan2(x,y); }); return;
    case ScalarType::Float16: binary_half<Float16>(a,b,out,[](float x,float y){ return std::atan2(x,y); }, float16_to_float, float16_from_float); return;
    case ScalarType::BFloat16: binary_half<BFloat16>(a,b,out,[](float x,float y){ return std::atan2(x,y); }, bfloat16_to_float, bfloat16_from_float); return;
    default: throw_error("atan2: dtype not implemented");
  }
}

void fmod(const Series& a, const Series& b, const Series& out) {
  switch (a.scalar_type()) {
    case ScalarType::Float64: binary_apply<double>(a,b,out,[](double x,double y){ return std::fmod(x,y); }); return;
    case ScalarType::Float32: binary_apply<float>(a,b,out,[](float x,float y){ return std::fmod(x,y); }); return;
    case ScalarType::Int64: binary_apply<std::int64_t>(a,b,out,[](std::int64_t x,std::int64_t y){ return x % y; }); return;
    case ScalarType::Int32: binary_apply<std::int32_t>(a,b,out,[](std::int32_t x,std::int32_t y){ return x % y; }); return;
    case ScalarType::Int16: binary_apply<std::int16_t>(a,b,out,[](std::int16_t x,std::int16_t y){ return (std::int16_t)(x % y); }); return;
    case ScalarType::Int8: binary_apply<std::int8_t>(a,b,out,[](std::int8_t x,std::int8_t y){ return (std::int8_t)(x % y); }); return;
    default: throw_error("fmod: dtype not implemented");
  }
}

void remainder(const Series& a, const Series& b, const Series& out) {
  // Torch remainder follows Python's sign rule: result has sign of divisor.
  switch (a.scalar_type()) {
    case ScalarType::Float64: binary_apply<double>(a,b,out,[](double x,double y){
      double r = std::fmod(x,y);
      if ((r != 0) && ((y < 0) != (r < 0))) r += y;
      return r;
    }); return;
    case ScalarType::Float32: binary_apply<float>(a,b,out,[](float x,float y){
      float r = std::fmod(x,y);
      if ((r != 0) && ((y < 0) != (r < 0))) r += y;
      return r;
    }); return;
    case ScalarType::Int64: binary_apply<std::int64_t>(a,b,out,[](std::int64_t x,std::int64_t y){
      auto r = x % y;
      if (r != 0 && ((y < 0) != (r < 0))) r += y;
      return r;
    }); return;
    case ScalarType::Int32: binary_apply<std::int32_t>(a,b,out,[](std::int32_t x,std::int32_t y){
      auto r = x % y;
      if (r != 0 && ((y < 0) != (r < 0))) r += y;
      return r;
    }); return;
    default: throw_error("remainder: dtype not implemented");
  }
}

} // namespace
