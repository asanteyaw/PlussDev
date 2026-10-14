#include "bundle/Locus.h"

#include <algorithm>
#include <limits>
#include <numeric>
#include <unordered_map>
#include <unordered_set>

#include <fmt/format.h>

#include "CodeM/core/Errors.h"
#include "relational/Links.h"
#include "relational/Match.h"
#include "relational/Pull.h"

// Native pull and traversal for Locus. Everything here is composed from three
// Catalog-level pieces: match_keys (which records pair up), relational::expand
// (compose hops without building the tables in between) and Stream::gather /
// Stream::reduce_groups (rebuild or reduce a stream through record lists).

namespace pluss::bundle {

namespace {

using pluss::kindle::Catalog;
using pluss::kindle::CatalogMatch;
using pluss::kindle::GroupReduce;
using pluss::kindle::RaggedLayout;
using pluss::kindle::Stream;
using pluss::kindle::StreamKind;

std::string qualify(std::string_view table, std::string_view feature) {
  PLUSS_CHECK(!table.empty(), "pull table scope cannot be empty");
  PLUSS_CHECK(!feature.empty(), "pull feature cannot be empty");
  return fmt::format("{}.{}", table, feature);
}

void check_pull_policy(const locus::RaggedPolicy& p, std::string_view where) {
  p.validate(where);
  PLUSS_CHECK(p.alignment != locus::RaggedAlignment::Label,
              std::string(where) + ": label alignment is not a pull mode");
  PLUSS_CHECK(p.domain != locus::RaggedDomain::Anchor,
              std::string(where) + ": an anchor-stream domain is not supported for pulls");
  PLUSS_CHECK(p.absent == locus::RaggedAbsent::Null,
              std::string(where) + ": a root record with no related record is always an explicit null; absent must be Null");
  PLUSS_CHECK(p.nulls == locus::RaggedNulls::Skip && p.min_support == 1,
              std::string(where) + ": nulls and min_support do not apply to pulls");
}

GroupReduce reduce_of(relational::PullAggregateOp op) {
  switch (op) {
    case relational::PullAggregateOp::Count: return GroupReduce::Count;
    case relational::PullAggregateOp::Ndistinct: return GroupReduce::Ndistinct;
    case relational::PullAggregateOp::Sum: return GroupReduce::Sum;
    case relational::PullAggregateOp::Mean: return GroupReduce::Mean;
    case relational::PullAggregateOp::Median: return GroupReduce::Median;
    case relational::PullAggregateOp::Min: return GroupReduce::Min;
    case relational::PullAggregateOp::Max: return GroupReduce::Max;
    case relational::PullAggregateOp::Std: return GroupReduce::Std;
    case relational::PullAggregateOp::Var: return GroupReduce::Var;
  }
  return GroupReduce::Count;
}

// epoch nanoseconds of a Timeline stream, plus its validity.
struct TimeColumn {
  std::vector<std::int64_t> ns{};
  std::vector<bool> valid{};
  std::int64_t size() const { return static_cast<std::int64_t>(ns.size()); }
};

TimeColumn load_time(const Catalog& catalog, const std::string& feature, std::string_view table,
                     std::string_view where) {
  PLUSS_CHECK(catalog.has(feature), fmt::format("{}: time field '{}' is missing on table '{}'", where, feature, table));
  const auto stream = catalog.keys(feature);
  PLUSS_CHECK(stream.kind() == StreamKind::Timeline,
              fmt::format("{}: time field '{}' on table '{}' must be a Timeline stream", where, feature, table));
  TimeColumn out;
  out.ns = stream.as_timeline().epoch_nanos().to_vec<std::int64_t>();
  out.valid = pluss::kindle::detail::has_validity_mask(stream)
                  ? pluss::kindle::detail::validity_mask(stream)->to_vec<bool>()
                  : std::vector<bool>(out.ns.size(), true);
  return out;
}

}  // namespace

// ---------------------------------------------------------------------------
// Pull
// ---------------------------------------------------------------------------

LocusPull Locus::pull(std::string_view root) const {
  PLUSS_CHECK(has(root), "Locus::pull: unknown catalog '" + std::string(root) + "'");
  return LocusPull(*this, std::string(root));
}

LocusPull::LocusPull(Locus locus, std::string root) : locus_(std::move(locus)), root_(std::move(root)) {}

LocusPull& LocusPull::options(const locus::PullOptions& options) {
  options_ = options;
  return *this;
}

LocusPull& LocusPull::add(const locus::PullSpec& spec) {
  specs_.push_back(spec);
  return *this;
}

LocusPull& LocusPull::add(std::initializer_list<locus::PullSpec> specs) {
  for (const auto& spec : specs) specs_.push_back(spec);
  return *this;
}

LocusPull& LocusPull::take(pluss::msharp::RangeView::Of<std::string_view> sources) {
  for (const auto source : sources) add(nexus::take(source));
  return *this;
}

LocusPull& LocusPull::take(pluss::msharp::RangeView::Of<std::pair<std::string_view, std::string_view>> fields) {
  for (const auto& [source, alias] : fields) add(nexus::take(source, alias));
  return *this;
}

LocusPull& LocusPull::take(std::string_view table,
                           pluss::msharp::RangeView::Of<std::pair<std::string_view, std::string_view>> fields) {
  for (const auto& [source, alias] : fields) add(nexus::take(qualify(table, source), alias));
  return *this;
}

LocusPull& LocusPull::take(std::string_view source, std::string_view alias) { return add(nexus::take(source, alias)); }
LocusPull& LocusPull::count(std::string_view source, std::string_view alias) { return add(nexus::count(source, alias)); }
LocusPull& LocusPull::ndistinct(std::string_view source, std::string_view alias) { return add(nexus::ndistinct(source, alias)); }
LocusPull& LocusPull::sum(std::string_view source, std::string_view alias) { return add(nexus::sum(source, alias)); }
LocusPull& LocusPull::mean(std::string_view source, std::string_view alias) { return add(nexus::mean(source, alias)); }
LocusPull& LocusPull::median(std::string_view source, std::string_view alias) { return add(nexus::median(source, alias)); }
LocusPull& LocusPull::min(std::string_view source, std::string_view alias) { return add(nexus::min(source, alias)); }
LocusPull& LocusPull::max(std::string_view source, std::string_view alias) { return add(nexus::max(source, alias)); }
LocusPull& LocusPull::std(std::string_view source, std::string_view alias) { return add(nexus::std(source, alias)); }
LocusPull& LocusPull::var(std::string_view source, std::string_view alias) { return add(nexus::var(source, alias)); }

LocusPullAgg LocusPull::agg(std::string_view table) {
  PLUSS_CHECK(!table.empty(), "LocusPull::agg: table scope cannot be empty");
  return LocusPullAgg(*this, std::string(table));
}

const std::string& LocusPull::root() const noexcept { return root_; }
const std::vector<locus::PullSpec>& LocusPull::specs() const noexcept { return specs_; }

LocusPullAgg::LocusPullAgg(LocusPull& pull, std::string table) : pull_(&pull), table_(std::move(table)) {}

#define LOCUS_PULL_AGG(NAME)                                                                          \
  LocusPullAgg& LocusPullAgg::NAME(std::string_view feature, std::string_view alias) {                \
    pull_->add(nexus::NAME(qualify(table_, feature), alias));                                         \
    return *this;                                                                                     \
  }                                                                                                   \
  LocusPullAgg& LocusPullAgg::NAME(pluss::msharp::RangeView::Of<std::pair<std::string_view, std::string_view>> fields) { \
    for (const auto& [feature, alias] : fields) NAME(feature, alias);                                \
    return *this;                                                                                     \
  }
LOCUS_PULL_AGG(count)
LOCUS_PULL_AGG(ndistinct)
LOCUS_PULL_AGG(sum)
LOCUS_PULL_AGG(mean)
LOCUS_PULL_AGG(median)
LOCUS_PULL_AGG(min)
LOCUS_PULL_AGG(max)
LOCUS_PULL_AGG(std)
LOCUS_PULL_AGG(var)
#undef LOCUS_PULL_AGG

LocusPull& LocusPullAgg::end() const { return *pull_; }

Catalog LocusPull::collect() const {
  constexpr std::string_view where = "Locus::pull";
  const Locus& loc = locus_;
  const auto& policy = options_.ragged;
  PLUSS_CHECK(loc.has(root_), fmt::format("{}: unknown catalog '{}'", where, root_));
  PLUSS_CHECK(!specs_.empty(), fmt::format("{}: at least one field is required", where));
  check_pull_policy(policy, where);
  const auto fields = relational::parse_pull_fields(root_, specs_, where);

  // Each table is read once: its catalog and how its streams sit under the policy.
  struct Table {
    Catalog catalog{};
    RaggedLayout layout{};
  };
  std::unordered_map<std::string, Table> tables;
  const auto table = [&](const std::string& name) -> Table& {
    auto it = tables.find(name);
    if (it == tables.end()) {
      PLUSS_CHECK(loc.has(name), fmt::format("{}: unknown catalog '{}'", where, name));
      auto catalog = loc.catalog(name);
      auto layout = RaggedLayout::of(catalog, policy, where);
      it = tables.emplace(name, Table{std::move(catalog), std::move(layout)}).first;
    }
    return it->second;
  };

  for (const auto& f : fields) {
    PLUSS_CHECK(table(f.table).catalog.has(f.feature),
                fmt::format("{}: unknown feature '{}' on table '{}'", where, f.feature, f.table));
  }
  const std::int64_t n = table(root_).layout.domain();
  std::vector<std::int64_t> identity(static_cast<std::size_t>(n));
  std::iota(identity.begin(), identity.end(), std::int64_t{0});
  const bool trim = policy.materialize == locus::RaggedMaterialize::Preserve;

  // A hop of a path as the shared resolver sees it; its matches are computed once.
  std::unordered_map<std::string, CatalogMatch> hops;
  const auto hop = [&](const locus::JoinPathStep& step) -> const CatalogMatch& {
    const auto key = step.from_table + "\n" + step.to_table + "\n" + step.relationship_name;
    auto it = hops.find(key);
    if (it == hops.end()) {
      locus::JoinSpec spec;
      spec.how = locus::JoinHow::Inner;
      spec.relationship_name = step.relationship_name;
      const auto resolved =
          relational::resolve_join(loc.relationships(), step.from_table, step.to_table, spec, where);
      it = hops.emplace(key, table(step.from_table).catalog.match_keys(
                                 table(step.to_table).catalog, resolved.left_keys, resolved.right_keys,
                                 locus::JoinHow::Inner, resolved.validate, policy)).first;
    }
    return it->second;
  };
  const auto oriented_cardinality = [&](const locus::JoinPathStep& step) {
    locus::JoinSpec spec;
    spec.relationship_name = step.relationship_name;
    return *relational::resolve_join(loc.relationships(), step.from_table, step.to_table, spec, where).validate;
  };
  const auto path_to = [&](const std::string& target) {
    auto path = loc.path(root_, target);
    PLUSS_CHECK(path.has_value(), fmt::format("{}: no relationship path from '{}' to '{}'", where, root_, target));
    return *path;
  };

  // Plain fields: follow many-to-one / one-to-one links, composing the matches
  // into one root -> leaf record list per table.
  std::unordered_map<std::string, std::vector<std::int64_t>> composite;
  const auto composite_for = [&](const relational::PullField& f) -> const std::vector<std::int64_t>& {
    auto it = composite.find(f.table);
    if (it != composite.end()) return it->second;
    std::vector<std::int64_t> idx = identity;
    for (const auto& step : path_to(f.table).steps) {
      const auto card = oriented_cardinality(step);
      PLUSS_CHECK(card == locus::RelationshipCardinality::ManyToOne || card == locus::RelationshipCardinality::OneToOne,
                  fmt::format("{}: requested field '{}' requires expanding relationship '{}' from '{}' to '{}'. "
                              "pull preserves root grain and cannot traverse one-to-many or many-to-many paths",
                              where, f.token, step.relationship_name, step.from_table, step.to_table));
      const auto& m = hop(step);
      std::vector<std::int64_t> partner(static_cast<std::size_t>(m.left.domain()), -1);
      for (std::size_t p = 0; p < m.pairs.left.size(); ++p) {
        if (m.pairs.left[p] >= 0 && m.pairs.right[p] >= 0) partner[static_cast<std::size_t>(m.pairs.left[p])] = m.pairs.right[p];
      }
      for (auto& v : idx) v = v >= 0 ? partner[static_cast<std::size_t>(v)] : -1;
    }
    return composite.emplace(f.table, std::move(idx)).first->second;
  };

  // Aggregates: expand root -> related records (any cardinality), grouped by root.
  struct Expanded {
    std::vector<std::int64_t> root{};
    std::vector<std::int64_t> leaf{};
  };
  std::unordered_map<std::string, Expanded> expanded;
  const auto expanded_for = [&](const std::string& target) -> const Expanded& {
    auto it = expanded.find(target);
    if (it != expanded.end()) return it->second;
    std::vector<std::int64_t> root_list = identity, cur_list = identity;
    for (const auto& step : path_to(target).steps) {
      const auto& m = hop(step);
      auto e = relational::expand(root_list, cur_list, m.pairs, m.left.domain(), false);
      root_list = std::move(e.root);
      cur_list = std::move(e.leaf);
    }
    return expanded.emplace(target, Expanded{std::move(root_list), std::move(cur_list)}).first->second;
  };

  const bool has_aggregate = std::any_of(fields.begin(), fields.end(), [](const auto& f) { return f.aggregate.has_value(); });
  if (has_aggregate && options_.require_primary_key) {
    const auto meta = loc.table_meta(root_);
    PLUSS_CHECK(meta.has_value() && !meta->primary_key.empty(),
                fmt::format("{}: aggregating fields group related records under a root record, so root table '{}' "
                            "needs a declared primary key (Locus::meta), or turn off PullOptions::require_primary_key",
                            where, root_));
    const auto& root_catalog = table(root_).catalog;
    const auto audit = root_catalog.audit_keys(root_catalog, meta->primary_key, meta->primary_key, policy);
    PLUSS_CHECK(audit.left_unique && audit.left_unkeyed == 0,
                fmt::format("{}: root table '{}' must have a complete, unique declared primary key before "
                            "aggregate pulls", where, root_));
  }

  std::vector<std::pair<std::string, Stream>> out;
  out.reserve(fields.size());
  std::unordered_set<std::string> aliases;
  for (const auto& f : fields) {
    const auto alias = relational::pull_output_alias(root_, f);
    PLUSS_CHECK(aliases.insert(alias).second, fmt::format("{}: output alias collision for '{}'", where, alias));
    auto& tb = table(f.table);
    const auto leaf = tb.catalog.keys(f.feature);
    const auto leaf_shift = tb.layout.shift_of(f.feature);

    if (!f.aggregate.has_value()) {
      const auto& idx = f.table == root_ ? identity : composite_for(f);
      out.emplace_back(alias, leaf.gather(idx, leaf_shift, trim));
      continue;
    }

    const auto aggregate = relational::parse_pull_aggregate(*f.aggregate);
    PLUSS_CHECK(aggregate.has_value(), fmt::format("{}: unsupported aggregate token '{}'", where, *f.aggregate));
    const auto& ex = expanded_for(f.table);
    std::vector<std::int64_t> roots = ex.root, leaves = ex.leaf;
    std::vector<std::uint8_t> root_ok(static_cast<std::size_t>(n), 1);

    if (aggregate->window_value.has_value()) {
      // Keep related records whose time lies in [anchor - window, anchor]; the
      // anchor is the latest related time of the root record, or the root's own
      // time for "_root" tokens.
      const auto child_meta = loc.table_meta(f.table);
      PLUSS_CHECK(child_meta.has_value() && child_meta->time_field.has_value(),
                  fmt::format("{}: window aggregate '{}' requires a declared time_field on table '{}'", where,
                              f.token, f.table));
      const auto child_time = load_time(tb.catalog, *child_meta->time_field, f.table, where);
      const auto child_shift = tb.layout.shift_of(*child_meta->time_field);
      const auto window_seconds = relational::pull_window_seconds(*aggregate, where);
      PLUSS_CHECK(window_seconds <= std::numeric_limits<std::int64_t>::max() / 1'000'000'000LL,
                  fmt::format("{}: pull window length exceeds the supported nanosecond range", where));
      const std::int64_t window_ns = window_seconds * 1'000'000'000LL;

      const auto time_of = [&](std::int64_t record, std::int64_t& out_ns) {
        const auto p = record + child_shift;
        if (record < 0 || p < 0 || p >= child_time.size() || !child_time.valid[static_cast<std::size_t>(p)]) return false;
        out_ns = child_time.ns[static_cast<std::size_t>(p)];
        return true;
      };

      std::vector<std::int64_t> anchor(static_cast<std::size_t>(n), 0);
      std::vector<std::uint8_t> has_anchor(static_cast<std::size_t>(n), 0);
      if (aggregate->anchor == relational::PullAggregate::Anchor::Root) {
        const auto root_meta = loc.table_meta(root_);
        PLUSS_CHECK(root_meta.has_value() && root_meta->time_field.has_value(),
                    fmt::format("{}: root-anchored window aggregate '{}' requires a declared time_field on root "
                                "table '{}'", where, f.token, root_));
        const auto root_time = load_time(table(root_).catalog, *root_meta->time_field, root_, where);
        const auto root_shift = table(root_).layout.shift_of(*root_meta->time_field);
        for (std::int64_t i = 0; i < n; ++i) {
          const auto p = i + root_shift;
          if (p >= 0 && p < root_time.size() && root_time.valid[static_cast<std::size_t>(p)]) {
            anchor[static_cast<std::size_t>(i)] = root_time.ns[static_cast<std::size_t>(p)];
            has_anchor[static_cast<std::size_t>(i)] = 1;
          }
        }
      } else {
        for (std::size_t k = 0; k < roots.size(); ++k) {
          std::int64_t t = 0;
          if (!time_of(leaves[k], t)) continue;
          const auto r = static_cast<std::size_t>(roots[k]);
          if (!has_anchor[r] || t > anchor[r]) {
            anchor[r] = t;
            has_anchor[r] = 1;
          }
        }
      }
      std::vector<std::int64_t> kept_roots, kept_leaves;
      for (std::size_t k = 0; k < roots.size(); ++k) {
        const auto r = static_cast<std::size_t>(roots[k]);
        std::int64_t t = 0;
        if (!has_anchor[r] || !time_of(leaves[k], t)) continue;
        const auto lower = anchor[r] < std::numeric_limits<std::int64_t>::min() + window_ns
                               ? std::numeric_limits<std::int64_t>::min()
                               : anchor[r] - window_ns;
        if (t >= lower && t <= anchor[r]) {
          kept_roots.push_back(roots[k]);
          kept_leaves.push_back(leaves[k]);
        }
      }
      roots = std::move(kept_roots);
      leaves = std::move(kept_leaves);
      if (aggregate->anchor == relational::PullAggregate::Anchor::Root) {
        for (std::int64_t i = 0; i < n; ++i) root_ok[static_cast<std::size_t>(i)] = has_anchor[static_cast<std::size_t>(i)];
      }
    }

    std::vector<std::int64_t> offsets(static_cast<std::size_t>(n) + 1, 0);
    for (const auto r : roots) ++offsets[static_cast<std::size_t>(r) + 1];
    for (std::size_t i = 1; i < offsets.size(); ++i) offsets[i] += offsets[i - 1];

    auto reduced = leaf.reduce_groups(leaves, leaf_shift, offsets, reduce_of(aggregate->op));
    if (std::find(root_ok.begin(), root_ok.end(), std::uint8_t{0}) != root_ok.end()) {
      // A root-anchored window needs a root time; without one the value is unknown.
      std::vector<double> mask(static_cast<std::size_t>(n), 1.0);
      if (pluss::kindle::detail::has_validity_mask(reduced)) {
        const auto existing = pluss::kindle::detail::validity_mask(reduced)->to_vec<bool>();
        for (std::size_t i = 0; i < mask.size(); ++i) mask[i] = existing[i] ? 1.0 : 0.0;
      }
      for (std::size_t i = 0; i < mask.size(); ++i) {
        if (!root_ok[i]) mask[i] = 0.0;
      }
      reduced = pluss::kindle::detail::with_validity_mask(
          reduced, pluss::make_series(mask, {n}).to(pluss::codem::ScalarType::Bool));
    }
    out.emplace_back(alias, std::move(reduced));
  }
  return Catalog(std::move(out));
}

// ---------------------------------------------------------------------------
// Traversal
// ---------------------------------------------------------------------------

LocusTraverse Locus::from(std::string_view table) const {
  PLUSS_CHECK(has(table), "Locus::from: unknown catalog '" + std::string(table) + "'");
  return LocusTraverse(*this, std::string(table));
}

LocusTraverse::LocusTraverse(Locus locus, std::string root)
    : locus_(std::move(locus)),
      plan_(locus::JoinPlan{.root_table = root, .result_name = root, .steps = {}}),
      anchor_(std::move(root)) {}

LocusTraverse::LocusTraverse(Locus locus, locus::JoinPlan plan, std::string anchor, locus::JoinOptions options,
                             std::optional<std::vector<std::string>> picked,
                             std::optional<std::vector<std::string>> dropped)
    : locus_(std::move(locus)),
      plan_(std::move(plan)),
      anchor_(std::move(anchor)),
      options_(std::move(options)),
      picked_(std::move(picked)),
      dropped_(std::move(dropped)) {}

LocusTraverse LocusTraverse::options(const locus::JoinOptions& options) const {
  return LocusTraverse(locus_, plan_, anchor_, options, picked_, dropped_);
}

LocusTraverse LocusTraverse::join(std::string_view right) const { return join(right, locus::JoinSpec{}); }

LocusTraverse LocusTraverse::join(std::string_view right, const locus::JoinSpec& spec) const {
  constexpr std::string_view where = "LocusTraverse::join";
  PLUSS_CHECK(locus_.has(anchor_), fmt::format("{}: unknown catalog '{}'", where, anchor_));
  PLUSS_CHECK(locus_.has(right), fmt::format("{}: unknown catalog '{}'", where, right));

  // With no keys and no named link, the single direct link between the two
  // tables supplies them; several links is an error, never a guess.
  auto resolved = spec;
  if (resolved.how != locus::JoinHow::Cross && resolved.keys.empty() && !resolved.relationship_name.has_value()) {
    resolved.relationship_name = relational::require_direct_relationship_name(locus_.relationships(), anchor_, right, where);
  }
  if (resolved.right_suffix.empty()) resolved.right_suffix = "_" + std::string(right);

  auto next = plan_;
  next.steps.push_back(locus::JoinPlanStep{.left_table = anchor_, .right_table = std::string(right), .spec = std::move(resolved)});
  next.result_name = relational::stable_plan_result_name(next.root_table, next.steps);
  return LocusTraverse(locus_, std::move(next), std::string(right), options_, picked_, dropped_);
}

LocusTraverse LocusTraverse::via(std::string_view to, locus::JoinHow how) const {
  const auto sub = locus_.plan(anchor_, to, how);
  auto next = plan_;
  for (const auto& step : sub.steps) next.steps.push_back(step);
  next.result_name = relational::stable_plan_result_name(next.root_table, next.steps);
  return LocusTraverse(locus_, std::move(next), std::string(to), options_, picked_, dropped_);
}

namespace {

std::vector<std::string> normalized_names(const std::vector<std::string>& names, std::string_view where) {
  std::unordered_set<std::string> seen;
  for (const auto& name : names) {
    PLUSS_CHECK(!name.empty(), fmt::format("{}: feature names cannot be empty", where));
    PLUSS_CHECK(seen.insert(name).second, fmt::format("{}: duplicate feature '{}'", where, name));
  }
  return names;
}

}  // namespace

LocusTraverse LocusTraverse::pick(const std::vector<std::string>& features) const {
  return LocusTraverse(locus_, plan_, anchor_, options_, normalized_names(features, "LocusTraverse::pick"), dropped_);
}

LocusTraverse LocusTraverse::pick(std::initializer_list<std::string_view> features) const {
  return pick(std::vector<std::string>(features.begin(), features.end()));
}

LocusTraverse LocusTraverse::drop(const std::vector<std::string>& features) const {
  return LocusTraverse(locus_, plan_, anchor_, options_, picked_, normalized_names(features, "LocusTraverse::drop"));
}

LocusTraverse LocusTraverse::drop(std::initializer_list<std::string_view> features) const {
  return drop(std::vector<std::string>(features.begin(), features.end()));
}

Catalog LocusTraverse::collect() const {
  auto out = locus_.materialize(plan_, options_);
  if (picked_.has_value()) {
    const std::vector<std::string_view> views(picked_->begin(), picked_->end());
    out = out.keys(std::span<const std::string_view>(views.data(), views.size()));
  }
  if (dropped_.has_value()) out = out.drop(*dropped_);
  return out;
}

}  // namespace pluss::bundle
