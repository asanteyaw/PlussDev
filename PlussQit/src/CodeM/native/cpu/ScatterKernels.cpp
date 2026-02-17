#include "CodeM/native/cpu/ScatterKernels.h"
#include "CodeM/core/Errors.h"
#include "CodeM/core/ScalarType.h"
#include "CodeM/core/SeriesIterator.h"
#include "CodeM/core/TypePromotion.h"
#include "CodeM/core/Half.h"

#include <vector>
#include <algorithm>
#include <limits>
#include <cstring>

namespace pluss::codem::native::cpu {

struct ScatterShape {
  std::int64_t dim;
  std::int64_t outer;
  std::int64_t inner;
  std::int64_t in_dim;
  std::int64_t scat_dim; // index/src size at dim
};

static ScatterShape compute_shape(const Series& a, std::int64_t dim, const Series& index, const char* name) {
  const auto nd = (std::int64_t)a.dim();
  std::int64_t d = dim;
  if (d < 0) d += nd;
  PLUSS_CHECK(d >= 0 && d < nd, std::string(name) + ": bad dim");
  PLUSS_CHECK(index.dim() == nd, std::string(name) + ": index must have same dim as input");
  const std::int64_t in_dim = a.sizes()[(std::size_t)d];
  const std::int64_t scat_dim = index.sizes()[(std::size_t)d];

  // For dims != dim: index sizes must match input sizes. For dim: can differ (Torch-ish).
  for (std::int64_t k=0;k<nd;++k) {
    if (k == d) continue;
    PLUSS_CHECK(index.sizes()[(std::size_t)k] == a.sizes()[(std::size_t)k],
                std::string(name) + ": index size mismatch (non-scatter dims must match input)");
  }

  std::int64_t outer = 1;
  for (std::int64_t i=0;i<d;++i) outer *= a.sizes()[(std::size_t)i];
  std::int64_t inner = 1;
  for (std::int64_t i=d+1;i<nd;++i) inner *= a.sizes()[(std::size_t)i];

  return ScatterShape{d, outer, inner, in_dim, scat_dim};
}

static void check_scatter_args(const Series& a, std::int64_t& dim, const Series& index, const Series& src, const char* name) {
  PLUSS_CHECK(a.defined() && index.defined() && src.defined(), std::string(name) + ": undefined input");
  PLUSS_CHECK(index.scalar_type() == ScalarType::Int64, std::string(name) + ": index must be int64");
  auto sh = compute_shape(a, dim, index, name);

  // src must be broadcastable to index shape (Torch-ish).
  PLUSS_CHECK(src.dim() == index.dim(), std::string(name) + ": src must have same dim as index");
  const auto nd = (std::int64_t)src.dim();
  for (std::int64_t k=0;k<nd;++k) {
    const auto ss = src.sizes()[(std::size_t)k];
    const auto is = index.sizes()[(std::size_t)k];
    PLUSS_CHECK(ss == is || ss == 1, std::string(name) + ": src must be broadcastable to index shape");
  }

  dim = sh.dim;
}

static Series broadcast_src_contig(const Series& src, const Series& index_like) {
  // Make a contiguous src expanded to index_like shape (very explicit/safe).
  auto s = src.contiguous();
  auto ish = index_like.sizes();
  Series out = Series::empty(ish, s.scalar_type());
  PLUSS_CHECK(out.is_contiguous(), "broadcast_src_contig: internal");
  PLUSS_CHECK(index_like.is_contiguous(), "broadcast_src_contig: expects contiguous index");
  // Iterate over index_like linear index and map to src index with broadcasting.
  const auto nd = (std::int64_t)ish.size();
  std::vector<std::int64_t> strides_src(s.strides().begin(), s.strides().end());
  std::vector<std::int64_t> sizes_src(s.sizes().begin(), s.sizes().end());

  // Build per-dim stride for broadcast: if size==1 -> stride=0
  std::vector<std::int64_t> bstride((std::size_t)nd);
  for (std::int64_t k=0;k<nd;++k) {
    bstride[(std::size_t)k] = (sizes_src[(std::size_t)k] == 1) ? 0 : strides_src[(std::size_t)k];
  }

  // linear to multi-index
  const auto numel = index_like.numel();
  const auto* sp = reinterpret_cast<const std::byte*>(s.raw_data());
  auto* op = reinterpret_cast<std::byte*>(out.raw_data());
  const auto item = s.dtype().itemsize;

  std::vector<std::int64_t> idx((std::size_t)nd, 0);
  for (std::int64_t lin=0; lin<numel; ++lin) {
    // compute src offset via idx and bstride
    std::int64_t off = 0;
    for (std::int64_t k=0;k<nd;++k) off += idx[(std::size_t)k] * bstride[(std::size_t)k];
    std::memcpy(op + lin*item, sp + off*item, (std::size_t)item);

    // increment idx (row-major)
    for (std::int64_t k=nd-1; k>=0; --k) {
      idx[(std::size_t)k] += 1;
      if (idx[(std::size_t)k] < ish[(std::size_t)k]) break;
      idx[(std::size_t)k] = 0;
    }
  }

  return out;
}

template <class T>
static inline T init_for_reduce(const std::string& reduce) {
  if (reduce == "sum") return (T)0;
  if (reduce == "amax") return std::numeric_limits<T>::lowest();
  if (reduce == "amin") return std::numeric_limits<T>::max();
  throw_error("scatter_reduce: bad reduce");
}

template <class T>
static inline T apply_reduce(T cur, T val, const std::string& reduce) {
  if (reduce == "sum") return cur + val;
  if (reduce == "amax") return std::max(cur, val);
  if (reduce == "amin") return std::min(cur, val);
  throw_error("scatter_reduce: bad reduce");
}

template <class T>
static void scatter_reduce_impl_contig(Series& a, std::int64_t dim, const Series& index, const Series& src, const std::string& reduce, bool include_self, bool has_initial, double initial) {
  PLUSS_CHECK(a.is_contiguous() && index.is_contiguous() && src.is_contiguous(), "scatter_reduce: requires contiguous");
  auto sh = compute_shape(a, dim, index, "scatter_reduce");

  // Initialization policy (Torch-ish):
// - if has_initial: base is filled with `initial`
//   - if include_self: fold original self values into base using reduce
// - else (no initial):
//   - if include_self: keep existing self values
//   - else: fill identity for reduce
  if (has_initial) {
    Series self_backup = Series::undefined();
    if (include_self) self_backup = a.clone();
    T* ap0 = reinterpret_cast<T*>(const_cast<std::byte*>(a.raw_data()));
    const T init = (T)initial;
    for (std::int64_t i=0;i<a.numel();++i) ap0[i] = init;
    if (include_self) {
      const T* bp = reinterpret_cast<const T*>(self_backup.raw_data());
      for (std::int64_t i=0;i<a.numel();++i) ap0[i] = apply_reduce<T>(ap0[i], bp[i], reduce);
    }
  } else {
    if (!include_self) {
      T* ap0 = reinterpret_cast<T*>(const_cast<std::byte*>(a.raw_data()));
      const T init = init_for_reduce<T>(reduce);
      for (std::int64_t i=0;i<a.numel();++i) ap0[i] = init;
    }
  }

  T* ap = reinterpret_cast<T*>(const_cast<std::byte*>(a.raw_data()));
  const T* sp = reinterpret_cast<const T*>(src.raw_data());
  const std::int64_t* ip = reinterpret_cast<const std::int64_t*>(index.raw_data());

  for (std::int64_t o=0; o<sh.outer; ++o) {
    for (std::int64_t d=0; d<sh.scat_dim; ++d) {
      for (std::int64_t in=0; in<sh.inner; ++in) {
        const std::int64_t idx = (o*sh.scat_dim*sh.inner + d*sh.inner + in);
        std::int64_t dst = ip[idx];
        if (dst < 0) dst += sh.in_dim;
        PLUSS_CHECK(dst >= 0 && dst < sh.in_dim, "scatter_reduce: index out of range");
        const std::int64_t out_idx = (o*sh.in_dim*sh.inner + dst*sh.inner + in);
        ap[out_idx] = apply_reduce<T>(ap[out_idx], sp[idx], reduce);
      }
    }
  }
}

// Half/bfloat16 compute in float
template <class HalfT, class LoadF, class StoreF>
static void scatter_reduce_half_contig(Series& a, std::int64_t dim, const Series& index, const Series& src,
                                       const std::string& reduce, bool include_self, bool has_initial, double initial, LoadF&& load, StoreF&& store) {
  PLUSS_CHECK(a.is_contiguous() && index.is_contiguous() && src.is_contiguous(), "scatter_reduce: requires contiguous");
  auto sh = compute_shape(a, dim, index, "scatter_reduce");

  if (!include_self) {
    auto* ap0 = reinterpret_cast<HalfT*>(const_cast<std::byte*>(a.raw_data()));
    float init;
    if (reduce == "sum") init = 0.0f;
    else if (reduce == "amax") init = -std::numeric_limits<float>::infinity();
    else if (reduce == "amin") init = +std::numeric_limits<float>::infinity();
    else throw_error("scatter_reduce: bad reduce");
    for (std::int64_t i=0;i<a.numel();++i) ap0[i] = store(init);
  }

  auto* ap = reinterpret_cast<HalfT*>(const_cast<std::byte*>(a.raw_data()));
  const auto* sp = reinterpret_cast<const HalfT*>(src.raw_data());
  const std::int64_t* ip = reinterpret_cast<const std::int64_t*>(index.raw_data());

  auto apply = [&](float cur, float val)->float{
    if (reduce == "sum") return cur + val;
    if (reduce == "amax") return std::max(cur, val);
    if (reduce == "amin") return std::min(cur, val);
    throw_error("scatter_reduce: bad reduce");
  };

  for (std::int64_t o=0; o<sh.outer; ++o) {
    for (std::int64_t d=0; d<sh.scat_dim; ++d) {
      for (std::int64_t in=0; in<sh.inner; ++in) {
        const std::int64_t idx = (o*sh.scat_dim*sh.inner + d*sh.inner + in);
        std::int64_t dst = ip[idx];
        if (dst < 0) dst += sh.in_dim;
        PLUSS_CHECK(dst >= 0 && dst < sh.in_dim, "scatter_reduce: index out of range");
        const std::int64_t out_idx = (o*sh.in_dim*sh.inner + dst*sh.inner + in);
        float cur = load(ap[out_idx]);
        float val = load(sp[idx]);
        ap[out_idx] = store(apply(cur, val));
      }
    }
  }
}

Series scatter_reduce(const Series& a, std::int64_t dim, const Series& index, const Series& src,
                      const std::string& reduce, bool include_self) {
  std::int64_t d = dim;
  check_scatter_args(a, d, index, src, "scatter_reduce");
  const auto out_dt = pluss::codem::promote_types(a.scalar_type(), src.scalar_type());

  // out must have input shape
  auto out = a.to(out_dt).contiguous();
  auto idx = index.contiguous();

  // broadcast src to index shape first, then dtype cast
  auto src_bc = broadcast_src_contig(src, idx);
  auto ss = src_bc.to(out_dt).contiguous();

  scatter_reduce_inplace(out, d, idx, ss, reduce, include_self);
  return out;
}


Series scatter_reduce(const Series& a, std::int64_t dim, const Series& index, const Series& src,
                      const std::string& reduce, bool include_self, bool has_initial, double initial) {
  std::int64_t d = dim;
  check_scatter_args(a, d, index, src, "scatter_reduce");
  const auto out_dt = pluss::codem::promote_types(a.scalar_type(), src.scalar_type());

  auto out = a.to(out_dt).contiguous();
  auto idx = index.contiguous();
  auto src_bc = broadcast_src_contig(src, idx);
  auto ss = src_bc.to(out_dt).contiguous();

  scatter_reduce_inplace(out, d, idx, ss, reduce, include_self, has_initial, initial);
  return out;
}




void scatter_reduce_inplace(Series& a, std::int64_t dim, const Series& index, const Series& src,
                            const std::string& reduce, bool include_self) {
  scatter_reduce_inplace(a, dim, index, src, reduce, include_self, false, 0.0);
}

void scatter_reduce_inplace(Series& a, std::int64_t dim, const Series& index, const Series& src,
                            const std::string& reduce, bool include_self, bool has_initial, double initial) {
  std::int64_t d = dim;
  check_scatter_args(a, d, index, src, "scatter_reduce_");
  PLUSS_CHECK(a.scalar_type() == src.scalar_type(), "scatter_reduce_: dtype mismatch");
  // safety policy: contiguous-only until full alias/overlap rules are generalized
  PLUSS_CHECK(a.is_contiguous() && index.is_contiguous() && src.is_contiguous(), "scatter_reduce_: requires contiguous");

  switch (a.scalar_type()) {
    case ScalarType::Float32: scatter_reduce_impl_contig<float>(a,d,index,src,reduce,include_self,has_initial,initial); return;
    case ScalarType::Float64: scatter_reduce_impl_contig<double>(a,d,index,src,reduce,include_self,has_initial,initial); return;
    case ScalarType::Int64: scatter_reduce_impl_contig<std::int64_t>(a,d,index,src,reduce,include_self,has_initial,initial); return;
    case ScalarType::Int32: scatter_reduce_impl_contig<std::int32_t>(a,d,index,src,reduce,include_self,has_initial,initial); return;
    case ScalarType::Int16: scatter_reduce_impl_contig<std::int16_t>(a,d,index,src,reduce,include_self,has_initial,initial); return;
    case ScalarType::Int8: scatter_reduce_impl_contig<std::int8_t>(a,d,index,src,reduce,include_self,has_initial,initial); return;
    case ScalarType::Float16:
      scatter_reduce_half_contig<Float16>(a,d,index,src,reduce,include_self,has_initial,initial,
        [](Float16 h){ return float16_to_float(h); },
        [](float f){ return float16_from_float(f); });
      return;
    case ScalarType::BFloat16:
      scatter_reduce_half_contig<BFloat16>(a,d,index,src,reduce,include_self,has_initial,initial,
        [](BFloat16 h){ return bfloat16_to_float(h); },
        [](float f){ return bfloat16_from_float(f); });
      return;
    default: break;
  }
  throw_error("scatter_reduce_: dtype not implemented");
}

Series scatter_add(const Series& a, std::int64_t dim, const Series& index, const Series& src) {
  return scatter_reduce(a, dim, index, src, "sum", true);
}

void scatter_add_inplace(Series& a, std::int64_t dim, const Series& index, const Series& src) {
  scatter_reduce_inplace(a, dim, index, src, "sum", true);
}

} // namespace pluss::codem::native::cpu
