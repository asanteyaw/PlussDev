#include "CodeM/core/Errors.h"
#include "CodeM/core/Series.h"
#include "CodeM/core/ScalarType.h"
#include "CodeM/native/cpu/Reductions.h"

#include <limits>
#include <cstring>

namespace pluss::codem::native::cpu {

Series mean(const Series& a, std::int64_t dim, bool keepdim) {
  auto s = sum(a, dim, keepdim);
  // divide by reduction size
  auto aa = a.contiguous();
  const auto nd = aa.dim();
  if (dim < 0) dim += nd;
  const auto reduce_n = aa.sizes()[(std::size_t)dim];

  if (s.scalar_type() == ScalarType::Float64) {
    auto* p = reinterpret_cast<double*>(s.raw_data());
    for (std::int64_t i=0;i<s.numel();++i) p[i] /= (double)reduce_n;
    return s;
  }
  if (s.scalar_type() == ScalarType::Float32) {
    auto* p = reinterpret_cast<float*>(s.raw_data());
    for (std::int64_t i=0;i<s.numel();++i) p[i] /= (float)reduce_n;
    return s;
  }
  throw_error("mean: dtype not implemented");
}

Series amax(const Series& a, std::int64_t dim, bool keepdim) {
  if (!a.defined()) throw_error("amax: undefined");
  auto aa = a.contiguous();
  const auto nd = aa.dim();
  if (nd == 0) return aa.clone();
  if (dim < 0) dim += nd;
  if (dim < 0 || dim >= nd) throw_error("amax: dim out of range");

  // output sizes
  auto out_sizes = std::vector<std::int64_t>(aa.sizes().begin(), aa.sizes().end());
  if (keepdim) out_sizes[(std::size_t)dim] = 1;
  else out_sizes.erase(out_sizes.begin() + dim);
  Series out = Series::empty(out_sizes, aa.scalar_type());

  const auto reduce_n = aa.sizes()[(std::size_t)dim];
  std::int64_t outer = 1;
  for (std::int64_t i=0;i<dim;++i) outer *= aa.sizes()[(std::size_t)i];
  std::int64_t inner = 1;
  for (std::int64_t i=dim+1;i<nd;++i) inner *= aa.sizes()[(std::size_t)i];

  if (aa.scalar_type() == ScalarType::Float64) {
    const auto* p = reinterpret_cast<const double*>(aa.raw_data());
    auto* q = reinterpret_cast<double*>(out.raw_data());
    for (std::int64_t o=0;o<outer;++o){
      for (std::int64_t i=0;i<inner;++i){
        double m = -std::numeric_limits<double>::infinity();
        const std::int64_t base = o*reduce_n*inner + i;
        for (std::int64_t r=0;r<reduce_n;++r) m = std::max(m, p[base + r*inner]);
        q[o*inner + i] = m;
      }
    }
    return out;
  }
  if (aa.scalar_type() == ScalarType::Float32) {
    const auto* p = reinterpret_cast<const float*>(aa.raw_data());
    auto* q = reinterpret_cast<float*>(out.raw_data());
    for (std::int64_t o=0;o<outer;++o){
      for (std::int64_t i=0;i<inner;++i){
        float m = -std::numeric_limits<float>::infinity();
        const std::int64_t base = o*reduce_n*inner + i;
        for (std::int64_t r=0;r<reduce_n;++r) m = std::max(m, p[base + r*inner]);
        q[o*inner + i] = m;
      }
    }
    return out;
  }
  throw_error("amax: dtype not implemented");
}

Series argmax(const Series& a, std::int64_t dim, bool keepdim) {
  if (!a.defined()) throw_error("argmax: undefined");
  auto aa = a.contiguous();
  const auto nd = aa.dim();
  if (nd == 0) {
    Series out = Series::zeros({}, ScalarType::Int64);
    *reinterpret_cast<std::int64_t*>(out.raw_data()) = 0;
    return out;
  }
  if (dim < 0) dim += nd;
  if (dim < 0 || dim >= nd) throw_error("argmax: dim out of range");

  auto out_sizes = std::vector<std::int64_t>(aa.sizes().begin(), aa.sizes().end());
  if (keepdim) out_sizes[(std::size_t)dim] = 1;
  else out_sizes.erase(out_sizes.begin() + dim);
  Series out = Series::empty(out_sizes, ScalarType::Int64);

  const auto reduce_n = aa.sizes()[(std::size_t)dim];
  std::int64_t outer = 1;
  for (std::int64_t i=0;i<dim;++i) outer *= aa.sizes()[(std::size_t)i];
  std::int64_t inner = 1;
  for (std::int64_t i=dim+1;i<nd;++i) inner *= aa.sizes()[(std::size_t)i];

  auto* q = reinterpret_cast<std::int64_t*>(out.raw_data());

  if (aa.scalar_type() == ScalarType::Float64) {
    const auto* p = reinterpret_cast<const double*>(aa.raw_data());
    for (std::int64_t o=0;o<outer;++o){
      for (std::int64_t i=0;i<inner;++i){
        double best = -std::numeric_limits<double>::infinity();
        std::int64_t arg = 0;
        const std::int64_t base = o*reduce_n*inner + i;
        for (std::int64_t r=0;r<reduce_n;++r){
          const double v = p[base + r*inner];
          if (v > best) { best = v; arg = r; }
        }
        q[o*inner + i] = arg;
      }
    }
    return out;
  }
  if (aa.scalar_type() == ScalarType::Float32) {
    const auto* p = reinterpret_cast<const float*>(aa.raw_data());
    for (std::int64_t o=0;o<outer;++o){
      for (std::int64_t i=0;i<inner;++i){
        float best = -std::numeric_limits<float>::infinity();
        std::int64_t arg = 0;
        const std::int64_t base = o*reduce_n*inner + i;
        for (std::int64_t r=0;r<reduce_n;++r){
          const float v = p[base + r*inner];
          if (v > best) { best = v; arg = r; }
        }
        q[o*inner + i] = arg;
      }
    }
    return out;
  }
  throw_error("argmax: dtype not implemented");
}

} // namespace pluss::codem::native::cpu


#include <limits>

namespace pluss::codem::native::cpu {

Series amin(const Series& a, std::int64_t dim, bool keepdim) {
  if (!a.defined()) throw_error("amin: undefined");
  auto aa = a.contiguous();
  const auto nd = aa.dim();
  if (nd == 0) return aa.clone();
  if (dim < 0) dim += nd;
  if (dim < 0 || dim >= nd) throw_error("amin: dim out of range");

  auto out_sizes = std::vector<std::int64_t>(aa.sizes().begin(), aa.sizes().end());
  if (keepdim) out_sizes[(std::size_t)dim] = 1;
  else out_sizes.erase(out_sizes.begin() + dim);
  Series out = Series::empty(out_sizes, aa.scalar_type());

  const auto reduce_n = aa.sizes()[(std::size_t)dim];
  std::int64_t outer = 1;
  for (std::int64_t i=0;i<dim;++i) outer *= aa.sizes()[(std::size_t)i];
  std::int64_t inner = 1;
  for (std::int64_t i=dim+1;i<nd;++i) inner *= aa.sizes()[(std::size_t)i];

  if (aa.scalar_type() == ScalarType::Float64) {
    const auto* p = reinterpret_cast<const double*>(aa.raw_data());
    auto* q = reinterpret_cast<double*>(out.raw_data());
    for (std::int64_t o=0;o<outer;++o){
      for (std::int64_t i=0;i<inner;++i){
        double m = std::numeric_limits<double>::infinity();
        const std::int64_t base = o*reduce_n*inner + i;
        for (std::int64_t r=0;r<reduce_n;++r) m = std::min(m, p[base + r*inner]);
        q[o*inner + i] = m;
      }
    }
    return out;
  }
  if (aa.scalar_type() == ScalarType::Float32) {
    const auto* p = reinterpret_cast<const float*>(aa.raw_data());
    auto* q = reinterpret_cast<float*>(out.raw_data());
    for (std::int64_t o=0;o<outer;++o){
      for (std::int64_t i=0;i<inner;++i){
        float m = std::numeric_limits<float>::infinity();
        const std::int64_t base = o*reduce_n*inner + i;
        for (std::int64_t r=0;r<reduce_n;++r) m = std::min(m, p[base + r*inner]);
        q[o*inner + i] = m;
      }
    }
    return out;
  }
  throw_error("amin: dtype not implemented");
}

Series argmin(const Series& a, std::int64_t dim, bool keepdim) {
  if (!a.defined()) throw_error("argmin: undefined");
  auto aa = a.contiguous();
  const auto nd = aa.dim();
  if (nd == 0) {
    Series out = Series::zeros({}, ScalarType::Int64);
    *reinterpret_cast<std::int64_t*>(out.raw_data()) = 0;
    return out;
  }
  if (dim < 0) dim += nd;
  if (dim < 0 || dim >= nd) throw_error("argmin: dim out of range");

  auto out_sizes = std::vector<std::int64_t>(aa.sizes().begin(), aa.sizes().end());
  if (keepdim) out_sizes[(std::size_t)dim] = 1;
  else out_sizes.erase(out_sizes.begin() + dim);
  Series out = Series::empty(out_sizes, ScalarType::Int64);

  const auto reduce_n = aa.sizes()[(std::size_t)dim];
  std::int64_t outer = 1;
  for (std::int64_t i=0;i<dim;++i) outer *= aa.sizes()[(std::size_t)i];
  std::int64_t inner = 1;
  for (std::int64_t i=dim+1;i<nd;++i) inner *= aa.sizes()[(std::size_t)i];

  auto* q = reinterpret_cast<std::int64_t*>(out.raw_data());

  if (aa.scalar_type() == ScalarType::Float64) {
    const auto* p = reinterpret_cast<const double*>(aa.raw_data());
    for (std::int64_t o=0;o<outer;++o){
      for (std::int64_t i=0;i<inner;++i){
        double best = std::numeric_limits<double>::infinity();
        std::int64_t arg = 0;
        const std::int64_t base = o*reduce_n*inner + i;
        for (std::int64_t r=0;r<reduce_n;++r){
          const double v = p[base + r*inner];
          if (v < best) { best = v; arg = r; }
        }
        q[o*inner + i] = arg;
      }
    }
    return out;
  }
  if (aa.scalar_type() == ScalarType::Float32) {
    const auto* p = reinterpret_cast<const float*>(aa.raw_data());
    for (std::int64_t o=0;o<outer;++o){
      for (std::int64_t i=0;i<inner;++i){
        float best = std::numeric_limits<float>::infinity();
        std::int64_t arg = 0;
        const std::int64_t base = o*reduce_n*inner + i;
        for (std::int64_t r=0;r<reduce_n;++r){
          const float v = p[base + r*inner];
          if (v < best) { best = v; arg = r; }
        }
        q[o*inner + i] = arg;
      }
    }
    return out;
  }
  throw_error("argmin: dtype not implemented");
}

} // namespace pluss::codem::native::cpu


namespace pluss::codem::native::cpu {


Series sum_all(const Series& a) {
  if (!a.defined()) throw_error("sum: undefined");
  auto aa = a.contiguous();
  const auto n = aa.numel();

  auto make_scalar = [&](ScalarType st) {
    return Series::empty({}, st);
  };

  switch (aa.scalar_type()) {
    case ScalarType::Float64: {
      const auto* p = reinterpret_cast<const double*>(aa.raw_data());
      double s = 0; for (std::int64_t i=0;i<n;++i) s += p[i];
      auto out = make_scalar(ScalarType::Float64);
      *reinterpret_cast<double*>(out.raw_data()) = s;
      return out;
    }
    case ScalarType::Float32: {
      const auto* p = reinterpret_cast<const float*>(aa.raw_data());
      double s = 0; for (std::int64_t i=0;i<n;++i) s += p[i];
      auto out = make_scalar(ScalarType::Float32);
      *reinterpret_cast<float*>(out.raw_data()) = (float)s;
      return out;
    }
    case ScalarType::Float16: {
      const auto* p = reinterpret_cast<const Float16*>(aa.raw_data());
      double s = 0; for (std::int64_t i=0;i<n;++i) s += float16_to_float(p[i]);
      auto out = make_scalar(ScalarType::Float16);
      *reinterpret_cast<Float16*>(out.raw_data()) = float16_from_float((float)s);
      return out;
    }
    case ScalarType::BFloat16: {
      const auto* p = reinterpret_cast<const BFloat16*>(aa.raw_data());
      double s = 0; for (std::int64_t i=0;i<n;++i) s += bfloat16_to_float(p[i]);
      auto out = make_scalar(ScalarType::BFloat16);
      *reinterpret_cast<BFloat16*>(out.raw_data()) = bfloat16_from_float((float)s);
      return out;
    }
    case ScalarType::Bool: {
      const auto* p = reinterpret_cast<const std::uint8_t*>(aa.raw_data());
      std::uint64_t s = 0; for (std::int64_t i=0;i<n;++i) s += (p[i] != 0);
      auto out = make_scalar(ScalarType::UInt64);
      *reinterpret_cast<std::uint64_t*>(out.raw_data()) = s;
      return out;
    }
    case ScalarType::UInt8: {
      const auto* p = reinterpret_cast<const std::uint8_t*>(aa.raw_data());
      std::uint64_t s=0; for (std::int64_t i=0;i<n;++i) s += p[i];
      auto out = make_scalar(ScalarType::UInt64);
      *reinterpret_cast<std::uint64_t*>(out.raw_data()) = s;
      return out;
    }
    case ScalarType::UInt16: {
      const auto* p = reinterpret_cast<const std::uint16_t*>(aa.raw_data());
      std::uint64_t s=0; for (std::int64_t i=0;i<n;++i) s += p[i];
      auto out = make_scalar(ScalarType::UInt64);
      *reinterpret_cast<std::uint64_t*>(out.raw_data()) = s;
      return out;
    }
    case ScalarType::UInt32: {
      const auto* p = reinterpret_cast<const std::uint32_t*>(aa.raw_data());
      std::uint64_t s=0; for (std::int64_t i=0;i<n;++i) s += p[i];
      auto out = make_scalar(ScalarType::UInt64);
      *reinterpret_cast<std::uint64_t*>(out.raw_data()) = s;
      return out;
    }
    case ScalarType::UInt64: {
      const auto* p = reinterpret_cast<const std::uint64_t*>(aa.raw_data());
      std::uint64_t s=0; for (std::int64_t i=0;i<n;++i) s += p[i];
      auto out = make_scalar(ScalarType::UInt64);
      *reinterpret_cast<std::uint64_t*>(out.raw_data()) = s;
      return out;
    }
    case ScalarType::Int8: {
      const auto* p = reinterpret_cast<const std::int8_t*>(aa.raw_data());
      std::int64_t s=0; for (std::int64_t i=0;i<n;++i) s += p[i];
      auto out = make_scalar(ScalarType::Int64);
      *reinterpret_cast<std::int64_t*>(out.raw_data()) = s;
      return out;
    }
    case ScalarType::Int16: {
      const auto* p = reinterpret_cast<const std::int16_t*>(aa.raw_data());
      std::int64_t s=0; for (std::int64_t i=0;i<n;++i) s += p[i];
      auto out = make_scalar(ScalarType::Int64);
      *reinterpret_cast<std::int64_t*>(out.raw_data()) = s;
      return out;
    }
    case ScalarType::Int32: {
      const auto* p = reinterpret_cast<const std::int32_t*>(aa.raw_data());
      std::int64_t s=0; for (std::int64_t i=0;i<n;++i) s += p[i];
      auto out = make_scalar(ScalarType::Int64);
      *reinterpret_cast<std::int64_t*>(out.raw_data()) = s;
      return out;
    }
    case ScalarType::Int64: {
      const auto* p = reinterpret_cast<const std::int64_t*>(aa.raw_data());
      std::int64_t s=0; for (std::int64_t i=0;i<n;++i) s += p[i];
      auto out = make_scalar(ScalarType::Int64);
      *reinterpret_cast<std::int64_t*>(out.raw_data()) = s;
      return out;
    }
    default:
      break;
  }
  throw_error("sum_all: dtype not implemented");
}


Series all_all(const Series& a) {
  if (!a.defined()) throw_error("all: undefined");
  auto aa = a.contiguous();
  const auto n = aa.numel();
  bool ok = true;

  auto step = [&](bool v) {
    ok = ok && v;
  };

  switch (aa.scalar_type()) {
    case ScalarType::Bool: {
      const auto* p = reinterpret_cast<const std::uint8_t*>(aa.raw_data());
      for (std::int64_t i=0;i<n;++i) step(p[i] != 0);
      break;
    }
    case ScalarType::UInt8: {
      const auto* p = reinterpret_cast<const std::uint8_t*>(aa.raw_data());
      for (std::int64_t i=0;i<n;++i) step(p[i] != 0);
      break;
    }
    case ScalarType::UInt16: {
      const auto* p = reinterpret_cast<const std::uint16_t*>(aa.raw_data());
      for (std::int64_t i=0;i<n;++i) step(p[i] != 0);
      break;
    }
    case ScalarType::UInt32: {
      const auto* p = reinterpret_cast<const std::uint32_t*>(aa.raw_data());
      for (std::int64_t i=0;i<n;++i) step(p[i] != 0);
      break;
    }
    case ScalarType::UInt64: {
      const auto* p = reinterpret_cast<const std::uint64_t*>(aa.raw_data());
      for (std::int64_t i=0;i<n;++i) step(p[i] != 0);
      break;
    }
    case ScalarType::Int8: {
      const auto* p = reinterpret_cast<const std::int8_t*>(aa.raw_data());
      for (std::int64_t i=0;i<n;++i) step(p[i] != 0);
      break;
    }
    case ScalarType::Int16: {
      const auto* p = reinterpret_cast<const std::int16_t*>(aa.raw_data());
      for (std::int64_t i=0;i<n;++i) step(p[i] != 0);
      break;
    }
    case ScalarType::Int32: {
      const auto* p = reinterpret_cast<const std::int32_t*>(aa.raw_data());
      for (std::int64_t i=0;i<n;++i) step(p[i] != 0);
      break;
    }
    case ScalarType::Int64: {
      const auto* p = reinterpret_cast<const std::int64_t*>(aa.raw_data());
      for (std::int64_t i=0;i<n;++i) step(p[i] != 0);
      break;
    }
    case ScalarType::Float32: {
      const auto* p = reinterpret_cast<const float*>(aa.raw_data());
      for (std::int64_t i=0;i<n;++i) step(p[i] != 0.0f);
      break;
    }
    case ScalarType::Float64: {
      const auto* p = reinterpret_cast<const double*>(aa.raw_data());
      for (std::int64_t i=0;i<n;++i) step(p[i] != 0.0);
      break;
    }
    case ScalarType::Float16: {
      const auto* p = reinterpret_cast<const Float16*>(aa.raw_data());
      for (std::int64_t i=0;i<n;++i) step(float16_to_float(p[i]) != 0.0f);
      break;
    }
    case ScalarType::BFloat16: {
      const auto* p = reinterpret_cast<const BFloat16*>(aa.raw_data());
      for (std::int64_t i=0;i<n;++i) step(bfloat16_to_float(p[i]) != 0.0f);
      break;
    }
    default:
      throw_error("all_all: dtype not implemented");
  }

  Series out = Series::empty({}, ScalarType::Bool);
  *reinterpret_cast<std::uint8_t*>(out.raw_data()) = ok ? 1 : 0;
  return out;
}


Series any_all(const Series& a) {
  if (!a.defined()) throw_error("any: undefined");
  auto aa = a.contiguous();
  const auto n = aa.numel();
  bool ok = false;

  auto step = [&](bool v) {
    ok = ok || v;
  };

  switch (aa.scalar_type()) {
    case ScalarType::Bool: {
      const auto* p = reinterpret_cast<const std::uint8_t*>(aa.raw_data());
      for (std::int64_t i=0;i<n;++i) step(p[i] != 0);
      break;
    }
    case ScalarType::UInt8: {
      const auto* p = reinterpret_cast<const std::uint8_t*>(aa.raw_data());
      for (std::int64_t i=0;i<n;++i) step(p[i] != 0);
      break;
    }
    case ScalarType::UInt16: {
      const auto* p = reinterpret_cast<const std::uint16_t*>(aa.raw_data());
      for (std::int64_t i=0;i<n;++i) step(p[i] != 0);
      break;
    }
    case ScalarType::UInt32: {
      const auto* p = reinterpret_cast<const std::uint32_t*>(aa.raw_data());
      for (std::int64_t i=0;i<n;++i) step(p[i] != 0);
      break;
    }
    case ScalarType::UInt64: {
      const auto* p = reinterpret_cast<const std::uint64_t*>(aa.raw_data());
      for (std::int64_t i=0;i<n;++i) step(p[i] != 0);
      break;
    }
    case ScalarType::Int8: {
      const auto* p = reinterpret_cast<const std::int8_t*>(aa.raw_data());
      for (std::int64_t i=0;i<n;++i) step(p[i] != 0);
      break;
    }
    case ScalarType::Int16: {
      const auto* p = reinterpret_cast<const std::int16_t*>(aa.raw_data());
      for (std::int64_t i=0;i<n;++i) step(p[i] != 0);
      break;
    }
    case ScalarType::Int32: {
      const auto* p = reinterpret_cast<const std::int32_t*>(aa.raw_data());
      for (std::int64_t i=0;i<n;++i) step(p[i] != 0);
      break;
    }
    case ScalarType::Int64: {
      const auto* p = reinterpret_cast<const std::int64_t*>(aa.raw_data());
      for (std::int64_t i=0;i<n;++i) step(p[i] != 0);
      break;
    }
    case ScalarType::Float32: {
      const auto* p = reinterpret_cast<const float*>(aa.raw_data());
      for (std::int64_t i=0;i<n;++i) step(p[i] != 0.0f);
      break;
    }
    case ScalarType::Float64: {
      const auto* p = reinterpret_cast<const double*>(aa.raw_data());
      for (std::int64_t i=0;i<n;++i) step(p[i] != 0.0);
      break;
    }
    case ScalarType::Float16: {
      const auto* p = reinterpret_cast<const Float16*>(aa.raw_data());
      for (std::int64_t i=0;i<n;++i) step(float16_to_float(p[i]) != 0.0f);
      break;
    }
    case ScalarType::BFloat16: {
      const auto* p = reinterpret_cast<const BFloat16*>(aa.raw_data());
      for (std::int64_t i=0;i<n;++i) step(bfloat16_to_float(p[i]) != 0.0f);
      break;
    }
    default:
      throw_error("any_all: dtype not implemented");
  }

  Series out = Series::empty({}, ScalarType::Bool);
  *reinterpret_cast<std::uint8_t*>(out.raw_data()) = ok ? 1 : 0;
  return out;
}

} // namespace pluss::codem::native::cpu
