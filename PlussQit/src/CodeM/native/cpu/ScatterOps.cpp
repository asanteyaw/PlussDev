#include "CodeM/native/cpu/ScatterOps.h"
#include "CodeM/core/Errors.h"
#include "CodeM/core/SeriesIterator.h"
#include "CodeM/core/Half.h"
#include "CodeM/core/Complex.h"
#include <cstdint>
#include <limits>
#include <type_traits>

namespace pluss::codem::native::cpu {

static std::int64_t norm_dim(std::int64_t dim, std::int64_t nd) {
  if (dim < 0) dim += nd;
  PLUSS_CHECK(dim >= 0 && dim < nd, "scatter: dim out of range");
  return dim;
}

template <class T>
static inline T identity_(ScatterReduce r) {
  if (r == ScatterReduce::Sum) return T(0);
  if (r == ScatterReduce::AMax) return std::numeric_limits<T>::lowest();
  return std::numeric_limits<T>::max();
}

template <class T>
static inline void apply_reduce_(T& dst, T v, ScatterReduce r) {
  if (r == ScatterReduce::Sum) { dst += v; return; }
  if (r == ScatterReduce::AMax) { if (v > dst) dst = v; return; }
  if (v < dst) dst = v;
}

// For Complex: only Sum is supported here (Torch-like stance).
template <class C>
static inline void apply_reduce_complex_(C& dst, C v, ScatterReduce r) {
  if (r != ScatterReduce::Sum) throw_error("scatter_reduce: amax/amin not supported for complex");
  dst += v;
}

static void validate_and_expand_(const Series& self, std::int64_t dim,
                                 const Series& index_in, const Series& src_in,
                                 Series& index_e, Series& src_e) {
  const auto nd = (std::int64_t)self.sizes().size();
  dim = norm_dim(dim, nd);
  PLUSS_CHECK(index_in.scalar_type() == ScalarType::Int64, "scatter: index must be Int64");

  auto bx = SeriesIterator::broadcast_shape(index_in.sizes(), src_in.sizes());
  auto self_shape = std::vector<std::int64_t>(self.sizes().begin(), self.sizes().end());
  PLUSS_CHECK(bx == self_shape, "scatter: broadcast(index, src) must equal self.shape");

  index_e = index_in.expand(self.sizes()).contiguous();
  src_e   = src_in.expand(self.sizes()).contiguous();
}

template <class T>
static void scatter_add_impl(const Series& self, std::int64_t dim, const Series& index, const Series& src, Series& out) {
  out.copy_(self);
  const auto nd = (std::int64_t)out.sizes().size();
  dim = norm_dim(dim, nd);

  Series idxc, srcc;
  validate_and_expand_(out, dim, index, src, idxc, srcc);

  Series outc = out.is_contiguous() ? out : out.contiguous();

  const auto* ip = reinterpret_cast<const std::int64_t*>(idxc.raw_data());
  const auto* sp = reinterpret_cast<const T*>(srcc.raw_data());
  auto* op = reinterpret_cast<T*>(outc.raw_data());

  const auto sz = outc.sizes();
  const std::int64_t dim_size = sz[(size_t)dim];

  std::int64_t inner = 1;
  for (std::int64_t d=dim+1; d<nd; ++d) inner *= sz[(size_t)d];
  std::int64_t outer = 1;
  for (std::int64_t d=0; d<dim; ++d) outer *= sz[(size_t)d];

  for (std::int64_t o=0; o<outer; ++o) {
    for (std::int64_t i=0; i<inner; ++i) {
      const std::int64_t base = o*dim_size*inner + i;
      for (std::int64_t r=0; r<dim_size; ++r) {
        const std::int64_t lin = base + r*inner;
        const std::int64_t dst_r = ip[lin];
        PLUSS_CHECK(dst_r >= 0 && dst_r < dim_size, "scatter_add: index out of bounds");
        const std::int64_t dst = base + dst_r*inner;
        op[dst] += sp[lin];
      }
    }
  }

  if (!out.is_contiguous()) out.copy_(outc);
}

template <class T>
static void init_out_(const Series& self, Series& out, ScatterReduce reduce,
                      bool include_self, bool has_initial, double initial_scalar) {
  if (include_self) {
    out.copy_(self);
    return;
  }

  const auto n = out.numel();
  T init = has_initial ? (T)initial_scalar : identity_<T>(reduce);

  Series outc = out.is_contiguous() ? out : out.contiguous();
  auto* op = reinterpret_cast<T*>(outc.raw_data());
  for (std::int64_t i=0;i<n;++i) op[i] = init;
  if (!out.is_contiguous()) out.copy_(outc);
}

static void init_out_half_(const Series& self, Series& out, ScatterReduce reduce,
                           bool include_self, bool has_initial, double initial_scalar) {
  if (include_self) { out.copy_(self); return; }
  const auto n = out.numel();
  float init = has_initial ? (float)initial_scalar : (reduce==ScatterReduce::Sum ? 0.0f :
                                                      (reduce==ScatterReduce::AMax ? -std::numeric_limits<float>::infinity()
                                                                                  :  std::numeric_limits<float>::infinity()));
  Series outc = out.is_contiguous() ? out : out.contiguous();
  if (out.scalar_type() == ScalarType::Float16) {
    auto* op = reinterpret_cast<Float16*>(outc.raw_data());
    for (std::int64_t i=0;i<n;++i) op[i] = float16_from_float(init);
  } else {
    auto* op = reinterpret_cast<BFloat16*>(outc.raw_data());
    for (std::int64_t i=0;i<n;++i) op[i] = bfloat16_from_float(init);
  }
  if (!out.is_contiguous()) out.copy_(outc);
}

template <class T>
static void scatter_reduce_impl(const Series& self, std::int64_t dim, const Series& index, const Series& src,
                                ScatterReduce reduce, Series& out,
                                bool include_self, bool has_initial, double initial_scalar) {
  const auto nd = (std::int64_t)self.sizes().size();
  dim = norm_dim(dim, nd);

  Series idxc, srcc;
  validate_and_expand_(self, dim, index, src, idxc, srcc);

  init_out_<T>(self, out, reduce, include_self, has_initial, initial_scalar);
  Series outc = out.is_contiguous() ? out : out.contiguous();

  const auto* ip = reinterpret_cast<const std::int64_t*>(idxc.raw_data());
  const auto* sp = reinterpret_cast<const T*>(srcc.raw_data());
  auto* op = reinterpret_cast<T*>(outc.raw_data());

  const auto sz = outc.sizes();
  const std::int64_t dim_size = sz[(size_t)dim];

  std::int64_t inner = 1;
  for (std::int64_t d=dim+1; d<nd; ++d) inner *= sz[(size_t)d];
  std::int64_t outer = 1;
  for (std::int64_t d=0; d<dim; ++d) outer *= sz[(size_t)d];

  for (std::int64_t o=0; o<outer; ++o) {
    for (std::int64_t i=0; i<inner; ++i) {
      const std::int64_t base = o*dim_size*inner + i;
      for (std::int64_t r=0; r<dim_size; ++r) {
        const std::int64_t lin = base + r*inner;
        const std::int64_t dst_r = ip[lin];
        PLUSS_CHECK(dst_r >= 0 && dst_r < dim_size, "scatter_reduce: index out of bounds");
        const std::int64_t dst = base + dst_r*inner;
        apply_reduce_(op[dst], sp[lin], reduce);
      }
    }
  }

  if (!out.is_contiguous()) out.copy_(outc);
}

template <class C>
static void scatter_reduce_complex_impl(const Series& self, std::int64_t dim, const Series& index, const Series& src,
                                        ScatterReduce reduce, Series& out,
                                        bool include_self, bool has_initial, double initial_scalar) {
  const auto nd = (std::int64_t)self.sizes().size();
  dim = norm_dim(dim, nd);

  Series idxc, srcc;
  validate_and_expand_(self, dim, index, src, idxc, srcc);

  if (include_self) out.copy_(self);
  else {
    // initial for complex: real=initial_scalar imag=0, else identity=0
    const auto n = out.numel();
    const double init = has_initial ? initial_scalar : 0.0;
    Series outc = out.is_contiguous() ? out : out.contiguous();
    auto* op = reinterpret_cast<C*>(outc.raw_data());
    for (std::int64_t i=0;i<n;++i) op[i] = C((decltype(op[i].real()))init, (decltype(op[i].imag()))0);
    if (!out.is_contiguous()) out.copy_(outc);
  }

  Series outc = out.is_contiguous() ? out : out.contiguous();
  const auto* ip = reinterpret_cast<const std::int64_t*>(idxc.raw_data());
  const auto* sp = reinterpret_cast<const C*>(srcc.raw_data());
  auto* op = reinterpret_cast<C*>(outc.raw_data());

  const auto sz = outc.sizes();
  const std::int64_t dim_size = sz[(size_t)dim];

  std::int64_t inner = 1;
  for (std::int64_t d=dim+1; d<nd; ++d) inner *= sz[(size_t)d];
  std::int64_t outer = 1;
  for (std::int64_t d=0; d<dim; ++d) outer *= sz[(size_t)d];

  for (std::int64_t o=0; o<outer; ++o) {
    for (std::int64_t i=0; i<inner; ++i) {
      const std::int64_t base = o*dim_size*inner + i;
      for (std::int64_t r=0; r<dim_size; ++r) {
        const std::int64_t lin = base + r*inner;
        const std::int64_t dst_r = ip[lin];
        PLUSS_CHECK(dst_r >= 0 && dst_r < dim_size, "scatter_reduce: index out of bounds");
        const std::int64_t dst = base + dst_r*inner;
        apply_reduce_complex_(op[dst], sp[lin], reduce);
      }
    }
  }

  if (!out.is_contiguous()) out.copy_(outc);
}

static void scatter_reduce_half_impl(const Series& self, std::int64_t dim, const Series& index, const Series& src,
                                     ScatterReduce reduce, Series& out,
                                     bool include_self, bool has_initial, double initial_scalar) {
  const auto nd = (std::int64_t)self.sizes().size();
  dim = norm_dim(dim, nd);

  Series idxc, srcc;
  validate_and_expand_(self, dim, index, src, idxc, srcc);

  init_out_half_(self, out, reduce, include_self, has_initial, initial_scalar);
  Series outc = out.is_contiguous() ? out : out.contiguous();

  const auto* ip = reinterpret_cast<const std::int64_t*>(idxc.raw_data());

  const auto sz = outc.sizes();
  const std::int64_t dim_size = sz[(size_t)dim];

  std::int64_t inner = 1;
  for (std::int64_t d=dim+1; d<nd; ++d) inner *= sz[(size_t)d];
  std::int64_t outer = 1;
  for (std::int64_t d=0; d<dim; ++d) outer *= sz[(size_t)d];

  if (self.scalar_type() == ScalarType::Float16) {
    const auto* sp = reinterpret_cast<const Float16*>(srcc.raw_data());
    auto* op = reinterpret_cast<Float16*>(outc.raw_data());
    for (std::int64_t o=0; o<outer; ++o) {
      for (std::int64_t i=0; i<inner; ++i) {
        const std::int64_t base = o*dim_size*inner + i;
        for (std::int64_t r=0; r<dim_size; ++r) {
          const std::int64_t lin = base + r*inner;
          const std::int64_t dst_r = ip[lin];
          PLUSS_CHECK(dst_r >= 0 && dst_r < dim_size, "scatter_reduce: index out of bounds");
          const std::int64_t dst = base + dst_r*inner;
          float dstv = float16_to_float(op[dst]);
          float srcv = float16_to_float(sp[lin]);
          if (reduce == ScatterReduce::Sum) dstv += srcv;
          else if (reduce == ScatterReduce::AMax) dstv = (srcv > dstv ? srcv : dstv);
          else dstv = (srcv < dstv ? srcv : dstv);
          op[dst] = float16_from_float(dstv);
        }
      }
    }
  } else {
    const auto* sp = reinterpret_cast<const BFloat16*>(srcc.raw_data());
    auto* op = reinterpret_cast<BFloat16*>(outc.raw_data());
    for (std::int64_t o=0; o<outer; ++o) {
      for (std::int64_t i=0; i<inner; ++i) {
        const std::int64_t base = o*dim_size*inner + i;
        for (std::int64_t r=0; r<dim_size; ++r) {
          const std::int64_t lin = base + r*inner;
          const std::int64_t dst_r = ip[lin];
          PLUSS_CHECK(dst_r >= 0 && dst_r < dim_size, "scatter_reduce: index out of bounds");
          const std::int64_t dst = base + dst_r*inner;
          float dstv = bfloat16_to_float(op[dst]);
          float srcv = bfloat16_to_float(sp[lin]);
          if (reduce == ScatterReduce::Sum) dstv += srcv;
          else if (reduce == ScatterReduce::AMax) dstv = (srcv > dstv ? srcv : dstv);
          else dstv = (srcv < dstv ? srcv : dstv);
          op[dst] = bfloat16_from_float(dstv);
        }
      }
    }
  }

  if (!out.is_contiguous()) out.copy_(outc);
}

void scatter_add(const Series& self, std::int64_t dim, const Series& index, const Series& src, Series& out) {
  PLUSS_CHECK(self.scalar_type() == out.scalar_type(), "scatter_add: dtype mismatch");
  switch (self.scalar_type()) {
    case ScalarType::Float64: scatter_add_impl<double>(self, dim, index, src, out); return;
    case ScalarType::Float32: scatter_add_impl<float>(self, dim, index, src, out); return;
    case ScalarType::Int64: scatter_add_impl<std::int64_t>(self, dim, index, src, out); return;
    case ScalarType::Int32: scatter_add_impl<std::int32_t>(self, dim, index, src, out); return;
    case ScalarType::Int16: scatter_add_impl<std::int16_t>(self, dim, index, src, out); return;
    case ScalarType::Int8: scatter_add_impl<std::int8_t>(self, dim, index, src, out); return;
    case ScalarType::UInt64: scatter_add_impl<std::uint64_t>(self, dim, index, src, out); return;
    case ScalarType::UInt32: scatter_add_impl<std::uint32_t>(self, dim, index, src, out); return;
    case ScalarType::UInt16: scatter_add_impl<std::uint16_t>(self, dim, index, src, out); return;
    case ScalarType::UInt8: scatter_add_impl<std::uint8_t>(self, dim, index, src, out); return;
    case ScalarType::Complex64: scatter_add_impl<Complex64>(self, dim, index, src, out); return;
    case ScalarType::Complex128: scatter_add_impl<Complex128>(self, dim, index, src, out); return;
    default: break;
  }
  throw_error("scatter_add: dtype not implemented");
}

void scatter_reduce(const Series& self, std::int64_t dim, const Series& index, const Series& src,
                    ScatterReduce reduce, Series& out,
                    bool include_self, bool has_initial, double initial_scalar) {
  PLUSS_CHECK(self.scalar_type() == out.scalar_type(), "scatter_reduce: dtype mismatch");
  switch (self.scalar_type()) {
    case ScalarType::Float64: scatter_reduce_impl<double>(self, dim, index, src, reduce, out, include_self, has_initial, initial_scalar); return;
    case ScalarType::Float32: scatter_reduce_impl<float>(self, dim, index, src, reduce, out, include_self, has_initial, initial_scalar); return;
    case ScalarType::Float16:
    case ScalarType::BFloat16: scatter_reduce_half_impl(self, dim, index, src, reduce, out, include_self, has_initial, initial_scalar); return;
    case ScalarType::Int64: scatter_reduce_impl<std::int64_t>(self, dim, index, src, reduce, out, include_self, has_initial, initial_scalar); return;
    case ScalarType::Int32: scatter_reduce_impl<std::int32_t>(self, dim, index, src, reduce, out, include_self, has_initial, initial_scalar); return;
    case ScalarType::Int16: scatter_reduce_impl<std::int16_t>(self, dim, index, src, reduce, out, include_self, has_initial, initial_scalar); return;
    case ScalarType::Int8: scatter_reduce_impl<std::int8_t>(self, dim, index, src, reduce, out, include_self, has_initial, initial_scalar); return;
    case ScalarType::UInt64: scatter_reduce_impl<std::uint64_t>(self, dim, index, src, reduce, out, include_self, has_initial, initial_scalar); return;
    case ScalarType::UInt32: scatter_reduce_impl<std::uint32_t>(self, dim, index, src, reduce, out, include_self, has_initial, initial_scalar); return;
    case ScalarType::UInt16: scatter_reduce_impl<std::uint16_t>(self, dim, index, src, reduce, out, include_self, has_initial, initial_scalar); return;
    case ScalarType::UInt8: scatter_reduce_impl<std::uint8_t>(self, dim, index, src, reduce, out, include_self, has_initial, initial_scalar); return;
    case ScalarType::Complex64: scatter_reduce_complex_impl<Complex64>(self, dim, index, src, reduce, out, include_self, has_initial, initial_scalar); return;
    case ScalarType::Complex128: scatter_reduce_complex_impl<Complex128>(self, dim, index, src, reduce, out, include_self, has_initial, initial_scalar); return;
    default: break;
  }
  throw_error("scatter_reduce: dtype not implemented");
}

} // namespace pluss::codem::native::cpu
