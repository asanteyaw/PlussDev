#include "CodeM/Traverse/Traverse.h"

#include <utility>
#include <vector>

#include "CodeM/Traverse/TraverseTypes.h"
#include "CodeM/core/Errors.h"

namespace pluss::codem {
namespace {

std::vector<std::int64_t> shape_vec_(const Series& x) {
  return std::vector<std::int64_t>(x.sizes().begin(), x.sizes().end());
}

bool same_shape_(const Series& a, const Series& b) {
  if (a.dim() != b.dim()) {
    return false;
  }
  for (std::int64_t d = 0; d < a.dim(); ++d) {
    if (a.size(d) != b.size(d)) {
      return false;
    }
  }
  return true;
}

Series normalize_mask_for_source_(const Series& source, const Series& mask) {
  PLUSS_CHECK(mask.defined(), "Series::traverse: mask is undefined");

  Series out = mask;
  if (out.platform() != source.platform()) {
    out = out.to(source.platform());
  }
  if (out.scalar_type() != ScalarType::Bool) {
    out = out.ne(0.0);
  }

  PLUSS_CHECK(
      same_shape_(source, out),
      "Series::traverse: mask shape must exactly match the queried Series/view shape");
  return out;
}

TraverseMatch make_match_from_mask_(const Series& source, const Series& mask) {
  const Series normalized_mask = normalize_mask_for_source_(source, mask);
  const Series coords = normalized_mask.nonzero();
  return TraverseMatch(coords, shape_vec_(source), source.dim());
}

TraverseMatch make_empty_match_(const Series& source) {
  const std::vector<std::int64_t> shape = shape_vec_(source);
  Series coords = Series::empty({0, source.dim()}, ScalarType::Int64, source.platform());
  return TraverseMatch(std::move(coords), shape, source.dim());
}

} // namespace

SeriesTraverseProxy::SeriesTraverseProxy(const Series& owner)
    : owner_(&owner), spec_(normalize_query_spec(TraverseQuerySpec{})) {}

SeriesTraverseProxy::SeriesTraverseProxy(const Series& owner, TraverseQuerySpec spec)
    : owner_(&owner), spec_(normalize_query_spec(spec)) {}

const Series& SeriesTraverseProxy::owner() const {
  PLUSS_CHECK(owner_ != nullptr, "Series::traverse: unbound proxy");
  return *owner_;
}

Series SeriesTraverseProxy::source_() const {
  const Series& src = owner();
  if (!spec_.has_on_clause()) {
    return src;
  }
  return src.traverse_slice(pluss::msharp::RangeView::Of<ExtractSpec>(spec_.on_specs));
}

TraverseMatch SeriesTraverseProxy::find_all_from_mask_(const Series& mask) const {
  const Series source = source_();
  return make_match_from_mask_(source, mask);
}

TraverseMatch SeriesTraverseProxy::find_all(const Series& mask) const {
  return find_all_from_mask_(mask);
}

TraverseMatch SeriesTraverseProxy::find_if(const Series& mask) const {
  return find_all_from_mask_(mask);
}

TraverseMatch SeriesTraverseProxy::find_eq(const Series& rhs) const {
  const Series source = source_();
  return find_all_from_mask_(source.eq(rhs));
}

TraverseMatch SeriesTraverseProxy::find_eq(double rhs) const {
  const Series source = source_();
  return find_all_from_mask_(source.eq(rhs));
}

TraverseMatch SeriesTraverseProxy::find_neq(const Series& rhs) const {
  const Series source = source_();
  return find_all_from_mask_(source.ne(rhs));
}

TraverseMatch SeriesTraverseProxy::find_neq(double rhs) const {
  const Series source = source_();
  return find_all_from_mask_(source.ne(rhs));
}

TraverseMatch SeriesTraverseProxy::find_ge(const Series& rhs) const {
  const Series source = source_();
  return find_all_from_mask_(source.ge(rhs));
}

TraverseMatch SeriesTraverseProxy::find_ge(double rhs) const {
  const Series source = source_();
  return find_all_from_mask_(source.ge(rhs));
}

TraverseMatch SeriesTraverseProxy::find_le(const Series& rhs) const {
  const Series source = source_();
  return find_all_from_mask_(source.le(rhs));
}

TraverseMatch SeriesTraverseProxy::find_le(double rhs) const {
  const Series source = source_();
  return find_all_from_mask_(source.le(rhs));
}

TraverseMatch SeriesTraverseProxy::find_btw(const Series& lo, const Series& hi, bool inclusive) const {
  const Series source = source_();
  const Series mask = inclusive
                          ? source.ge(lo).logical_and(source.le(hi))
                          : source.gt(lo).logical_and(source.lt(hi));
  return find_all_from_mask_(mask);
}

TraverseMatch SeriesTraverseProxy::find_btw(double lo, double hi, bool inclusive) const {
  const Series source = source_();
  const Series mask = inclusive
                          ? source.ge(lo).logical_and(source.le(hi))
                          : source.gt(lo).logical_and(source.lt(hi));
  return find_all_from_mask_(mask);
}

TraverseMatch SeriesTraverseProxy::find_and(const Series& lhs_mask, const Series& rhs_mask) const {
  return find_all_from_mask_(lhs_mask.logical_and(rhs_mask));
}

TraverseMatch SeriesTraverseProxy::find_or(const Series& lhs_mask, const Series& rhs_mask) const {
  return find_all_from_mask_(lhs_mask.logical_or(rhs_mask));
}

TraverseMatch SeriesTraverseProxy::find_not(const Series& mask) const {
  return find_all_from_mask_(mask.logical_not());
}

TraverseMatch SeriesTraverseProxy::find_first(const Series& mask, std::int64_t dim, bool per_batch) const {
  PLUSS_CHECK(
      dim == -1 && !per_batch,
      "Series::traverse.find_first: only global-first mode is currently supported; "
      "dim/per_batch variants are intentionally deferred");

  const Series source = source_();
  const Series normalized_mask = normalize_mask_for_source_(source, mask);
  const Series all_coords = normalized_mask.nonzero();

  if (all_coords.size(0) == 0) {
    return make_empty_match_(source);
  }

  const Series first = all_coords.select(0, 0).unsqueeze(0);
  return TraverseMatch(first, shape_vec_(source), source.dim());
}

SeriesTraverseAccessor::SeriesTraverseAccessor(Series& owner) : owner_(&owner) {}

void SeriesTraverseAccessor::rebind(Series& owner) noexcept {
  owner_ = &owner;
}

Series& SeriesTraverseAccessor::owner() const {
  PLUSS_CHECK(owner_ != nullptr, "Series::traverse: unbound accessor");
  return *owner_;
}

SeriesTraverseProxy SeriesTraverseAccessor::operator()() const {
  return SeriesTraverseProxy(owner());
}

SeriesTraverseProxy SeriesTraverseAccessor::operator()(pluss::msharp::RangeView::Of<ExtractSpec> on_specs) const {
  TraverseQuerySpec spec;
  spec.on_specs.assign(on_specs.begin(), on_specs.end());
  return SeriesTraverseProxy(owner(), std::move(spec));
}

TraverseMatch SeriesTraverseAccessor::find_all(const Series& mask) const {
  return (*this)().find_all(mask);
}

TraverseMatch SeriesTraverseAccessor::find_if(const Series& mask) const {
  return (*this)().find_if(mask);
}

TraverseMatch SeriesTraverseAccessor::find_eq(const Series& rhs) const {
  return (*this)().find_eq(rhs);
}

TraverseMatch SeriesTraverseAccessor::find_eq(double rhs) const {
  return (*this)().find_eq(rhs);
}

TraverseMatch SeriesTraverseAccessor::find_neq(const Series& rhs) const {
  return (*this)().find_neq(rhs);
}

TraverseMatch SeriesTraverseAccessor::find_neq(double rhs) const {
  return (*this)().find_neq(rhs);
}

TraverseMatch SeriesTraverseAccessor::find_ge(const Series& rhs) const {
  return (*this)().find_ge(rhs);
}

TraverseMatch SeriesTraverseAccessor::find_ge(double rhs) const {
  return (*this)().find_ge(rhs);
}

TraverseMatch SeriesTraverseAccessor::find_le(const Series& rhs) const {
  return (*this)().find_le(rhs);
}

TraverseMatch SeriesTraverseAccessor::find_le(double rhs) const {
  return (*this)().find_le(rhs);
}

TraverseMatch SeriesTraverseAccessor::find_btw(const Series& lo, const Series& hi, bool inclusive) const {
  return (*this)().find_btw(lo, hi, inclusive);
}

TraverseMatch SeriesTraverseAccessor::find_btw(double lo, double hi, bool inclusive) const {
  return (*this)().find_btw(lo, hi, inclusive);
}

TraverseMatch SeriesTraverseAccessor::find_and(const Series& lhs_mask, const Series& rhs_mask) const {
  return (*this)().find_and(lhs_mask, rhs_mask);
}

TraverseMatch SeriesTraverseAccessor::find_or(const Series& lhs_mask, const Series& rhs_mask) const {
  return (*this)().find_or(lhs_mask, rhs_mask);
}

TraverseMatch SeriesTraverseAccessor::find_not(const Series& mask) const {
  return (*this)().find_not(mask);
}

TraverseMatch SeriesTraverseAccessor::find_first(const Series& mask, std::int64_t dim, bool per_batch) const {
  return (*this)().find_first(mask, dim, per_batch);
}

} // namespace pluss::codem
