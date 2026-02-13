#include "CodeM/native/cpu/ReduceKernels.h"
#include "CodeM/core/Errors.h"
#include "CodeM/core/Half.h"
#include "CodeM/core/Complex.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace pluss::codem::native::cpu {

static std::int64_t norm_dim(std::int64_t dim, std::int64_t nd) {
  if (dim < 0) dim += nd;
  PLUSS_CHECK(dim >= 0 && dim < nd, "reduce: dim out of range");
  return dim;
}

static void shape_for_reduce(const Series& a, std::int64_t dim, std::vector<std::int64_t>& out_sizes,
                             std::int64_t& outer, std::int64_t& reduce, std::int64_t& inner) {
  const auto sz = a.sizes();
  const std::int64_t nd = (std::int64_t)sz.size();
  dim = norm_dim(dim, nd);
  out_sizes.clear();
  for (std::int64_t i=0;i<nd;++i) if (i!=dim) out_sizes.push_back(sz[(size_t)i]);
  outer = 1; for (std::int64_t i=0;i<dim;++i) outer *= sz[(size_t)i];
  reduce = sz[(size_t)dim];
  inner = 1; for (std::int64_t i=dim+1;i<nd;++i) inner *= sz[(size_t)i];
}

template <class AccT, class InT, class OutT, class Load, class Store, class Init, class Step>
static void reduce_dim_impl(const Series& a, std::int64_t dim, Series& out, Load load, Store store, Init init, Step step) {
  std::vector<std::int64_t> out_sz;
  std::int64_t outer, reduce, inner;
  shape_for_reduce(a, dim, out_sz, outer, reduce, inner);

  const InT* ap = reinterpret_cast<const InT*>(a.raw_data());
  OutT* op = reinterpret_cast<OutT*>(out.raw_data());

  const std::int64_t out_elems = outer * inner;
  for (std::int64_t oi=0; oi<out_elems; ++oi) {
    AccT acc = init();
    const std::int64_t o = oi / inner;
    const std::int64_t i = oi - o*inner;
    const std::int64_t base = (o*reduce*inner) + i;
    for (std::int64_t r=0; r<reduce; ++r) {
      const std::int64_t idx = base + r*inner;
      acc = step(acc, load(ap[idx]));
    }
    op[oi] = store(acc, reduce);
  }
}

static float load_f16(Float16 x){ return float16_to_float(x); }
static float load_bf16(BFloat16 x){ return bfloat16_to_float(x); }
static Float16 store_f16(float x){ return float16_from_float(x); }
static BFloat16 store_bf16(float x){ return bfloat16_from_float(x); }

void sum_dim(const Series& a, std::int64_t dim, Series& out) {
  PLUSS_CHECK(a.is_contiguous(), "sum_dim: requires contiguous");
  switch (a.scalar_type()) {
    case ScalarType::Float64:
      reduce_dim_impl<double,double,double>(a,dim,out,[](double x){return x;},[](double acc, std::int64_t){return acc;},[](){return 0.0;},[](double acc,double x){return acc+x;});
      return;
    case ScalarType::Float32:
      reduce_dim_impl<float,float,float>(a,dim,out,[](float x){return x;},[](float acc, std::int64_t){return acc;},[](){return 0.0f;},[](float acc,float x){return acc+x;});
      return;
    case ScalarType::Float16:
      reduce_dim_impl<float,Float16,Float16>(a,dim,out,load_f16,[](float acc, std::int64_t){return store_f16(acc);},[](){return 0.0f;},[](float acc,float x){return acc+x;});
      return;
    case ScalarType::BFloat16:
      reduce_dim_impl<float,BFloat16,BFloat16>(a,dim,out,load_bf16,[](float acc, std::int64_t){return store_bf16(acc);},[](){return 0.0f;},[](float acc,float x){return acc+x;});
      return;
    case ScalarType::Int64:
      reduce_dim_impl<std::int64_t,std::int64_t,std::int64_t>(a,dim,out,[](std::int64_t x){return x;},[](std::int64_t acc, std::int64_t){return acc;},[](){return (std::int64_t)0;},[](std::int64_t acc,std::int64_t x){return acc+x;});
      return;

    case ScalarType::Int32:
      reduce_dim_impl<std::int64_t,std::int32_t,std::int32_t>(a,dim,out,[](std::int32_t x){return (std::int64_t)x;},
        [](std::int64_t acc, std::int64_t){return (std::int32_t)acc;},[](){return (std::int64_t)0;},[](std::int64_t acc,std::int64_t x){return acc+x;});
      return;
    case ScalarType::Int16:
      reduce_dim_impl<std::int64_t,std::int16_t,std::int16_t>(a,dim,out,[](std::int16_t x){return (std::int64_t)x;},
        [](std::int64_t acc, std::int64_t){return (std::int16_t)acc;},[](){return (std::int64_t)0;},[](std::int64_t acc,std::int64_t x){return acc+x;});
      return;
    case ScalarType::Int8:
      reduce_dim_impl<std::int64_t,std::int8_t,std::int8_t>(a,dim,out,[](std::int8_t x){return (std::int64_t)x;},
        [](std::int64_t acc, std::int64_t){return (std::int8_t)acc;},[](){return (std::int64_t)0;},[](std::int64_t acc,std::int64_t x){return acc+x;});
      return;
    case ScalarType::UInt64:
      reduce_dim_impl<std::uint64_t,std::uint64_t,std::uint64_t>(a,dim,out,[](std::uint64_t x){return x;},
        [](std::uint64_t acc, std::int64_t){return acc;},[](){return (std::uint64_t)0;},[](std::uint64_t acc,std::uint64_t x){return acc+x;});
      return;
    case ScalarType::UInt32:
      reduce_dim_impl<std::uint64_t,std::uint32_t,std::uint32_t>(a,dim,out,[](std::uint32_t x){return (std::uint64_t)x;},
        [](std::uint64_t acc, std::int64_t){return (std::uint32_t)acc;},[](){return (std::uint64_t)0;},[](std::uint64_t acc,std::uint64_t x){return acc+x;});
      return;
    case ScalarType::UInt16:
      reduce_dim_impl<std::uint64_t,std::uint16_t,std::uint16_t>(a,dim,out,[](std::uint16_t x){return (std::uint64_t)x;},
        [](std::uint64_t acc, std::int64_t){return (std::uint16_t)acc;},[](){return (std::uint64_t)0;},[](std::uint64_t acc,std::uint64_t x){return acc+x;});
      return;
    case ScalarType::UInt8:
      reduce_dim_impl<std::uint64_t,std::uint8_t,std::uint8_t>(a,dim,out,[](std::uint8_t x){return (std::uint64_t)x;},
        [](std::uint64_t acc, std::int64_t){return (std::uint8_t)acc;},[](){return (std::uint64_t)0;},[](std::uint64_t acc,std::uint64_t x){return acc+x;});
      return;
    case ScalarType::Complex64:
      reduce_dim_impl<Complex64,Complex64,Complex64>(a,dim,out,
        [](Complex64 x){return x;},
        [](Complex64 acc, std::int64_t){return acc;},
        [](){return Complex64(0.0f,0.0f);},
        [](Complex64 acc, Complex64 x){ return acc + x; });
      return;
    case ScalarType::Complex128:
      reduce_dim_impl<Complex128,Complex128,Complex128>(a,dim,out,
        [](Complex128 x){return x;},
        [](Complex128 acc, std::int64_t){return acc;},
        [](){return Complex128(0.0,0.0);},
        [](Complex128 acc, Complex128 x){ return acc + x; });
      return;
default: break;
  }
  throw_error("sum_dim: dtype not implemented");
}

void prod_dim(const Series& a, std::int64_t dim, Series& out) {
  PLUSS_CHECK(a.is_contiguous(), "prod_dim: requires contiguous");
  switch (a.scalar_type()) {
    case ScalarType::Float64:
      reduce_dim_impl<double,double,double>(a,dim,out,[](double x){return x;},[](double acc, std::int64_t){return acc;},[](){return 1.0;},[](double acc,double x){return acc*x;});
      return;
    case ScalarType::Float32:
      reduce_dim_impl<float,float,float>(a,dim,out,[](float x){return x;},[](float acc, std::int64_t){return acc;},[](){return 1.0f;},[](float acc,float x){return acc*x;});
      return;

    case ScalarType::Float16:
      reduce_dim_impl<float,Float16,Float16>(a,dim,out,load_f16,[](float acc, std::int64_t){return store_f16(acc);},[](){return 1.0f;},[](float acc,float x){return acc*x;});
      return;
    case ScalarType::BFloat16:
      reduce_dim_impl<float,BFloat16,BFloat16>(a,dim,out,load_bf16,[](float acc, std::int64_t){return store_bf16(acc);},[](){return 1.0f;},[](float acc,float x){return acc*x;});
      return;
    case ScalarType::Int64:
      reduce_dim_impl<std::int64_t,std::int64_t,std::int64_t>(a,dim,out,[](std::int64_t x){return x;},[](std::int64_t acc, std::int64_t){return acc;},[](){return (std::int64_t)1;},[](std::int64_t acc,std::int64_t x){return acc*x;});
      return;
    case ScalarType::Int32:
      reduce_dim_impl<std::int64_t,std::int32_t,std::int32_t>(a,dim,out,[](std::int32_t x){return (std::int64_t)x;},[](std::int64_t acc, std::int64_t){return (std::int32_t)acc;},[](){return (std::int64_t)1;},[](std::int64_t acc,std::int64_t x){return acc*x;});
      return;

    case ScalarType::Complex64:
      reduce_dim_impl<Complex64,Complex64,Complex64>(a,dim,out,
        [](Complex64 x){return x;},
        [](Complex64 acc, std::int64_t){return acc;},
        [](){return Complex64(1.0f,0.0f);},
        [](Complex64 acc, Complex64 x){ return acc * x; });
      return;
    case ScalarType::Complex128:
      reduce_dim_impl<Complex128,Complex128,Complex128>(a,dim,out,
        [](Complex128 x){return x;},
        [](Complex128 acc, std::int64_t){return acc;},
        [](){return Complex128(1.0,0.0);},
        [](Complex128 acc, Complex128 x){ return acc * x; });
      return;
default: break;
  }
  throw_error("prod_dim: dtype not implemented");
}

void amax_dim(const Series& a, std::int64_t dim, Series& out) {
  PLUSS_CHECK(a.is_contiguous(), "amax_dim: requires contiguous");
  switch (a.scalar_type()) {
    case ScalarType::Float64:
      reduce_dim_impl<double,double,double>(a,dim,out,[](double x){return x;},[](double acc, std::int64_t){return acc;},[](){return -std::numeric_limits<double>::infinity();},[](double acc,double x){return std::max(acc,x);});
      return;
    case ScalarType::Float32:
      reduce_dim_impl<float,float,float>(a,dim,out,[](float x){return x;},[](float acc, std::int64_t){return acc;},[](){return -std::numeric_limits<float>::infinity();},[](float acc,float x){return std::max(acc,x);});
      return;
    case ScalarType::Int64:
      reduce_dim_impl<std::int64_t,std::int64_t,std::int64_t>(a,dim,out,[](std::int64_t x){return x;},[](std::int64_t acc, std::int64_t){return acc;},[](){return std::numeric_limits<std::int64_t>::min();},[](std::int64_t acc,std::int64_t x){return std::max(acc,x);});
      return;

    case ScalarType::Float16:
      reduce_dim_impl<float,Float16,Float16>(a,dim,out,load_f16,[](float acc, std::int64_t){return store_f16(acc);},[](){return -std::numeric_limits<float>::infinity();},[](float acc,float x){return std::max(acc,x);});
      return;
    case ScalarType::BFloat16:
      reduce_dim_impl<float,BFloat16,BFloat16>(a,dim,out,load_bf16,[](float acc, std::int64_t){return store_bf16(acc);},[](){return -std::numeric_limits<float>::infinity();},[](float acc,float x){return std::max(acc,x);});
      return;
    case ScalarType::Int32:
      reduce_dim_impl<std::int32_t,std::int32_t,std::int32_t>(a,dim,out,[](std::int32_t x){return x;},[](std::int32_t acc, std::int64_t){return acc;},[](){return std::numeric_limits<std::int32_t>::min();},[](std::int32_t acc,std::int32_t x){return std::max(acc,x);});
      return;
    case ScalarType::UInt8:
      reduce_dim_impl<std::uint8_t,std::uint8_t,std::uint8_t>(a,dim,out,[](std::uint8_t x){return x;},[](std::uint8_t acc, std::int64_t){return acc;},[](){return (std::uint8_t)0;},[](std::uint8_t acc,std::uint8_t x){return (std::uint8_t)std::max(acc,x);});
      return;
default: break;
  }
  throw_error("amax_dim: dtype not implemented");
}

void amin_dim(const Series& a, std::int64_t dim, Series& out) {
  PLUSS_CHECK(a.is_contiguous(), "amin_dim: requires contiguous");
  switch (a.scalar_type()) {
    case ScalarType::Float64:
      reduce_dim_impl<double,double,double>(a,dim,out,[](double x){return x;},[](double acc, std::int64_t){return acc;},[](){return std::numeric_limits<double>::infinity();},[](double acc,double x){return std::min(acc,x);});
      return;
    case ScalarType::Float32:
      reduce_dim_impl<float,float,float>(a,dim,out,[](float x){return x;},[](float acc, std::int64_t){return acc;},[](){return std::numeric_limits<float>::infinity();},[](float acc,float x){return std::min(acc,x);});
      return;
    case ScalarType::Int64:
      reduce_dim_impl<std::int64_t,std::int64_t,std::int64_t>(a,dim,out,[](std::int64_t x){return x;},[](std::int64_t acc, std::int64_t){return acc;},[](){return std::numeric_limits<std::int64_t>::max();},[](std::int64_t acc,std::int64_t x){return std::min(acc,x);});
      return;

    case ScalarType::Float16:
      reduce_dim_impl<float,Float16,Float16>(a,dim,out,load_f16,[](float acc, std::int64_t){return store_f16(acc);},[](){return std::numeric_limits<float>::infinity();},[](float acc,float x){return std::min(acc,x);});
      return;
    case ScalarType::BFloat16:
      reduce_dim_impl<float,BFloat16,BFloat16>(a,dim,out,load_bf16,[](float acc, std::int64_t){return store_bf16(acc);},[](){return std::numeric_limits<float>::infinity();},[](float acc,float x){return std::min(acc,x);});
      return;
    case ScalarType::Int32:
      reduce_dim_impl<std::int32_t,std::int32_t,std::int32_t>(a,dim,out,[](std::int32_t x){return x;},[](std::int32_t acc, std::int64_t){return acc;},[](){return std::numeric_limits<std::int32_t>::max();},[](std::int32_t acc,std::int32_t x){return std::min(acc,x);});
      return;
    case ScalarType::UInt8:
      reduce_dim_impl<std::uint8_t,std::uint8_t,std::uint8_t>(a,dim,out,[](std::uint8_t x){return x;},[](std::uint8_t acc, std::int64_t){return acc;},[](){return (std::uint8_t)255;},[](std::uint8_t acc,std::uint8_t x){return (std::uint8_t)std::min(acc,x);});
      return;
default: break;
  }
  throw_error("amin_dim: dtype not implemented");
}

void all_dim(const Series& a, std::int64_t dim, Series& out) {
  PLUSS_CHECK(a.is_contiguous(), "all_dim: requires contiguous");
  PLUSS_CHECK(out.scalar_type()==ScalarType::Bool, "all_dim: out must be Bool");
  const auto* ap = reinterpret_cast<const std::uint8_t*>(a.raw_data());
  auto* op = reinterpret_cast<std::uint8_t*>(out.raw_data());
  std::vector<std::int64_t> out_sz; std::int64_t outer, reduce, inner;
  shape_for_reduce(a, dim, out_sz, outer, reduce, inner);
  const std::int64_t out_elems = outer*inner;
  for (std::int64_t oi=0; oi<out_elems; ++oi) {
    const std::int64_t o = oi/inner;
    const std::int64_t i = oi - o*inner;
    const std::int64_t base = o*reduce*inner + i;
    std::uint8_t acc = 1;
    for (std::int64_t r=0;r<reduce;++r) {
      if (ap[base + r*inner]==0) { acc=0; break; }
    }
    op[oi]=acc;
  }
}

void any_dim(const Series& a, std::int64_t dim, Series& out) {
  PLUSS_CHECK(a.is_contiguous(), "any_dim: requires contiguous");
  PLUSS_CHECK(out.scalar_type()==ScalarType::Bool, "any_dim: out must be Bool");
  const auto* ap = reinterpret_cast<const std::uint8_t*>(a.raw_data());
  auto* op = reinterpret_cast<std::uint8_t*>(out.raw_data());
  std::vector<std::int64_t> out_sz; std::int64_t outer, reduce, inner;
  shape_for_reduce(a, dim, out_sz, outer, reduce, inner);
  const std::int64_t out_elems = outer*inner;
  for (std::int64_t oi=0; oi<out_elems; ++oi) {
    const std::int64_t o = oi/inner;
    const std::int64_t i = oi - o*inner;
    const std::int64_t base = o*reduce*inner + i;
    std::uint8_t acc = 0;
    for (std::int64_t r=0;r<reduce;++r) {
      if (ap[base + r*inner]!=0) { acc=1; break; }
    }
    op[oi]=acc;
  }
}

void sum_all(const Series& a, Series& out) {
  PLUSS_CHECK(a.is_contiguous(), "sum_all: requires contiguous");
  PLUSS_CHECK(out.numel()==1, "sum_all: out must be scalar");
  switch (a.scalar_type()) {
    case ScalarType::Float64: {
      const auto* ap = reinterpret_cast<const double*>(a.raw_data());
      double acc=0; for (std::int64_t i=0;i<a.numel();++i) acc += ap[i];
      *reinterpret_cast<double*>(out.raw_data()) = acc;
      return;
    }
    case ScalarType::Float32: {
      const auto* ap = reinterpret_cast<const float*>(a.raw_data());
      float acc=0; for (std::int64_t i=0;i<a.numel();++i) acc += ap[i];
      *reinterpret_cast<float*>(out.raw_data()) = acc;
      return;
    }

    case ScalarType::Float16: {
      const auto* ap = reinterpret_cast<const Float16*>(a.raw_data());
      float acc=0; for (std::int64_t i=0;i<a.numel();++i) acc += float16_to_float(ap[i]);
      *reinterpret_cast<Float16*>(out.raw_data()) = float16_from_float(acc);
      return;
    }
    case ScalarType::BFloat16: {
      const auto* ap = reinterpret_cast<const BFloat16*>(a.raw_data());
      float acc=0; for (std::int64_t i=0;i<a.numel();++i) acc += bfloat16_to_float(ap[i]);
      *reinterpret_cast<BFloat16*>(out.raw_data()) = bfloat16_from_float(acc);
      return;
    }
    case ScalarType::Int64: {
      const auto* ap = reinterpret_cast<const std::int64_t*>(a.raw_data());
      std::int64_t acc=0; for (std::int64_t i=0;i<a.numel();++i) acc += ap[i];
      *reinterpret_cast<std::int64_t*>(out.raw_data()) = acc;
      return;
    }
    case ScalarType::Int32: {
      const auto* ap = reinterpret_cast<const std::int32_t*>(a.raw_data());
      std::int64_t acc=0; for (std::int64_t i=0;i<a.numel();++i) acc += ap[i];
      *reinterpret_cast<std::int32_t*>(out.raw_data()) = (std::int32_t)acc;
      return;
    }
    case ScalarType::Complex64: {
      const auto* ap = reinterpret_cast<const Complex64*>(a.raw_data());
      float re=0, im=0; for (std::int64_t i=0;i<a.numel();++i){ re += std::real(ap[i]); im += std::imag(ap[i]); }
      *reinterpret_cast<Complex64*>(out.raw_data()) = Complex64(re,im);
      return;
    }
    case ScalarType::Complex128: {
      const auto* ap = reinterpret_cast<const Complex128*>(a.raw_data());
      double re=0, im=0; for (std::int64_t i=0;i<a.numel();++i){ re += std::real(ap[i]); im += std::imag(ap[i]); }
      *reinterpret_cast<Complex128*>(out.raw_data()) = Complex128(re,im);
      return;
    }
default: break;
  }
  throw_error("sum_all: dtype not implemented");
}

void mean_all(const Series& a, Series& out) {
  sum_all(a, out);
  switch (out.scalar_type()) {
    case ScalarType::Float64: *reinterpret_cast<double*>(out.raw_data()) /= (double)a.numel(); return;
    case ScalarType::Float32: *reinterpret_cast<float*>(out.raw_data()) /= (float)a.numel(); return;
    case ScalarType::Float16: *reinterpret_cast<Float16*>(out.raw_data()) = float16_from_float(float16_to_float(*reinterpret_cast<Float16*>(out.raw_data())) / (float)a.numel()); return;
    case ScalarType::BFloat16: *reinterpret_cast<BFloat16*>(out.raw_data()) = bfloat16_from_float(bfloat16_to_float(*reinterpret_cast<BFloat16*>(out.raw_data())) / (float)a.numel()); return;
    default: break;
  }
  throw_error("mean_all: dtype not implemented");
}

} // namespace
