#pragma once

#include <cstdint>
#include <initializer_list>
#include <span>

#include "CodeM/Traverse/TraverseFwd.h"
#include "CodeM/Traverse/TraverseQuerySpec.h"
#include "MSharp/RangeView.h"
#include "pluss/export.h"

namespace pluss::codem {

class CODEM_API SeriesTraverseProxy final {
public:
  SeriesTraverseProxy() = default;
  explicit SeriesTraverseProxy(const Series& owner);
  SeriesTraverseProxy(const Series& owner, TraverseQuerySpec spec);

  TraverseMatch find_all(const Series& mask) const;
  TraverseMatch find_if(const Series& mask) const;

  TraverseMatch find_eq(const Series& rhs) const;
  TraverseMatch find_eq(double rhs) const;
  TraverseMatch find_neq(const Series& rhs) const;
  TraverseMatch find_neq(double rhs) const;
  TraverseMatch find_ge(const Series& rhs) const;
  TraverseMatch find_ge(double rhs) const;
  TraverseMatch find_le(const Series& rhs) const;
  TraverseMatch find_le(double rhs) const;
  TraverseMatch find_btw(const Series& lo, const Series& hi, bool inclusive = true) const;
  TraverseMatch find_btw(double lo, double hi, bool inclusive = true) const;

  TraverseMatch find_and(const Series& lhs_mask, const Series& rhs_mask) const;
  TraverseMatch find_or(const Series& lhs_mask, const Series& rhs_mask) const;
  TraverseMatch find_not(const Series& mask) const;

  TraverseMatch find_first(const Series& mask, std::int64_t dim = -1, bool per_batch = false) const;

private:
  const Series& owner() const;
  Series source_() const;
  TraverseMatch find_all_from_mask_(const Series& mask) const;

  const Series* owner_{nullptr};
  TraverseQuerySpec spec_{};
};

class CODEM_API SeriesTraverseAccessor final {
public:
  SeriesTraverseAccessor() = default;
  explicit SeriesTraverseAccessor(Series& owner);

  void rebind(Series& owner) noexcept;

  SeriesTraverseProxy operator()() const;
  SeriesTraverseProxy operator()(std::initializer_list<ExtractSpec> on_specs) const;
  SeriesTraverseProxy operator()(std::span<const ExtractSpec> on_specs) const;
  SeriesTraverseProxy operator()(pluss::msharp::RangeView::Of<ExtractSpec> on_specs) const;

  TraverseMatch find_all(const Series& mask) const;
  TraverseMatch find_if(const Series& mask) const;

  TraverseMatch find_eq(const Series& rhs) const;
  TraverseMatch find_eq(double rhs) const;
  TraverseMatch find_neq(const Series& rhs) const;
  TraverseMatch find_neq(double rhs) const;
  TraverseMatch find_ge(const Series& rhs) const;
  TraverseMatch find_ge(double rhs) const;
  TraverseMatch find_le(const Series& rhs) const;
  TraverseMatch find_le(double rhs) const;
  TraverseMatch find_btw(const Series& lo, const Series& hi, bool inclusive = true) const;
  TraverseMatch find_btw(double lo, double hi, bool inclusive = true) const;

  TraverseMatch find_and(const Series& lhs_mask, const Series& rhs_mask) const;
  TraverseMatch find_or(const Series& lhs_mask, const Series& rhs_mask) const;
  TraverseMatch find_not(const Series& mask) const;

  TraverseMatch find_first(const Series& mask, std::int64_t dim = -1, bool per_batch = false) const;

private:
  Series& owner() const;

  Series* owner_{nullptr};
};

} // namespace pluss::codem
