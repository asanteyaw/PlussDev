#include "CodeM/core/Errors.h"
#include "CodeM/core/Half.h"
#include "CodeM/core/SeriesSlim.h"
#include "CodeM/core/ScalarType.h"
#include "CodeM/core/SeriesIterator.h"

#include <algorithm>
#include <cstdint>
#include <cmath>

namespace pluss::codem::native::cpu {

template <class T>
static void binary_apply(const Series& a, const Series& b, Series& out, T(*fn)(T,T)) {
  SeriesIterator it = SeriesIterator::binary(a, b, out);
  const auto* ap = reinterpret_cast<const T*>(a.raw_data());
  const auto* bp = reinterpret_cast<const T*>(b.raw_data());
  auto* op = reinterpret_cast<T*>(out.raw_data());
  it.for_each_index([&](std::int64_t lin, std::int64_t ao, std::int64_t bo, std::int64_t){
    op[lin] = fn(ap[ao], bp[bo]);
  });
}

template <class T>
static void clamp_apply(const Series& a, Series& out, T lo, T hi) {
  SeriesIterator it = SeriesIterator::unary(a, out);
  const auto* ap = reinterpret_cast<const T*>(a.raw_data());
  auto* op = reinterpret_cast<T*>(out.raw_data());
  it.for_each_index([&](std::int64_t lin, std::int64_t ao, std::int64_t, std::int64_t){
    const T v = ap[ao];
    op[lin] = std::isnan(v) ? v : std::min(hi, std::max(lo, v));
  });
}

template <class HalfT, class ToF32, class FromF32>
static void minmax_half(const Series& a, const Series& b, Series& out, bool is_min, ToF32&& to, FromF32&& from) {
  SeriesIterator it = SeriesIterator::binary(a, b, out);
  const auto* ap = reinterpret_cast<const HalfT*>(a.raw_data());
  const auto* bp = reinterpret_cast<const HalfT*>(b.raw_data());
  auto* op = reinterpret_cast<HalfT*>(out.raw_data());
  it.for_each_index([&](std::int64_t lin, std::int64_t ao, std::int64_t bo, std::int64_t){
    const float x = to(ap[ao]), y = to(bp[bo]);
    const float r = std::isnan(x) ? x
                                  : (std::isnan(y) ? y
                                                   : (is_min ? std::min(x,y) : std::max(x,y)));
    op[lin] = from(r);
  });
}

template <class HalfT, class ToF32, class FromF32>
static void clamp_half(const Series& a, Series& out, double lo, double hi, ToF32&& to, FromF32&& from) {
  SeriesIterator it = SeriesIterator::unary(a, out);
  const auto* ap = reinterpret_cast<const HalfT*>(a.raw_data());
  auto* op = reinterpret_cast<HalfT*>(out.raw_data());
  it.for_each_index([&](std::int64_t lin, std::int64_t ao, std::int64_t, std::int64_t){
    const float x = to(ap[ao]);
    const float r = std::isnan(x) ? x : std::min((float)hi, std::max((float)lo, x));
    op[lin] = from(r);
  });
}

static float fminf_(float x,float y){return std::isnan(x) ? x : (std::isnan(y) ? y : std::min(x,y));}
static float fmaxf_(float x,float y){return std::isnan(x) ? x : (std::isnan(y) ? y : std::max(x,y));}
static double fmin_(double x,double y){return std::isnan(x) ? x : (std::isnan(y) ? y : std::min(x,y));}
static double fmax_(double x,double y){return std::isnan(x) ? x : (std::isnan(y) ? y : std::max(x,y));}
template <class T> static T imin_(T x,T y){return std::min(x,y);}
template <class T> static T imax_(T x,T y){return std::max(x,y);}

void minimum(const Series& a, const Series& b, const Series& out_) {
  auto& out = const_cast<Series&>(out_);
  switch (a.scalar_type()) {
    case ScalarType::Float64: binary_apply<double>(a,b,out,fmin_); return;
    case ScalarType::Float32: binary_apply<float>(a,b,out,fminf_); return;
    case ScalarType::Float16: minmax_half<Float16>(a,b,out,true,float16_to_float,float16_from_float); return;
    case ScalarType::BFloat16: minmax_half<BFloat16>(a,b,out,true,bfloat16_to_float,bfloat16_from_float); return;
    case ScalarType::Int8: binary_apply<std::int8_t>(a,b,out,imin_<std::int8_t>); return;
    case ScalarType::UInt8: binary_apply<std::uint8_t>(a,b,out,imin_<std::uint8_t>); return;
    case ScalarType::Int16: binary_apply<std::int16_t>(a,b,out,imin_<std::int16_t>); return;
    case ScalarType::UInt16: binary_apply<std::uint16_t>(a,b,out,imin_<std::uint16_t>); return;
    case ScalarType::Int32: binary_apply<std::int32_t>(a,b,out,imin_<std::int32_t>); return;
    case ScalarType::UInt32: binary_apply<std::uint32_t>(a,b,out,imin_<std::uint32_t>); return;
    case ScalarType::Int64: binary_apply<std::int64_t>(a,b,out,imin_<std::int64_t>); return;
    case ScalarType::UInt64: binary_apply<std::uint64_t>(a,b,out,imin_<std::uint64_t>); return;
    default: throw_error("minimum: dtype not implemented");
  }
}

void maximum(const Series& a, const Series& b, const Series& out_) {
  auto& out = const_cast<Series&>(out_);
  switch (a.scalar_type()) {
    case ScalarType::Float64: binary_apply<double>(a,b,out,fmax_); return;
    case ScalarType::Float32: binary_apply<float>(a,b,out,fmaxf_); return;
    case ScalarType::Float16: minmax_half<Float16>(a,b,out,false,float16_to_float,float16_from_float); return;
    case ScalarType::BFloat16: minmax_half<BFloat16>(a,b,out,false,bfloat16_to_float,bfloat16_from_float); return;
    case ScalarType::Int8: binary_apply<std::int8_t>(a,b,out,imax_<std::int8_t>); return;
    case ScalarType::UInt8: binary_apply<std::uint8_t>(a,b,out,imax_<std::uint8_t>); return;
    case ScalarType::Int16: binary_apply<std::int16_t>(a,b,out,imax_<std::int16_t>); return;
    case ScalarType::UInt16: binary_apply<std::uint16_t>(a,b,out,imax_<std::uint16_t>); return;
    case ScalarType::Int32: binary_apply<std::int32_t>(a,b,out,imax_<std::int32_t>); return;
    case ScalarType::UInt32: binary_apply<std::uint32_t>(a,b,out,imax_<std::uint32_t>); return;
    case ScalarType::Int64: binary_apply<std::int64_t>(a,b,out,imax_<std::int64_t>); return;
    case ScalarType::UInt64: binary_apply<std::uint64_t>(a,b,out,imax_<std::uint64_t>); return;
    default: throw_error("maximum: dtype not implemented");
  }
}

void clamp(const Series& a, Series& out, double lo, double hi) {
  switch (a.scalar_type()) {
    case ScalarType::Float64: clamp_apply<double>(a,out,(double)lo,(double)hi); return;
    case ScalarType::Float32: clamp_apply<float>(a,out,(float)lo,(float)hi); return;
    case ScalarType::Float16: clamp_half<Float16>(a,out,lo,hi,float16_to_float,float16_from_float); return;
    case ScalarType::BFloat16: clamp_half<BFloat16>(a,out,lo,hi,bfloat16_to_float,bfloat16_from_float); return;
    case ScalarType::Int8: clamp_apply<std::int8_t>(a,out,(std::int8_t)lo,(std::int8_t)hi); return;
    case ScalarType::UInt8: clamp_apply<std::uint8_t>(a,out,(std::uint8_t)lo,(std::uint8_t)hi); return;
    case ScalarType::Int16: clamp_apply<std::int16_t>(a,out,(std::int16_t)lo,(std::int16_t)hi); return;
    case ScalarType::UInt16: clamp_apply<std::uint16_t>(a,out,(std::uint16_t)lo,(std::uint16_t)hi); return;
    case ScalarType::Int32: clamp_apply<std::int32_t>(a,out,(std::int32_t)lo,(std::int32_t)hi); return;
    case ScalarType::UInt32: clamp_apply<std::uint32_t>(a,out,(std::uint32_t)lo,(std::uint32_t)hi); return;
    case ScalarType::Int64: clamp_apply<std::int64_t>(a,out,(std::int64_t)lo,(std::int64_t)hi); return;
    case ScalarType::UInt64: clamp_apply<std::uint64_t>(a,out,(std::uint64_t)lo,(std::uint64_t)hi); return;
    default: throw_error("clamp: dtype not implemented");
  }
}

} // namespace pluss::codem::native::cpu
