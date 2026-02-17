#include "CodeM/native/cpu/UnaryFastOps.h"

#include "CodeM/core/Errors.h"
#include "CodeM/core/Float8.h"
#include "CodeM/core/Half.h"
#include "CodeM/core/SeriesIterator.h"

namespace pluss::codem::native::cpu {

template <class T>
static void relu_loop(const Series& a, Series& out) {
  SeriesIterator it = SeriesIterator::unary(a, out);
  auto* op = reinterpret_cast<T*>(out.raw_data());
  const auto* ap = reinterpret_cast<const T*>(a.raw_data());
  it.for_each_index([&](std::int64_t lin, std::int64_t ao, std::int64_t, std::int64_t) {
    const T x = ap[ao];
    op[lin] = x > static_cast<T>(0) ? x : static_cast<T>(0);
  });
}

void relu(const Series& a, Series& out) {
  switch (a.scalar_type()) {
    case ScalarType::Float64: relu_loop<double>(a, out); return;
    case ScalarType::Float32: relu_loop<float>(a, out); return;
    case ScalarType::Float16: {
      SeriesIterator it = SeriesIterator::unary(a, out);
      auto* op = reinterpret_cast<Float16*>(out.raw_data());
      const auto* ap = reinterpret_cast<const Float16*>(a.raw_data());
      it.for_each_index([&](std::int64_t lin, std::int64_t ao, std::int64_t, std::int64_t) {
        const float x = float16_to_float(ap[ao]);
        op[lin] = float16_from_float(x > 0.0f ? x : 0.0f);
      });
      return;
    }
    case ScalarType::BFloat16: {
      SeriesIterator it = SeriesIterator::unary(a, out);
      auto* op = reinterpret_cast<BFloat16*>(out.raw_data());
      const auto* ap = reinterpret_cast<const BFloat16*>(a.raw_data());
      it.for_each_index([&](std::int64_t lin, std::int64_t ao, std::int64_t, std::int64_t) {
        const float x = bfloat16_to_float(ap[ao]);
        op[lin] = bfloat16_from_float(x > 0.0f ? x : 0.0f);
      });
      return;
    }
    case ScalarType::Float8E4M3FN:
    case ScalarType::Float8E5M2: {
      SeriesIterator it = SeriesIterator::unary(a, out);
      if (a.scalar_type() == ScalarType::Float8E4M3FN) {
        auto* op = reinterpret_cast<Float8E4M3FN*>(out.raw_data());
        const auto* ap = reinterpret_cast<const Float8E4M3FN*>(a.raw_data());
        it.for_each_index([&](std::int64_t lin, std::int64_t ao, std::int64_t, std::int64_t) {
          const float x = float8e4m3fn_to_float(ap[ao]);
          op[lin] = float8e4m3fn_from_float(x > 0.0f ? x : 0.0f);
        });
      } else {
        auto* op = reinterpret_cast<Float8E5M2*>(out.raw_data());
        const auto* ap = reinterpret_cast<const Float8E5M2*>(a.raw_data());
        it.for_each_index([&](std::int64_t lin, std::int64_t ao, std::int64_t, std::int64_t) {
          const float x = float8e5m2_to_float(ap[ao]);
          op[lin] = float8e5m2_from_float(x > 0.0f ? x : 0.0f);
        });
      }
      return;
    }
    default: break;
  }
  throw_error("relu: dtype not implemented");
}

} // namespace pluss::codem::native::cpu
