#include "CodeM/core/Errors.h"
#include "CodeM/core/SeriesSlim.h"
#include "CodeM/core/ScalarType.h"
#include "CodeM/core/SeriesIterator.h"

#include <cstdint>

namespace pluss::codem::native::cpu {

template <class T, class Op>
static void binary_int(const Series& a, const Series& b, Series& out, Op&& op) {
  SeriesIterator it = SeriesIterator::binary(a, b, out);
  const auto* ap = reinterpret_cast<const T*>(a.raw_data());
  const auto* bp = reinterpret_cast<const T*>(b.raw_data());
  auto* optr = reinterpret_cast<T*>(out.raw_data());
  it.for_each_index([&](std::int64_t lin, std::int64_t ao, std::int64_t bo, std::int64_t){
    optr[lin] = op(ap[ao], bp[bo]);
  });
}

template <class T, class Op>
static void unary_int(const Series& a, Series& out, Op&& op) {
  SeriesIterator it = SeriesIterator::unary(a, out);
  const auto* ap = reinterpret_cast<const T*>(a.raw_data());
  auto* optr = reinterpret_cast<T*>(out.raw_data());
  it.for_each_index([&](std::int64_t lin, std::int64_t ao, std::int64_t, std::int64_t){
    optr[lin] = op(ap[ao]);
  });
}

static void ensure_int_like(ScalarType st) {
  switch (st) {
    case ScalarType::Bool:
    case ScalarType::UInt8: case ScalarType::UInt16: case ScalarType::UInt32: case ScalarType::UInt64:
    case ScalarType::Int8: case ScalarType::Int16: case ScalarType::Int32: case ScalarType::Int64:
      return;
    default:
      break;
  }
  throw_error("bitwise: dtype must be integer/bool");
}

void bitwise_and(const Series& a, const Series& b, const Series& out_) {
  auto& out = const_cast<Series&>(out_);
  ensure_int_like(a.scalar_type());
  switch (a.scalar_type()) {
    case ScalarType::Bool: binary_int<std::uint8_t>(a,b,out,[](auto x,auto y){ return (std::uint8_t)((x!=0) & (y!=0));}); return;
    case ScalarType::UInt8: binary_int<std::uint8_t>(a,b,out,[](auto x,auto y){ return (std::uint8_t)(x & y);}); return;
    case ScalarType::UInt16: binary_int<std::uint16_t>(a,b,out,[](auto x,auto y){ return (std::uint16_t)(x & y);}); return;
    case ScalarType::UInt32: binary_int<std::uint32_t>(a,b,out,[](auto x,auto y){ return (std::uint32_t)(x & y);}); return;
    case ScalarType::UInt64: binary_int<std::uint64_t>(a,b,out,[](auto x,auto y){ return (std::uint64_t)(x & y);}); return;
    case ScalarType::Int8: binary_int<std::int8_t>(a,b,out,[](auto x,auto y){ return (std::int8_t)(x & y);}); return;
    case ScalarType::Int16: binary_int<std::int16_t>(a,b,out,[](auto x,auto y){ return (std::int16_t)(x & y);}); return;
    case ScalarType::Int32: binary_int<std::int32_t>(a,b,out,[](auto x,auto y){ return (std::int32_t)(x & y);}); return;
    case ScalarType::Int64: binary_int<std::int64_t>(a,b,out,[](auto x,auto y){ return (std::int64_t)(x & y);}); return;
    default: break;
  }
}

void bitwise_or(const Series& a, const Series& b, const Series& out_) {
  auto& out = const_cast<Series&>(out_);
  ensure_int_like(a.scalar_type());
  switch (a.scalar_type()) {
    case ScalarType::Bool: binary_int<std::uint8_t>(a,b,out,[](auto x,auto y){ return (std::uint8_t)((x!=0) | (y!=0));}); return;
    case ScalarType::UInt8: binary_int<std::uint8_t>(a,b,out,[](auto x,auto y){ return (std::uint8_t)(x | y);}); return;
    case ScalarType::UInt16: binary_int<std::uint16_t>(a,b,out,[](auto x,auto y){ return (std::uint16_t)(x | y);}); return;
    case ScalarType::UInt32: binary_int<std::uint32_t>(a,b,out,[](auto x,auto y){ return (std::uint32_t)(x | y);}); return;
    case ScalarType::UInt64: binary_int<std::uint64_t>(a,b,out,[](auto x,auto y){ return (std::uint64_t)(x | y);}); return;
    case ScalarType::Int8: binary_int<std::int8_t>(a,b,out,[](auto x,auto y){ return (std::int8_t)(x | y);}); return;
    case ScalarType::Int16: binary_int<std::int16_t>(a,b,out,[](auto x,auto y){ return (std::int16_t)(x | y);}); return;
    case ScalarType::Int32: binary_int<std::int32_t>(a,b,out,[](auto x,auto y){ return (std::int32_t)(x | y);}); return;
    case ScalarType::Int64: binary_int<std::int64_t>(a,b,out,[](auto x,auto y){ return (std::int64_t)(x | y);}); return;
    default: break;
  }
}

void bitwise_xor(const Series& a, const Series& b, const Series& out_) {
  auto& out = const_cast<Series&>(out_);
  ensure_int_like(a.scalar_type());
  switch (a.scalar_type()) {
    case ScalarType::Bool: binary_int<std::uint8_t>(a,b,out,[](auto x,auto y){ return (std::uint8_t)((x!=0) ^ (y!=0));}); return;
    case ScalarType::UInt8: binary_int<std::uint8_t>(a,b,out,[](auto x,auto y){ return (std::uint8_t)(x ^ y);}); return;
    case ScalarType::UInt16: binary_int<std::uint16_t>(a,b,out,[](auto x,auto y){ return (std::uint16_t)(x ^ y);}); return;
    case ScalarType::UInt32: binary_int<std::uint32_t>(a,b,out,[](auto x,auto y){ return (std::uint32_t)(x ^ y);}); return;
    case ScalarType::UInt64: binary_int<std::uint64_t>(a,b,out,[](auto x,auto y){ return (std::uint64_t)(x ^ y);}); return;
    case ScalarType::Int8: binary_int<std::int8_t>(a,b,out,[](auto x,auto y){ return (std::int8_t)(x ^ y);}); return;
    case ScalarType::Int16: binary_int<std::int16_t>(a,b,out,[](auto x,auto y){ return (std::int16_t)(x ^ y);}); return;
    case ScalarType::Int32: binary_int<std::int32_t>(a,b,out,[](auto x,auto y){ return (std::int32_t)(x ^ y);}); return;
    case ScalarType::Int64: binary_int<std::int64_t>(a,b,out,[](auto x,auto y){ return (std::int64_t)(x ^ y);}); return;
    default: break;
  }
}

void bitwise_not(const Series& a, Series& out) {
  ensure_int_like(a.scalar_type());
  switch (a.scalar_type()) {
    case ScalarType::Bool: unary_int<std::uint8_t>(a,out,[](auto x){ return (std::uint8_t)((x==0)?1:0);}); return;
    case ScalarType::UInt8: unary_int<std::uint8_t>(a,out,[](auto x){ return (std::uint8_t)(~x);}); return;
    case ScalarType::UInt16: unary_int<std::uint16_t>(a,out,[](auto x){ return (std::uint16_t)(~x);}); return;
    case ScalarType::UInt32: unary_int<std::uint32_t>(a,out,[](auto x){ return (std::uint32_t)(~x);}); return;
    case ScalarType::UInt64: unary_int<std::uint64_t>(a,out,[](auto x){ return (std::uint64_t)(~x);}); return;
    case ScalarType::Int8: unary_int<std::int8_t>(a,out,[](auto x){ return (std::int8_t)(~x);}); return;
    case ScalarType::Int16: unary_int<std::int16_t>(a,out,[](auto x){ return (std::int16_t)(~x);}); return;
    case ScalarType::Int32: unary_int<std::int32_t>(a,out,[](auto x){ return (std::int32_t)(~x);}); return;
    case ScalarType::Int64: unary_int<std::int64_t>(a,out,[](auto x){ return (std::int64_t)(~x);}); return;
    default: break;
  }
}

} // namespace pluss::codem::native::cpu
