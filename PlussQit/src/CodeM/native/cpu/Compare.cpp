#include "CodeM/core/Errors.h"
#include "CodeM/core/Half.h"
#include "CodeM/core/SeriesSlim.h"
#include "CodeM/core/ScalarType.h"
#include "CodeM/core/SeriesIterator.h"

namespace pluss::codem::native::cpu {

template <class T, class Op>
static void cmp_apply(const Series& a, const Series& b, Series& out, Op&& op) {
  SeriesIterator it = SeriesIterator::binary(a, b, out);
  const auto* ap = reinterpret_cast<const T*>(a.raw_data());
  const auto* bp = reinterpret_cast<const T*>(b.raw_data());
  auto* optr = reinterpret_cast<std::uint8_t*>(out.raw_data());
  it.for_each_index([&](std::int64_t lin, std::int64_t ao, std::int64_t bo, std::int64_t){
    optr[lin] = op(ap[ao], bp[bo]) ? 1 : 0;
  });
}

template <class HalfT, class ToF32, class Op>
static void cmp_half_apply(const Series& a, const Series& b, Series& out, Op&& op, ToF32&& to) {
  SeriesIterator it = SeriesIterator::binary(a, b, out);
  const auto* ap = reinterpret_cast<const HalfT*>(a.raw_data());
  const auto* bp = reinterpret_cast<const HalfT*>(b.raw_data());
  auto* optr = reinterpret_cast<std::uint8_t*>(out.raw_data());
  it.for_each_index([&](std::int64_t lin, std::int64_t ao, std::int64_t bo, std::int64_t){
    optr[lin] = op(to(ap[ao]), to(bp[bo])) ? 1 : 0;
  });
}

#define DEFINE_CMP(name, OP) \
  void name(const Series& a, const Series& b, const Series& out_) { \
    auto& out = const_cast<Series&>(out_); \
    if (out.scalar_type() != ScalarType::Bool) throw_error(#name ": out must be bool"); \
    switch (a.scalar_type()) { \
      case ScalarType::Bool:    cmp_apply<std::uint8_t>(a,b,out,[](auto x,auto y){ return (x!=0) OP (y!=0); }); return; \
      case ScalarType::UInt8:   cmp_apply<std::uint8_t>(a,b,out,[](auto x,auto y){ return x OP y; }); return; \
      case ScalarType::UInt16:  cmp_apply<std::uint16_t>(a,b,out,[](auto x,auto y){ return x OP y; }); return; \
      case ScalarType::UInt32:  cmp_apply<std::uint32_t>(a,b,out,[](auto x,auto y){ return x OP y; }); return; \
      case ScalarType::UInt64:  cmp_apply<std::uint64_t>(a,b,out,[](auto x,auto y){ return x OP y; }); return; \
      case ScalarType::Int8:    cmp_apply<std::int8_t>(a,b,out,[](auto x,auto y){ return x OP y; }); return; \
      case ScalarType::Int16:   cmp_apply<std::int16_t>(a,b,out,[](auto x,auto y){ return x OP y; }); return; \
      case ScalarType::Int32:   cmp_apply<std::int32_t>(a,b,out,[](auto x,auto y){ return x OP y; }); return; \
      case ScalarType::Int64:   cmp_apply<std::int64_t>(a,b,out,[](auto x,auto y){ return x OP y; }); return; \
      case ScalarType::Float64: cmp_apply<double>(a, b, out, [](double x,double y){ return x OP y; }); return; \
      case ScalarType::Float32: cmp_apply<float>(a, b, out, [](float x,float y){ return x OP y; }); return; \
      case ScalarType::Float16: cmp_half_apply<Float16>(a,b,out,[](float x,float y){ return x OP y; }, float16_to_float); return; \
      case ScalarType::BFloat16: cmp_half_apply<BFloat16>(a,b,out,[](float x,float y){ return x OP y; }, bfloat16_to_float); return; \
      default: throw_error(#name ": dtype not implemented"); \
    } \
  }

DEFINE_CMP(eq, ==)
DEFINE_CMP(ne, !=)
DEFINE_CMP(lt, <)
DEFINE_CMP(le, <=)
DEFINE_CMP(gt, >)
DEFINE_CMP(ge, >=)

#undef DEFINE_CMP

} // namespace pluss::codem::native::cpu
