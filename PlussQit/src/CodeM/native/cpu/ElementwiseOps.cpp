#include "CodeM/core/Errors.h"
#include "CodeM/core/SeriesIterator.h"
#include "CodeM/core/ScalarType.h"
#include "CodeM/core/Half.h"
#include "CodeM/core/Complex.h"

namespace pluss::codem::native::cpu {

template <class T, class Fn>
static void binary_loop(SeriesIterator& it, Fn&& fn) {
  auto* outp = reinterpret_cast<T*>(const_cast<std::byte*>(it.out().raw_data()));
  const auto* ap = reinterpret_cast<const T*>(it.a().raw_data());
  const auto* bp = reinterpret_cast<const T*>(it.b().raw_data());
  it.for_each_index([&](std::int64_t lin, std::int64_t ao, std::int64_t bo, std::int64_t) {
    outp[lin] = fn(ap[ao], bp[bo]);
  });
}


template <class HalfT, class ToF32, class FromF32, class Fn>
static void binary_half_loop(SeriesIterator& it, Fn&& fn, ToF32&& to, FromF32&& from) {
  auto* outp = reinterpret_cast<HalfT*>(const_cast<std::byte*>(it.out().raw_data()));
  const auto* ap = reinterpret_cast<const HalfT*>(it.a().raw_data());
  const auto* bp = reinterpret_cast<const HalfT*>(it.b().raw_data());
  it.for_each_index([&](std::int64_t lin, std::int64_t ao, std::int64_t bo, std::int64_t) {
    outp[lin] = from(fn(to(ap[ao]), to(bp[bo])));
  });
}

template <class HalfT, class ToF32, class FromF32, class Fn>
static void unary_half_loop(const Series& a, Series& out, Fn&& fn, ToF32&& to, FromF32&& from) {
  SeriesIterator it = SeriesIterator::unary(a, out);
  auto* outp = reinterpret_cast<HalfT*>(out.raw_data());
  const auto* ap = reinterpret_cast<const HalfT*>(a.raw_data());
  it.for_each_index([&](std::int64_t lin, std::int64_t ao, std::int64_t, std::int64_t) {
    outp[lin] = from(fn(to(ap[ao])));
  });
}


template <class T, class Fn>
static void unary_loop(const Series& a, Series& out, Fn&& fn) {
  SeriesIterator it = SeriesIterator::unary(a, out);
  auto* outp = reinterpret_cast<T*>(out.raw_data());
  const auto* ap = reinterpret_cast<const T*>(a.raw_data());
  it.for_each_index([&](std::int64_t lin, std::int64_t ao, std::int64_t, std::int64_t) {
    outp[lin] = fn(ap[ao]);
  });
}

void add(const Series& a, const Series& b, const Series& out) {
  SeriesIterator it = SeriesIterator::binary(a, b, out);
  switch (a.scalar_type()) {
    case ScalarType::Float64: binary_loop<double>(it, [](auto x, auto y){ return x + y; }); return;
    case ScalarType::Float32: binary_loop<float>(it, [](auto x, auto y){ return x + y; }); return;
    case ScalarType::Complex64: binary_loop<pluss::codem::Complex64>(it, [](auto x, auto y){ return x + y; }); return;
    case ScalarType::Complex128: binary_loop<pluss::codem::Complex128>(it, [](auto x, auto y){ return x + y; }); return;
    case ScalarType::Int64:   binary_loop<std::int64_t>(it, [](auto x, auto y){ return x + y; }); return;
    case ScalarType::Int32:   binary_loop<std::int32_t>(it, [](auto x, auto y){ return x + y; }); return;
    
    case ScalarType::Float16: binary_half_loop<Float16>(it, [](float x,float y){ return x + y; }, float16_to_float, float16_from_float); return;
    case ScalarType::BFloat16: binary_half_loop<BFloat16>(it, [](float x,float y){ return x + y; }, bfloat16_to_float, bfloat16_from_float); return;
    case ScalarType::Int16:   binary_loop<std::int16_t>(it, [](auto x, auto y){ return (std::int16_t)(x + y); }); return;
    case ScalarType::Int8:    binary_loop<std::int8_t>(it, [](auto x, auto y){ return (std::int8_t)(x + y); }); return;
    case ScalarType::UInt64:  binary_loop<std::uint64_t>(it, [](auto x, auto y){ return x + y; }); return;
    case ScalarType::UInt32:  binary_loop<std::uint32_t>(it, [](auto x, auto y){ return x + y; }); return;
    case ScalarType::UInt16:  binary_loop<std::uint16_t>(it, [](auto x, auto y){ return (std::uint16_t)(x + y); }); return;
    case ScalarType::UInt8:   binary_loop<std::uint8_t>(it, [](auto x, auto y){ return (std::uint8_t)(x + y); }); return;
    default: throw_error("add: dtype not implemented");
  }
}

void sub(const Series& a, const Series& b, const Series& out) {
  SeriesIterator it = SeriesIterator::binary(a, b, out);
  switch (a.scalar_type()) {
    case ScalarType::Float64: binary_loop<double>(it, [](auto x, auto y){ return x - y; }); return;
    case ScalarType::Float32: binary_loop<float>(it, [](auto x, auto y){ return x - y; }); return;
    case ScalarType::Complex64: binary_loop<pluss::codem::Complex64>(it, [](auto x, auto y){ return x - y; }); return;
    case ScalarType::Complex128: binary_loop<pluss::codem::Complex128>(it, [](auto x, auto y){ return x - y; }); return;
    case ScalarType::Int64:   binary_loop<std::int64_t>(it, [](auto x, auto y){ return x - y; }); return;
    case ScalarType::Int32:   binary_loop<std::int32_t>(it, [](auto x, auto y){ return x - y; }); return;
    
    case ScalarType::Float16: binary_half_loop<Float16>(it, [](float x,float y){ return x - y; }, float16_to_float, float16_from_float); return;
    case ScalarType::BFloat16: binary_half_loop<BFloat16>(it, [](float x,float y){ return x - y; }, bfloat16_to_float, bfloat16_from_float); return;
    case ScalarType::Int16:   binary_loop<std::int16_t>(it, [](auto x, auto y){ return (std::int16_t)(x - y); }); return;
    case ScalarType::Int8:    binary_loop<std::int8_t>(it, [](auto x, auto y){ return (std::int8_t)(x - y); }); return;
    case ScalarType::UInt64:  binary_loop<std::uint64_t>(it, [](auto x, auto y){ return x - y; }); return;
    case ScalarType::UInt32:  binary_loop<std::uint32_t>(it, [](auto x, auto y){ return x - y; }); return;
    case ScalarType::UInt16:  binary_loop<std::uint16_t>(it, [](auto x, auto y){ return (std::uint16_t)(x - y); }); return;
    case ScalarType::UInt8:   binary_loop<std::uint8_t>(it, [](auto x, auto y){ return (std::uint8_t)(x - y); }); return;
    default: throw_error("sub: dtype not implemented");
  }
}

void mul(const Series& a, const Series& b, const Series& out) {
  SeriesIterator it = SeriesIterator::binary(a, b, out);
  switch (a.scalar_type()) {
    case ScalarType::Float64: binary_loop<double>(it, [](auto x, auto y){ return x * y; }); return;
    case ScalarType::Float32: binary_loop<float>(it, [](auto x, auto y){ return x * y; }); return;
    case ScalarType::Complex64: binary_loop<pluss::codem::Complex64>(it, [](auto x, auto y){ return x * y; }); return;
    case ScalarType::Complex128: binary_loop<pluss::codem::Complex128>(it, [](auto x, auto y){ return x * y; }); return;
    case ScalarType::Int64:   binary_loop<std::int64_t>(it, [](auto x, auto y){ return x * y; }); return;
    case ScalarType::Int32:   binary_loop<std::int32_t>(it, [](auto x, auto y){ return x * y; }); return;
    
    case ScalarType::Float16: binary_half_loop<Float16>(it, [](float x,float y){ return x * y; }, float16_to_float, float16_from_float); return;
    case ScalarType::BFloat16: binary_half_loop<BFloat16>(it, [](float x,float y){ return x * y; }, bfloat16_to_float, bfloat16_from_float); return;
    case ScalarType::Int16:   binary_loop<std::int16_t>(it, [](auto x, auto y){ return (std::int16_t)(x * y); }); return;
    case ScalarType::Int8:    binary_loop<std::int8_t>(it, [](auto x, auto y){ return (std::int8_t)(x * y); }); return;
    case ScalarType::UInt64:  binary_loop<std::uint64_t>(it, [](auto x, auto y){ return x * y; }); return;
    case ScalarType::UInt32:  binary_loop<std::uint32_t>(it, [](auto x, auto y){ return x * y; }); return;
    case ScalarType::UInt16:  binary_loop<std::uint16_t>(it, [](auto x, auto y){ return (std::uint16_t)(x * y); }); return;
    case ScalarType::UInt8:   binary_loop<std::uint8_t>(it, [](auto x, auto y){ return (std::uint8_t)(x * y); }); return;
    default: throw_error("mul: dtype not implemented");
  }
}

void div(const Series& a, const Series& b, const Series& out) {
  SeriesIterator it = SeriesIterator::binary(a, b, out);
  switch (a.scalar_type()) {

    case ScalarType::Float16: binary_half_loop<Float16>(it, [](float x,float y){ return x / y; }, float16_to_float, float16_from_float); return;
    case ScalarType::BFloat16: binary_half_loop<BFloat16>(it, [](float x,float y){ return x / y; }, bfloat16_to_float, bfloat16_from_float); return;
    case ScalarType::Float64: binary_loop<double>(it, [](auto x, auto y){ return x / y; }); return;
    case ScalarType::Float32: binary_loop<float>(it, [](auto x, auto y){ return x / y; }); return;
    case ScalarType::Complex64: binary_loop<pluss::codem::Complex64>(it, [](auto x, auto y){ return x / y; }); return;
    case ScalarType::Complex128: binary_loop<pluss::codem::Complex128>(it, [](auto x, auto y){ return x / y; }); return;
    case ScalarType::Int64:   binary_loop<std::int64_t>(it, [](auto x, auto y){ PLUSS_CHECK(y!=0, "div: integral division by zero"); return (std::int64_t)(x / y); }); return;
    case ScalarType::Int32:   binary_loop<std::int32_t>(it, [](auto x, auto y){ PLUSS_CHECK(y!=0, "div: integral division by zero"); return (std::int32_t)(x / y); }); return;
    case ScalarType::Int16:   binary_loop<std::int16_t>(it, [](auto x, auto y){ PLUSS_CHECK(y!=0, "div: integral division by zero"); return (std::int16_t)(x / y); }); return;
    case ScalarType::Int8:    binary_loop<std::int8_t>(it, [](auto x, auto y){ PLUSS_CHECK(y!=0, "div: integral division by zero"); return (std::int8_t)(x / y); }); return;
    case ScalarType::UInt64:  binary_loop<std::uint64_t>(it, [](auto x, auto y){ PLUSS_CHECK(y!=0, "div: integral division by zero"); return (std::uint64_t)(x / y); }); return;
    case ScalarType::UInt32:  binary_loop<std::uint32_t>(it, [](auto x, auto y){ PLUSS_CHECK(y!=0, "div: integral division by zero"); return (std::uint32_t)(x / y); }); return;
    case ScalarType::UInt16:  binary_loop<std::uint16_t>(it, [](auto x, auto y){ PLUSS_CHECK(y!=0, "div: integral division by zero"); return (std::uint16_t)(x / y); }); return;
    case ScalarType::UInt8:   binary_loop<std::uint8_t>(it, [](auto x, auto y){ PLUSS_CHECK(y!=0, "div: integral division by zero"); return (std::uint8_t)(x / y); }); return;
    default: throw_error("div: dtype not implemented");
  }
}

void neg(const Series& a, Series& out) {
  switch (a.scalar_type()) {
    case ScalarType::Float64: unary_loop<double>(a, out, [](auto x){ return -x; }); return;
    case ScalarType::Float32: unary_loop<float>(a, out, [](auto x){ return -x; }); return;
    case ScalarType::Complex64: unary_loop<pluss::codem::Complex64>(a, out, [](auto x){ return -x; }); return;
    case ScalarType::Complex128: unary_loop<pluss::codem::Complex128>(a, out, [](auto x){ return -x; }); return;
    case ScalarType::Int64: unary_loop<std::int64_t>(a, out, [](auto x){ return -x; }); return;
    case ScalarType::Int32: unary_loop<std::int32_t>(a, out, [](auto x){ return -x; }); return;
    case ScalarType::Int16: unary_loop<std::int16_t>(a, out, [](auto x){ return (std::int16_t)(-x); }); return;
    case ScalarType::Int8: unary_loop<std::int8_t>(a, out, [](auto x){ return (std::int8_t)(-x); }); return;
    case ScalarType::Float16: unary_half_loop<Float16>(a, out, [](float x){ return -x; }, float16_to_float, float16_from_float); return;
    case ScalarType::BFloat16: unary_half_loop<BFloat16>(a, out, [](float x){ return -x; }, bfloat16_to_float, bfloat16_from_float); return;
    default: throw_error("neg: dtype not implemented");
  }
}

void abs(const Series& a, Series& out) {
  switch (a.scalar_type()) {
    case ScalarType::Float64: unary_loop<double>(a, out, [](auto x){ return x < 0 ? -x : x; }); return;
    case ScalarType::Float32: unary_loop<float>(a, out, [](auto x){ return x < 0 ? -x : x; }); return;
    case ScalarType::Int64: unary_loop<std::int64_t>(a, out, [](auto x){ return x < 0 ? -x : x; }); return;
    case ScalarType::Int32: unary_loop<std::int32_t>(a, out, [](auto x){ return x < 0 ? -x : x; }); return;
    case ScalarType::Int16: unary_loop<std::int16_t>(a, out, [](auto x){ return (std::int16_t)(x < 0 ? -x : x); }); return;
    case ScalarType::Int8: unary_loop<std::int8_t>(a, out, [](auto x){ return (std::int8_t)(x < 0 ? -x : x); }); return;
    case ScalarType::UInt64: unary_loop<std::uint64_t>(a, out, [](auto x){ return x; }); return;
    case ScalarType::UInt32: unary_loop<std::uint32_t>(a, out, [](auto x){ return x; }); return;
    case ScalarType::UInt16: unary_loop<std::uint16_t>(a, out, [](auto x){ return x; }); return;
    case ScalarType::UInt8: unary_loop<std::uint8_t>(a, out, [](auto x){ return x; }); return;
    case ScalarType::Float16: unary_half_loop<Float16>(a, out, [](float x){ return x < 0 ? -x : x; }, float16_to_float, float16_from_float); return;
    case ScalarType::BFloat16: unary_half_loop<BFloat16>(a, out, [](float x){ return x < 0 ? -x : x; }, bfloat16_to_float, bfloat16_from_float); return;
    default: throw_error("abs: dtype not implemented");
  }
}

} // namespace pluss::codem::native::cpu
