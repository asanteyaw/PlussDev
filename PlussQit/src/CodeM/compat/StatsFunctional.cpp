#include "pluss/subsp/stats/stats.h"

#include "CodeM/core/Errors.h"

#include <string>

namespace pluss::subsp::stats {
namespace {

inline void require_defined_(const Series& x, const char* op) {
  PLUSS_CHECK(x.defined(), std::string("subsp::stats::") + op + ": expected defined Series");
}

inline void require_same_platform_(const Series& x, const Series& y, const char* op) {
  PLUSS_CHECK(
      x.platform() == y.platform(),
      std::string("subsp::stats::") + op + ": expected matching platforms");
}

inline Series invoke_self_(std::string_view op, const Series& x, const char* opname) {
  require_defined_(x, opname);
  return pluss::codem::_compat::invoke(op, x);
}

inline Series invoke_self_window_(std::string_view op,
                                  const char* opname,
                                  const Series& x,
                                  std::int64_t window,
                                  std::int64_t dim) {
  require_defined_(x, opname);
  if (dim == -1) {
    return pluss::codem::_compat::invoke(op, x, {window});
  }
  return pluss::codem::_compat::invoke(op, x, {window, dim});
}

inline Series invoke_pair_(std::string_view op, const char* opname, const Series& x, const Series& y) {
  require_defined_(x, opname);
  require_defined_(y, opname);
  require_same_platform_(x, y, opname);
  return pluss::codem::_compat::invoke(op, x, {y});
}

} // namespace

Series mean(const Series& x) {
  return invoke_self_("mean", x, "mean");
}

Series mean(const Series& x, std::int64_t dim, bool keepdim) {
  require_defined_(x, "mean");
  return x.mean(dim, keepdim);
}

Series var(const Series& x, bool unbiased) {
  require_defined_(x, "var");
  return x.var(unbiased);
}

Series var(const Series& x, std::int64_t dim, bool unbiased, bool keepdim) {
  require_defined_(x, "var");
  return x.var(dim, unbiased, keepdim);
}

Series std(const Series& x, bool unbiased) {
  require_defined_(x, "std");
  return x.std(unbiased);
}

Series std(const Series& x, std::int64_t dim, bool unbiased, bool keepdim) {
  require_defined_(x, "std");
  return x.std(dim, unbiased, keepdim);
}

Series nanvar(const Series& x) {
  return invoke_self_("nanvar", x, "nanvar");
}

Series nanstd(const Series& x) {
  return invoke_self_("nanstd", x, "nanstd");
}

Series skewness(const Series& x) {
  return invoke_self_("skewness", x, "skewness");
}

Series kurtosis(const Series& x) {
  return invoke_self_("kurtosis", x, "kurtosis");
}

Series movsum(const Series& x, std::int64_t window, std::int64_t dim) {
  return invoke_self_window_("movsum", "movsum", x, window, dim);
}

Series movmean(const Series& x, std::int64_t window, std::int64_t dim) {
  return invoke_self_window_("movmean", "movmean", x, window, dim);
}

Series movmax(const Series& x, std::int64_t window, std::int64_t dim) {
  return invoke_self_window_("movmax", "movmax", x, window, dim);
}

Series movmin(const Series& x, std::int64_t window, std::int64_t dim) {
  return invoke_self_window_("movmin", "movmin", x, window, dim);
}

Series movstd(const Series& x, std::int64_t window, std::int64_t dim) {
  return invoke_self_window_("movstd", "movstd", x, window, dim);
}

Series movvar(const Series& x, std::int64_t window, std::int64_t dim) {
  return invoke_self_window_("movvar", "movvar", x, window, dim);
}

Series normr(const Series& x) {
  return invoke_self_("normr", x, "normr");
}

Series normc(const Series& x) {
  return invoke_self_("normc", x, "normc");
}

Series standardize(const Series& x) {
  return invoke_self_("standardize", x, "standardize");
}

Series center(const Series& x) {
  return invoke_self_("center", x, "center");
}

Series pdist2(const Series& x, const Series& y) {
  return invoke_pair_("pdist2", "pdist2", x, y);
}

Series cdist(const Series& x, const Series& y) {
  return invoke_pair_("cdist", "cdist", x, y);
}

} // namespace pluss::subsp::stats
