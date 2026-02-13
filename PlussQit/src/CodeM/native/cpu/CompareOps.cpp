#include "CodeM/native/cpu/CompareOps.h"
#include "CodeM/core/Errors.h"
#include "CodeM/core/SeriesIterator.h"
#include "CodeM/core/Half.h"
#include "CodeM/core/Complex.h"
#include <complex>
#include <cstdint>

namespace pluss::codem::native::cpu {

template <class T, class Pred>
static void cmp_loop(SeriesIterator& it, Pred&& pred) {
  auto* outp = reinterpret_cast<std::uint8_t*>(const_cast<std::byte*>(it.out().raw_data()));
  const auto* ap = reinterpret_cast<const T*>(it.a().raw_data());
  const auto* bp = reinterpret_cast<const T*>(it.b().raw_data());
  it.for_each_index([&](std::int64_t lin, std::int64_t ao, std::int64_t bo, std::int64_t) {
    outp[lin] = pred(ap[ao], bp[bo]) ? 1 : 0;
  });
}

template <class HalfT, class ToF32, class Pred>
static void cmp_half_loop(SeriesIterator& it, Pred&& pred, ToF32&& to) {
  auto* outp = reinterpret_cast<std::uint8_t*>(const_cast<std::byte*>(it.out().raw_data()));
  const auto* ap = reinterpret_cast<const HalfT*>(it.a().raw_data());
  const auto* bp = reinterpret_cast<const HalfT*>(it.b().raw_data());
  it.for_each_index([&](std::int64_t lin, std::int64_t ao, std::int64_t bo, std::int64_t) {
    outp[lin] = pred(to(ap[ao]), to(bp[bo])) ? 1 : 0;
  });
}

template <class Pred>
static void dispatch_cmp(const Series& a, const Series& b, const Series& out, Pred&& pred, const char* name) {
  PLUSS_CHECK(out.scalar_type() == ScalarType::Bool, "compare: out must be Bool");
  SeriesIterator it = SeriesIterator::binary(a, b, out);
  switch (a.scalar_type()) {
    case ScalarType::Float64: cmp_loop<double>(it, pred); return;
    case ScalarType::Float32: cmp_loop<float>(it, pred); return;
    case ScalarType::Float16: cmp_half_loop<Float16>(it, pred, float16_to_float); return;
    case ScalarType::BFloat16: cmp_half_loop<BFloat16>(it, pred, bfloat16_to_float); return;
    case ScalarType::Int64: cmp_loop<std::int64_t>(it, pred); return;
    case ScalarType::Int32: cmp_loop<std::int32_t>(it, pred); return;
    case ScalarType::Int16: cmp_loop<std::int16_t>(it, pred); return;
    case ScalarType::Int8:  cmp_loop<std::int8_t>(it, pred); return;
    case ScalarType::UInt64: cmp_loop<std::uint64_t>(it, pred); return;
    case ScalarType::UInt32: cmp_loop<std::uint32_t>(it, pred); return;
    case ScalarType::UInt16: cmp_loop<std::uint16_t>(it, pred); return;
    case ScalarType::UInt8:  cmp_loop<std::uint8_t>(it, pred); return;
    case ScalarType::Bool:   cmp_loop<std::uint8_t>(it, pred); return;
    default: break;
  }
  throw_error(std::string(name) + ": dtype not implemented");
}

void eq(const Series& a, const Series& b, const Series& out) { dispatch_cmp(a,b,out,[](auto x, auto y){return x==y;}, "eq"); }
void ne(const Series& a, const Series& b, const Series& out) { dispatch_cmp(a,b,out,[](auto x, auto y){return x!=y;}, "ne"); }
void lt(const Series& a, const Series& b, const Series& out) { dispatch_cmp(a,b,out,[](auto x, auto y){return x< y;}, "lt"); }
void le(const Series& a, const Series& b, const Series& out) { dispatch_cmp(a,b,out,[](auto x, auto y){return x<=y;}, "le"); }
void gt(const Series& a, const Series& b, const Series& out) { dispatch_cmp(a,b,out,[](auto x, auto y){return x> y;}, "gt"); }
void ge(const Series& a, const Series& b, const Series& out) { dispatch_cmp(a,b,out,[](auto x, auto y){return x>=y;}, "ge"); }

} // namespace
