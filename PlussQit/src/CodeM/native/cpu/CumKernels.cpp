#include "CodeM/native/cpu/CumKernels.h"
#include "CodeM/core/Errors.h"
#include "CodeM/core/Half.h"
#include <vector>

namespace pluss::codem::native::cpu {

static std::int64_t norm_dim(std::int64_t dim, std::int64_t nd) {
  if (dim < 0) dim += nd;
  PLUSS_CHECK(dim >= 0 && dim < nd, "cum: dim out of range");
  return dim;
}

static void outer_reduce_inner(const Series& a, std::int64_t dim, std::int64_t& outer, std::int64_t& reduce, std::int64_t& inner) {
  auto sz = a.sizes();
  const std::int64_t nd = (std::int64_t)sz.size();
  dim = norm_dim(dim, nd);
  outer = 1; for (std::int64_t i=0;i<dim;++i) outer *= sz[(size_t)i];
  reduce = sz[(size_t)dim];
  inner = 1; for (std::int64_t i=dim+1;i<nd;++i) inner *= sz[(size_t)i];
}

template <class T, class Op>
static void cum_impl(const Series& a, std::int64_t dim, Series& out, Op&& op, T init) {
  PLUSS_CHECK(a.is_contiguous() && out.is_contiguous(), "cum: requires contiguous");
  std::int64_t outer, reduce, inner;
  outer_reduce_inner(a, dim, outer, reduce, inner);
  const T* ap = reinterpret_cast<const T*>(a.raw_data());
  T* optr = reinterpret_cast<T*>(out.raw_data());
  for (std::int64_t o=0; o<outer; ++o) {
    for (std::int64_t i=0; i<inner; ++i) {
      T acc = init;
      const std::int64_t base = o*reduce*inner + i;
      for (std::int64_t r=0; r<reduce; ++r) {
        const std::int64_t idx = base + r*inner;
        acc = op(acc, ap[idx], r);
        optr[idx] = acc;
      }
    }
  }
}

void cumsum_dim(const Series& a, std::int64_t dim, Series& out) {
  PLUSS_CHECK(a.scalar_type()==out.scalar_type(), "cumsum: dtype mismatch");
  switch (a.scalar_type()) {
    case ScalarType::Float64:
      cum_impl<double>(a, dim, out, [](double acc, double x, std::int64_t r){ return (r==0)? x : (acc+x); }, 0.0);
      return;
    case ScalarType::Float32:
      cum_impl<float>(a, dim, out, [](float acc, float x, std::int64_t r){ return (r==0)? x : (acc+x); }, 0.0f);
      return;
    case ScalarType::Int64:
      cum_impl<std::int64_t>(a, dim, out, [](std::int64_t acc, std::int64_t x, std::int64_t r){ return (r==0)? x : (acc+x); }, 0);
      return;
    case ScalarType::Int32:
      cum_impl<std::int32_t>(a, dim, out, [](std::int32_t acc, std::int32_t x, std::int64_t r){ return (r==0)? x : static_cast<std::int32_t>(acc+x); }, 0);
      return;
    default: break;
  }
  throw_error("cumsum: dtype not implemented");
}

void cumprod_dim(const Series& a, std::int64_t dim, Series& out) {
  PLUSS_CHECK(a.scalar_type()==out.scalar_type(), "cumprod: dtype mismatch");
  switch (a.scalar_type()) {
    case ScalarType::Float64:
      cum_impl<double>(a, dim, out, [](double acc, double x, std::int64_t r){ return (r==0)? x : (acc*x); }, 1.0);
      return;
    case ScalarType::Float32:
      cum_impl<float>(a, dim, out, [](float acc, float x, std::int64_t r){ return (r==0)? x : (acc*x); }, 1.0f);
      return;
    case ScalarType::Int64:
      cum_impl<std::int64_t>(a, dim, out, [](std::int64_t acc, std::int64_t x, std::int64_t r){ return (r==0)? x : (acc*x); }, 1);
      return;
    case ScalarType::Int32:
      cum_impl<std::int32_t>(a, dim, out, [](std::int32_t acc, std::int32_t x, std::int64_t r){ return (r==0)? x : static_cast<std::int32_t>(acc*x); }, 1);
      return;
    default: break;
  }
  throw_error("cumprod: dtype not implemented");
}

} // namespace
