#include "CodeM/native/cpu/ScatterOps.h"
#include "CodeM/native/cpu/ScatterKernels.h"
#include "CodeM/core/Errors.h"

#include <string>

namespace pluss::codem::native::cpu {

namespace {

static std::string to_reduce_name(ScatterReduce reduce) {
  switch (reduce) {
    case ScatterReduce::Sum: return "sum";
    case ScatterReduce::AMax: return "amax";
    case ScatterReduce::AMin: return "amin";
    default: break;
  }
  throw_error("scatter_reduce: unsupported reduce");
}

} // namespace

void scatter_add(const Series& self, std::int64_t dim, const Series& index, const Series& src, Series& out) {
  out = native::cpu::scatter_add(self, dim, index, src);
}

void scatter_reduce(const Series& self, std::int64_t dim, const Series& index, const Series& src,
                    ScatterReduce reduce, Series& out,
                    bool include_self, bool has_initial, double initial_scalar) {
  const auto reduce_name = to_reduce_name(reduce);
  out = has_initial
      ? native::cpu::scatter_reduce(self, dim, index, src, reduce_name, include_self, true, initial_scalar)
      : native::cpu::scatter_reduce(self, dim, index, src, reduce_name, include_self);
}

} // namespace pluss::codem::native::cpu
