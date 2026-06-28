#pragma once
#include "CodeM/core/SeriesSlim.h"
#include <string>

namespace pluss::codem::native::cpu {

  // scatter_add: out = a.clone(); out.scatter_add_(dim, index, src)
  // index is int64, src broadcastable to index (Torch-ish).
  Series scatter_add(const Series& a, std::int64_t dim, const Series& index, const Series& src);
  void scatter_add_inplace(Series& a, std::int64_t dim, const Series& index, const Series& src);

  // scatter_reduce: reduce in {"sum","amax","amin"} (cpu-only subset)
  Series scatter_reduce(const Series& a, std::int64_t dim, const Series& index, const Series& src,
                        const std::string& reduce, bool include_self=true);
  Series scatter_reduce(const Series& a, std::int64_t dim, const Series& index, const Series& src,
                        const std::string& reduce, bool include_self, bool has_initial, double initial);
  void scatter_reduce_inplace(Series& a, std::int64_t dim, const Series& index, const Series& src,
                              const std::string& reduce, bool include_self=true);
  void scatter_reduce_inplace(Series& a, std::int64_t dim, const Series& index, const Series& src,
                              const std::string& reduce, bool include_self, bool has_initial, double initial);

}
