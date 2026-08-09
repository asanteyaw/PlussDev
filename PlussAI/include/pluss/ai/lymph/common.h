#pragma once

#include "pluss/ai/nodej/nodej.h"
#include "pluss/ai/nodej/sparse_runtime.h"
#include "pluss/codem/series_factory.h"
#include "table/DataFrame.h"

#include <algorithm>
#include <bit>
#include <cstdint>
#include <functional>
#include <future>
#include <initializer_list>
#include <limits>
#include <memory>
#include <numeric>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

namespace pluss::ai::lymph {

using ::pluss::ai::nodej::AssessOptions;
using ::pluss::ai::nodej::BalanceCleanKind;
using ::pluss::ai::nodej::BalanceCleanVote;
using ::pluss::ai::nodej::BalanceGrowthKind;
using ::pluss::ai::nodej::BalancePlan;
using ::pluss::ai::nodej::BalanceTarget;
using ::pluss::ai::nodej::BalanceTargetKind;
using ::pluss::ai::nodej::BayesFamilyOptions;
using ::pluss::ai::nodej::BayesLikelihood;
using ::pluss::ai::nodej::CategoryMap;
using ::pluss::ai::nodej::CategoryMaps;
using ::pluss::ai::nodej::ClassMix;
using ::pluss::ai::nodej::ClassReport;
using ::pluss::ai::nodej::ClassReportOptions;
using ::pluss::ai::nodej::ClassWeightMode;
using ::pluss::ai::nodej::ClassifierBase;
using ::pluss::ai::nodej::ClusterFamilyOptions;
using ::pluss::ai::nodej::ClusterMethod;
using ::pluss::ai::nodej::ConfusionNormalize;
using ::pluss::ai::nodej::ConfusionOptions;
using ::pluss::ai::nodej::ConfusionTable;
using ::pluss::ai::nodej::CrossAssessReport;
using ::pluss::ai::nodej::CVPlanKind;
using ::pluss::ai::nodej::DataFrameBatchOptions;
using ::pluss::ai::nodej::DataNotePolicy;
using ::pluss::ai::nodej::DataThreadPolicy;
using ::pluss::ai::nodej::DependenceCurve;
using ::pluss::ai::nodej::DependenceSurface;
using ::pluss::ai::nodej::DecomposeFamilyOptions;
using ::pluss::ai::nodej::DecomposeMethod;
using ::pluss::ai::nodej::EnsembleFamilyOptions;
using ::pluss::ai::nodej::EnsembleMethod;
using ::pluss::ai::nodej::FeatureBatch;
using ::pluss::ai::nodej::Flow;
using ::pluss::ai::nodej::FlowGraphInfo;
using ::pluss::ai::nodej::FlowStageInfo;
using ::pluss::ai::nodej::FoldAssessResult;
using ::pluss::ai::nodej::IndexSplit;
using ::pluss::ai::nodej::InferOptions;
using ::pluss::ai::nodej::KernelFamilyOptions;
using ::pluss::ai::nodej::KernelForm;
using ::pluss::ai::nodej::KernelKind;
using ::pluss::ai::nodej::LabeledBatch;
using ::pluss::ai::nodej::LinearFamilyOptions;
using ::pluss::ai::nodej::LinearObjective;
using ::pluss::ai::nodej::MetricKind;
using ::pluss::ai::nodej::MissingPolicy;
using ::pluss::ai::nodej::ModelFamily;
using ::pluss::ai::nodej::ModelSpec;
using ::pluss::ai::nodej::NeighborFamilyOptions;
using ::pluss::ai::nodej::NeighborQueryMode;
using ::pluss::ai::nodej::ObservationLayout;
using ::pluss::ai::nodej::PermuteImpactReport;
using ::pluss::ai::nodej::ProbeSpecSampler;
using ::pluss::ai::nodej::RandomState;
using ::pluss::ai::nodej::RegressorBase;
using ::pluss::ai::nodej::RegularizationKind;
using ::pluss::ai::nodej::RouteRemainderPolicy;
using ::pluss::ai::nodej::ScoreCalibration;
using ::pluss::ai::nodej::SearchKind;
using ::pluss::ai::nodej::SplitOptions;
using ::pluss::ai::nodej::SplitPlan;
using ::pluss::ai::nodej::Status;
using ::pluss::ai::nodej::StatusOr;
using ::pluss::ai::nodej::TaskKind;
using ::pluss::ai::nodej::ThresholdCurve;
using ::pluss::ai::nodej::TrainOptions;
using ::pluss::ai::nodej::TransformerBase;
using ::pluss::ai::nodej::TransformKind;
using ::pluss::ai::nodej::TransformOptions;
using ::pluss::ai::nodej::TransformSpec;
using ::pluss::ai::nodej::TreeCriterion;
using ::pluss::ai::nodej::TreeFamilyOptions;
using ::pluss::ai::nodej::TuneOptions;
using ::pluss::ai::nodej::TuneObjectivePolicy;
using ::pluss::ai::nodej::TuneReport;
using ::pluss::ai::nodej::UnknownCategoryPolicy;
using ::pluss::ai::nodej::WeightPlan;
using ::pluss::Series;
using ::pluss::Config;
using ::pluss::ScalarType;

namespace io = ::pluss::ai::nodej::io;


template <class T>
T resolve(StatusOr<T> value_or, std::string_view where = {}) {
  if (!value_or.ok()) {
    const std::string prefix = where.empty() ? std::string{} : (std::string(where) + ": ");
    ::pluss::throw_error(prefix + value_or.status().message);
  }
  return value_or.move_value();
}

inline void resolve(Status status, std::string_view where = {}) {
  if (status.ok()) return;
  const std::string prefix = where.empty() ? std::string{} : (std::string(where) + ": ");
  ::pluss::throw_error(prefix + status.message);
}

[[nodiscard]] inline std::vector<std::int64_t> copy_feature_route(pluss::msharp::RangeView::Int feature_route) {
  std::vector<std::int64_t> out{};
  out.reserve(feature_route.size());
  out.insert(out.end(), feature_route.begin(), feature_route.end());
  return out;
}

template <class T>
[[nodiscard]] inline std::vector<T> copy_range_values(pluss::msharp::RangeView::Of<T> values) {
  std::vector<T> out{};
  out.reserve(values.size());
  out.insert(out.end(), values.begin(), values.end());
  return out;
}

namespace detail {

// DataFrame::dtypes() reports "<Kind><inner>"-shaped strings for every
// column regardless of thread kind -- "Series<float64>", "Note<utf8>",
// "Timeline<...>", ... -- so this strip is kind-agnostic by construction
// (it just takes whatever sits between the first '<' and the last '>'); the
// two route_where overloads below both use it, one restricted to Series
// columns (route_where's original, Note-blind shape), the other walking an
// explicit ordered name list so a Note column is just as visible as any
// other.
[[nodiscard]] inline std::string strip_series_dtype_(const std::string& combined) {
  const auto open = combined.find('<');
  const auto close = combined.rfind('>');
  if (open == std::string::npos || close == std::string::npos || close <= open) return combined;
  return combined.substr(open + 1, close - open - 1);
}

} // namespace detail

// Indices of `names` within `frame`'s Series-column order -- the same order
// every bare-DataFrame convenience overload (learn(x,y), cross_validate(x,y),
// ...) resolves a FeatureBatch in, so these indices plug directly into any
// prep stage's feature_route without a hand-built index list.
[[nodiscard]] inline std::vector<std::int64_t> route(const pluss::table::DataFrame& frame,
                                                      pluss::msharp::RangeView::Of<std::string_view> names) {
  const auto ordered = frame.feataxis().series().features();
  std::unordered_map<std::string_view, std::int64_t> index_of{};
  index_of.reserve(ordered.size());
  for (std::size_t i = 0; i < ordered.size(); ++i) {
    index_of.emplace(ordered[i], static_cast<std::int64_t>(i));
  }
  std::vector<std::int64_t> out{};
  out.reserve(names.size());
  for (const std::string_view name : names) {
    const auto it = index_of.find(name);
    if (it == index_of.end()) {
      ::pluss::throw_error("lymph::route: \"" + std::string(name)
                           + "\" is not one of the DataFrame's Series-kind columns.");
    }
    out.push_back(it->second);
  }
  return out;
}

[[nodiscard]] inline std::vector<std::int64_t> route(const pluss::table::DataFrame& frame,
                                                      std::initializer_list<std::string_view> names) {
  return route(frame, pluss::msharp::RangeView::Of<std::string_view>(names));
}

// Indices of every Series-kind column whose (name, dtype) satisfies
// `predicate` -- the generic dtype-selector counterpart to route(), for
// picking columns by shape ("every int64 column") rather than by an
// explicit name list.
[[nodiscard]] inline std::vector<std::int64_t> route_where(
    const pluss::table::DataFrame& frame,
    const std::function<bool(std::string_view feature_name, std::string_view dtype)>& predicate) {
  const auto ordered = frame.feataxis().series().features();
  const auto dtypes = frame.dtypes();
  std::vector<std::int64_t> out{};
  out.reserve(ordered.size());
  for (std::size_t i = 0; i < ordered.size(); ++i) {
    const std::string dtype = detail::strip_series_dtype_(dtypes.at(ordered[i]));
    if (predicate(ordered[i], dtype)) {
      out.push_back(static_cast<std::int64_t>(i));
    }
  }
  return out;
}

// Note-aware sibling of route_where above: walks `ordered_names` (typically
// a `DataFrameBatchOptions::feature_names` list, per the DataFrame-free
// route()/route_remaining() comment below) instead of
// `frame.feataxis().series().features()`, so a Note-typed column --
// invisible to the frame-only overload above -- is just as selectable by the
// predicate as any Series column, and the returned indices line up with the
// order a Note-including FeatureBatch (DataNotePolicy::Factorize) actually
// uses. `frame.dtypes()` already reports a "<Kind><inner>" entry for every
// column regardless of kind, so no extra lookup machinery is needed here.
[[nodiscard]] inline std::vector<std::int64_t> route_where(
    const pluss::table::DataFrame& frame,
    const std::vector<std::string>& ordered_names,
    const std::function<bool(std::string_view feature_name, std::string_view dtype)>& predicate) {
  const auto dtypes = frame.dtypes();
  std::vector<std::int64_t> out{};
  out.reserve(ordered_names.size());
  for (std::size_t i = 0; i < ordered_names.size(); ++i) {
    const std::string_view name = ordered_names[i];
    const std::string dtype = dtypes.contains(name) ? detail::strip_series_dtype_(dtypes.at(name)) : std::string{};
    if (predicate(name, dtype)) {
      out.push_back(static_cast<std::int64_t>(i));
    }
  }
  return out;
}

// Everything NOT in `names` -- the "give me the rest" complement to route().
[[nodiscard]] inline std::vector<std::int64_t> route_remaining(
    const pluss::table::DataFrame& frame,
    pluss::msharp::RangeView::Of<std::string_view> names) {
  const auto ordered = frame.feataxis().series().features();
  const std::unordered_set<std::string_view> excluded(names.begin(), names.end());
  std::vector<std::int64_t> out{};
  out.reserve(ordered.size());
  for (std::size_t i = 0; i < ordered.size(); ++i) {
    if (!excluded.contains(ordered[i])) {
      out.push_back(static_cast<std::int64_t>(i));
    }
  }
  return out;
}

[[nodiscard]] inline std::vector<std::int64_t> route_remaining(
    const pluss::table::DataFrame& frame,
    std::initializer_list<std::string_view> names) {
  return route_remaining(frame, pluss::msharp::RangeView::Of<std::string_view>(names));
}

// DataFrame-free counterpart to route()/route_remaining() above, for callers
// that already have their own resolved column ordering to index into --
// notably a `DataFrameBatchOptions::feature_names` list that includes
// Note-typed columns via `DataNotePolicy::Factorize`. The frame-derived
// overloads above can't serve that case: they walk
// `frame.feataxis().series().features()`, which only ever sees Series-kind
// columns, so a Note column is invisible to them and every numeric column
// after it gets the wrong index. `feature_batch_from_dataframe` uses
// `options.feature_names` verbatim as its column order whenever it's
// non-empty (`nodej/data.h`), so routing against that same list here is
// always correct, regardless of which columns started out as Note.
[[nodiscard]] inline std::vector<std::int64_t> route(
    const std::vector<std::string>& ordered_names,
    pluss::msharp::RangeView::Of<std::string_view> names) {
  std::unordered_map<std::string_view, std::int64_t> index_of{};
  index_of.reserve(ordered_names.size());
  for (std::size_t i = 0; i < ordered_names.size(); ++i) {
    index_of.emplace(ordered_names[i], static_cast<std::int64_t>(i));
  }
  std::vector<std::int64_t> out{};
  out.reserve(names.size());
  for (const std::string_view name : names) {
    const auto it = index_of.find(name);
    if (it == index_of.end()) {
      ::pluss::throw_error("lymph::route: \"" + std::string(name) + "\" is not in the given name list.");
    }
    out.push_back(it->second);
  }
  return out;
}

[[nodiscard]] inline std::vector<std::int64_t> route(
    const std::vector<std::string>& ordered_names,
    std::initializer_list<std::string_view> names) {
  return route(ordered_names, pluss::msharp::RangeView::Of<std::string_view>(names));
}

[[nodiscard]] inline std::vector<std::int64_t> route_remaining(
    const std::vector<std::string>& ordered_names,
    pluss::msharp::RangeView::Of<std::string_view> names) {
  const std::unordered_set<std::string_view> excluded(names.begin(), names.end());
  std::vector<std::int64_t> out{};
  out.reserve(ordered_names.size());
  for (std::size_t i = 0; i < ordered_names.size(); ++i) {
    if (!excluded.contains(ordered_names[i])) {
      out.push_back(static_cast<std::int64_t>(i));
    }
  }
  return out;
}

[[nodiscard]] inline std::vector<std::int64_t> route_remaining(
    const std::vector<std::string>& ordered_names,
    std::initializer_list<std::string_view> names) {
  return route_remaining(ordered_names, pluss::msharp::RangeView::Of<std::string_view>(names));
}

namespace detail {

[[nodiscard]] inline std::vector<std::string_view> as_string_views_(const std::vector<std::string>& names) {
  return std::vector<std::string_view>(names.begin(), names.end());
}

} // namespace detail

// ColumnRouter lives in columnrouter.h now (LYMPH_REDESIGN_PLAN.md phase 1
// move) -- it depends on route()/route_where()/route_remaining()/
// detail::as_string_views_ above, so it includes this file rather than the
// other way around. Include "pluss/ai/lymph/columnrouter.h" directly, or
// pull it in via the api.h umbrella.

} // namespace pluss::ai::lymph
