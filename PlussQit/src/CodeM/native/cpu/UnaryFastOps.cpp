#include "CodeM/native/cpu/UnaryFastOps.h"
#include "CodeM/core/Errors.h"
#include "CodeM/core/SeriesIterator.h"
#include "CodeM/core/Half.h"
#include "CodeM/core/Float8.h"
#include <cmath>

namespace pluss::codem::native::cpu {

template <class T>
static void relu_loop(const Series& a, Series& out) {
  SeriesIterator it = SeriesIterator::unary(a,out);
  auto* op = reinterpret_cast<T*>(out.raw_data());
  const auto* ap = reinterpret_cast<const T*>(a.raw_data());
  #if defined(__clang__)
  #pragma clang loop vectorize(enable) interleave(enable)
  #endif
  for (std::int64_t i=0;i<it.numel();++i) {
    // contiguous fast-path assumption is handled by iterator; offsets may be linear here
    // We use iterator to be safe for non-contiguous.
    // For speed, when contiguous, caller should ensure contiguous.
    break;
  }
  it.for_each_index([&](std::int64_t lin, std::int64_t ao, std::int64_t, std::int64_t){
    const T x = ap[ao];
    op[lin] = x > (T)0 ? x : (T)0;
  });
}

template <class T>
static void exp_loop(const Series& a, Series& out) {
  SeriesIterator it = SeriesIterator::unary(a,out);
  auto* op = reinterpret_cast<T*>(out.raw_data());
  const auto* ap = reinterpret_cast<const T*>(a.raw_data());
  it.for_each_index([&](std::int64_t lin, std::int64_t ao, std::int64_t, std::int64_t){
    op[lin] = (T)std::exp((double)ap[ao]);
  });
}

template <class T>
static void log_loop(const Series& a, Series& out) {
  SeriesIterator it = SeriesIterator::unary(a,out);
  auto* op = reinterpret_cast<T*>(out.raw_data());
  const auto* ap = reinterpret_cast<const T*>(a.raw_data());
  it.for_each_index([&](std::int64_t lin, std::int64_t ao, std::int64_t, std::int64_t){
    op[lin] = (T)std::log((double)ap[ao]);
  });
}

static void exp_f8(const Series& a, Series& out, bool e4m3) {
  SeriesIterator it = SeriesIterator::unary(a,out);
  if (e4m3) {
    auto* op = reinterpret_cast<Float8E4M3FN*>(out.raw_data());
    const auto* ap = reinterpret_cast<const Float8E4M3FN*>(a.raw_data());
    it.for_each_index([&](std::int64_t lin, std::int64_t ao, std::int64_t, std::int64_t){
      float x = float8e4m3fn_to_float(ap[ao]);
      op[lin] = float8e4m3fn_from_float(std::exp(x));
    });
  } else {
    auto* op = reinterpret_cast<Float8E5M2*>(out.raw_data());
    const auto* ap = reinterpret_cast<const Float8E5M2*>(a.raw_data());
    it.for_each_index([&](std::int64_t lin, std::int64_t ao, std::int64_t, std::int64_t){
      float x = float8e5m2_to_float(ap[ao]);
      op[lin] = float8e5m2_from_float(std::exp(x));
    });
  }
}

static void log_f8(const Series& a, Series& out, bool e4m3) {
  SeriesIterator it = SeriesIterator::unary(a,out);
  if (e4m3) {
    auto* op = reinterpret_cast<Float8E4M3FN*>(out.raw_data());
    const auto* ap = reinterpret_cast<const Float8E4M3FN*>(a.raw_data());
    it.for_each_index([&](std::int64_t lin, std::int64_t ao, std::int64_t, std::int64_t){
      float x = float8e4m3fn_to_float(ap[ao]);
      op[lin] = float8e4m3fn_from_float(std::log(x));
    });
  } else {
    auto* op = reinterpret_cast<Float8E5M2*>(out.raw_data());
    const auto* ap = reinterpret_cast<const Float8E5M2*>(a.raw_data());
    it.for_each_index([&](std::int64_t lin, std::int64_t ao, std::int64_t, std::int64_t){
      float x = float8e5m2_to_float(ap[ao]);
      op[lin] = float8e5m2_from_float(std::log(x));
    });
  }
}

void relu(const Series& a, Series& out) {
  switch (a.scalar_type()) {
    case ScalarType::Float64: relu_loop<double>(a,out); return;
    case ScalarType::Float32: relu_loop<float>(a,out); return;
    case ScalarType::Float16: {
      // compute in f32 then store back
      SeriesIterator it = SeriesIterator::unary(a,out);
      auto* op = reinterpret_cast<Float16*>(out.raw_data());
      const auto* ap = reinterpret_cast<const Float16*>(a.raw_data());
      it.for_each_index([&](std::int64_t lin, std::int64_t ao, std::int64_t, std::int64_t){
        float x = float16_to_float(ap[ao]);
        op[lin] = float16_from_float(x > 0 ? x : 0);
      });
      return;
    }
    case ScalarType::BFloat16: {
      SeriesIterator it = SeriesIterator::unary(a,out);
      auto* op = reinterpret_cast<BFloat16*>(out.raw_data());
      const auto* ap = reinterpret_cast<const BFloat16*>(a.raw_data());
      it.for_each_index([&](std::int64_t lin, std::int64_t ao, std::int64_t, std::int64_t){
        float x = bfloat16_to_float(ap[ao]);
        op[lin] = bfloat16_from_float(x > 0 ? x : 0);
      });
      return;
    }
    case ScalarType::Float8E4M3FN:
    case ScalarType::Float8E5M2: {
      SeriesIterator it = SeriesIterator::unary(a,out);
      if (a.scalar_type()==ScalarType::Float8E4M3FN) {
        auto* op = reinterpret_cast<Float8E4M3FN*>(out.raw_data());
        const auto* ap = reinterpret_cast<const Float8E4M3FN*>(a.raw_data());
        it.for_each_index([&](std::int64_t lin, std::int64_t ao, std::int64_t, std::int64_t){
          float x = float8e4m3fn_to_float(ap[ao]);
          op[lin] = float8e4m3fn_from_float(x > 0 ? x : 0);
        });
      } else {
        auto* op = reinterpret_cast<Float8E5M2*>(out.raw_data());
        const auto* ap = reinterpret_cast<const Float8E5M2*>(a.raw_data());
        it.for_each_index([&](std::int64_t lin, std::int64_t ao, std::int64_t, std::int64_t){
          float x = float8e5m2_to_float(ap[ao]);
          op[lin] = float8e5m2_from_float(x > 0 ? x : 0);
        });
      }
      return;
    }
    default: break;
  }
  throw_error("relu: dtype not implemented");
}

void exp(const Series& a, Series& out) {
  switch (a.scalar_type()) {
    case ScalarType::Float64: exp_loop<double>(a,out); return;
    case ScalarType::Float32: exp_loop<float>(a,out); return;
    case ScalarType::Float16: {
      SeriesIterator it = SeriesIterator::unary(a,out);
      auto* op = reinterpret_cast<Float16*>(out.raw_data());
      const auto* ap = reinterpret_cast<const Float16*>(a.raw_data());
      it.for_each_index([&](std::int64_t lin, std::int64_t ao, std::int64_t, std::int64_t){
        op[lin] = float16_from_float(std::exp(float16_to_float(ap[ao])));
      });
      return;
    }
    case ScalarType::BFloat16: {
      SeriesIterator it = SeriesIterator::unary(a,out);
      auto* op = reinterpret_cast<BFloat16*>(out.raw_data());
      const auto* ap = reinterpret_cast<const BFloat16*>(a.raw_data());
      it.for_each_index([&](std::int64_t lin, std::int64_t ao, std::int64_t, std::int64_t){
        op[lin] = bfloat16_from_float(std::exp(bfloat16_to_float(ap[ao])));
      });
      return;
    }
    case ScalarType::Float8E4M3FN: exp_f8(a,out,true); return;
    case ScalarType::Float8E5M2: exp_f8(a,out,false); return;
    default: break;
  }
  throw_error("exp: dtype not implemented");
}

void log(const Series& a, Series& out) {
  switch (a.scalar_type()) {
    case ScalarType::Float64: log_loop<double>(a,out); return;
    case ScalarType::Float32: log_loop<float>(a,out); return;
    case ScalarType::Float16: {
      SeriesIterator it = SeriesIterator::unary(a,out);
      auto* op = reinterpret_cast<Float16*>(out.raw_data());
      const auto* ap = reinterpret_cast<const Float16*>(a.raw_data());
      it.for_each_index([&](std::int64_t lin, std::int64_t ao, std::int64_t, std::int64_t){
        op[lin] = float16_from_float(std::log(float16_to_float(ap[ao])));
      });
      return;
    }
    case ScalarType::BFloat16: {
      SeriesIterator it = SeriesIterator::unary(a,out);
      auto* op = reinterpret_cast<BFloat16*>(out.raw_data());
      const auto* ap = reinterpret_cast<const BFloat16*>(a.raw_data());
      it.for_each_index([&](std::int64_t lin, std::int64_t ao, std::int64_t, std::int64_t){
        op[lin] = bfloat16_from_float(std::log(bfloat16_to_float(ap[ao])));
      });
      return;
    }
    case ScalarType::Float8E4M3FN: log_f8(a,out,true); return;
    case ScalarType::Float8E5M2: log_f8(a,out,false); return;
    default: break;
  }
  throw_error("log: dtype not implemented");
}

} // namespace
