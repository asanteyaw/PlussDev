#include "bundle/Locus.h"

#include <algorithm>
#include <cctype>
#include <deque>
#include <memory>
#include <mutex>
#include <filesystem>
#include <iostream>
#include <ostream>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

#include <fmt/format.h>

#include "CodeM/core/Errors.h"
#include "MSharp/Parallel.h"
#include "bundle/WorkspaceStore.h"
#include "relational/Codec.h"
#include "relational/Links.h"
#include "table/io/SnapshotCsv.h"
#include "table/DataFrame.h"

namespace pluss::bundle {

namespace detail {

struct FrameCache {
  struct Entry {
    pluss::kindle::Catalog holder;  // keeps the identity key alive
    pluss::table::DataFrame frame;
  };
  std::mutex mutex;
  std::unordered_map<std::string, Entry> entries;  // key: identity|policy
  std::int64_t hits{0}, misses{0};
};

class LocusImpl final : public pluss::msharp::RefCounted {
public:
  LocusImpl() = default;

  std::vector<std::string> keys_{};
  std::vector<pluss::kindle::Catalog> catalogs_{};
  std::unordered_map<std::string, std::size_t> name_to_pos_{};
  std::vector<locus::Relationship> relationships_{};
  std::unordered_map<std::string, locus::TableMeta> table_meta_{};
  std::unordered_map<std::string, locus::JoinPlan> plans_{};
  pluss::kindle::TabelizePolicy policy_{};
  // Adjacency, paths and plans over relationships_ (shared with Nexus).
  relational::LinkCache link_cache_{};

  // Aligned DataFrame workspace, rebuilt on demand; dropped on any change.
  mutable std::optional<Nexus> nexus_cache_{};
  // Aligned frame per catalog, shared by every clone of this Locus: editing one
  // table (or only the links) re-pads just what changed.
  std::shared_ptr<FrameCache> frame_cache_{std::make_shared<FrameCache>()};
  mutable std::string nexus_cache_key_{};
};

} // namespace detail

namespace {

using pluss::kindle::Catalog;
using pluss::kindle::TabelizePolicy;
using pluss::table::DataFrame;
using locus::JoinHow;
using locus::JoinPath;
using locus::JoinPathStep;
using locus::JoinSpec;
using locus::LinkAudit;
using locus::Relationship;
using locus::RelationshipCardinality;
using locus::RelationshipValidationStatus;
using Views = pluss::msharp::RangeView::Of<std::string_view>;

// The cardinality as seen from the other end of the link.
std::int64_t normalize_index(std::int64_t index, std::int64_t size, std::string_view where) {
  const auto idx = index < 0 ? index + size : index;
  PLUSS_CHECK(idx >= 0 && idx < size,
              fmt::format("{}: index {} out of range for {} tables", where, index, size));
  return idx;
}

std::vector<std::string_view> as_views(const std::vector<std::string>& names) {
  return std::vector<std::string_view>(names.begin(), names.end());
}

pluss::msharp::intrusive_ptr<detail::LocusImpl> clone_impl(const detail::LocusImpl& src) {
  auto out = pluss::msharp::make_intrusive<detail::LocusImpl>();
  out->keys_ = src.keys_;
  out->catalogs_ = src.catalogs_;
  out->name_to_pos_ = src.name_to_pos_;
  out->relationships_ = src.relationships_;
  out->table_meta_ = src.table_meta_;
  out->plans_ = src.plans_;
  out->policy_ = src.policy_;
  out->frame_cache_ = src.frame_cache_;
  return out;
}

bool plan_uses(const locus::JoinPlan& plan, const std::unordered_map<std::string, std::size_t>& kept) {
  if (kept.count(plan.root_table) == 0) return false;
  for (const auto& step : plan.steps) {
    if (kept.count(step.left_table) == 0 || kept.count(step.right_table) == 0) return false;
  }
  return true;
}

// New impl holding `names` (in that order); links survive when both ends do.
pluss::msharp::intrusive_ptr<detail::LocusImpl> subset_impl(const detail::LocusImpl& src,
                                                            const std::vector<std::string>& names) {
  auto out = pluss::msharp::make_intrusive<detail::LocusImpl>();
  for (const auto& name : names) {
    const auto it = src.name_to_pos_.find(name);
    PLUSS_CHECK(it != src.name_to_pos_.end(), "Locus::select: unknown catalog '" + name + "'");
    PLUSS_CHECK(out->name_to_pos_.find(name) == out->name_to_pos_.end(),
                "Locus::select: duplicate catalog '" + name + "'");
    out->name_to_pos_.emplace(name, out->keys_.size());
    out->keys_.push_back(name);
    out->catalogs_.push_back(src.catalogs_[it->second]);
  }
  for (const auto& rel : src.relationships_) {
    if (out->name_to_pos_.count(rel.left_table) != 0 && out->name_to_pos_.count(rel.right_table) != 0) {
      out->relationships_.push_back(rel);
    }
  }
  for (const auto& [name, meta] : src.table_meta_) {
    if (out->name_to_pos_.count(name) != 0) out->table_meta_.emplace(name, meta);
  }
  for (const auto& [name, plan] : src.plans_) {
    if (plan_uses(plan, out->name_to_pos_)) out->plans_.emplace(name, plan);
  }
  out->policy_ = src.policy_;
  return out;
}

bool keys_unique(const DataFrame& frame, const std::vector<std::string>& keys) {
  const auto views = as_views(keys);
  const auto selected = frame.features(Views(views));
  return selected.drop_duplicates().ncases() == selected.ncases();
}

} // namespace

TabelizePolicy Locus::default_policy() {
  TabelizePolicy policy;
  policy.align = pluss::kindle::Align::Outer;
  policy.align_by = pluss::kindle::AlignBy::Label;
  policy.fill = pluss::kindle::FillPolicy::Null;
  return policy;
}

Locus::Locus() = default;

Locus::Locus(Catalogs catalogs) : impl_(pluss::msharp::make_intrusive<detail::LocusImpl>()) {
  auto& impl = *impl_;
  impl.policy_ = default_policy();
  impl.keys_.reserve(catalogs.size());
  impl.catalogs_.reserve(catalogs.size());
  for (auto& [name, value] : catalogs) {
    PLUSS_CHECK(!name.empty(), "Locus: catalog name cannot be empty");
    PLUSS_CHECK(value.defined(), "Locus: catalog '" + name + "' is undefined");
    PLUSS_CHECK(impl.name_to_pos_.find(name) == impl.name_to_pos_.end(),
                "Locus: duplicate catalog name '" + name + "'");
    impl.name_to_pos_.emplace(name, impl.keys_.size());
    impl.keys_.push_back(std::move(name));
    impl.catalogs_.push_back(std::move(value));
  }
}

Locus::Locus(std::initializer_list<std::pair<std::string, pluss::kindle::Catalog>> catalogs)
    : Locus(Catalogs(catalogs.begin(), catalogs.end())) {}

Locus::Locus(const Locus& other) = default;
Locus::Locus(Locus&& other) noexcept = default;
Locus& Locus::operator=(const Locus& other) = default;
Locus& Locus::operator=(Locus&& other) noexcept = default;
Locus::~Locus() = default;

Locus::Locus(pluss::msharp::intrusive_ptr<detail::LocusImpl> impl) : impl_(std::move(impl)) {}

bool Locus::defined() const noexcept { return static_cast<bool>(impl_); }
bool Locus::unique() const noexcept { return impl_.unique(); }
bool Locus::empty() const { return !defined() || ntables() == 0; }

const detail::LocusImpl& Locus::impl_ref_() const {
  PLUSS_CHECK(defined(), "Locus: undefined handle");
  return *impl_;
}

detail::LocusImpl& Locus::impl_mut_() {
  PLUSS_CHECK(defined(), "Locus: undefined handle");
  if (!impl_.unique()) {
    impl_ = clone_impl(*impl_);
  }
  impl_->nexus_cache_.reset();
  impl_->link_cache_.clear();
  return *impl_;
}

Locus Locus::clone_() const { return Locus(clone_impl(impl_ref_())); }

std::int64_t Locus::size() const { return ntables(); }
std::int64_t Locus::ntables() const { return static_cast<std::int64_t>(impl_ref_().keys_.size()); }
std::int64_t Locus::nrelationships() const {
  return static_cast<std::int64_t>(impl_ref_().relationships_.size());
}
std::pair<std::int64_t, std::int64_t> Locus::shape() const { return {ntables(), nrelationships()}; }

const std::vector<std::string>& Locus::keys() const { return impl_ref_().keys_; }
const std::vector<std::string>& Locus::names() const { return keys(); }
const std::vector<pluss::kindle::Catalog>& Locus::catalogs() const { return impl_ref_().catalogs_; }

bool Locus::has(std::string_view name) const {
  const auto& src = impl_ref_();
  return src.name_to_pos_.find(std::string(name)) != src.name_to_pos_.end();
}

pluss::kindle::Catalog Locus::catalog(std::string_view name) const {
  const auto& src = impl_ref_();
  const auto it = src.name_to_pos_.find(std::string(name));
  PLUSS_CHECK(it != src.name_to_pos_.end(), "Locus::catalog: unknown catalog '" + std::string(name) + "'");
  return src.catalogs_[it->second];
}

pluss::kindle::Catalog Locus::catalog(std::int64_t index) const {
  const auto& src = impl_ref_();
  const auto idx = normalize_index(index, ntables(), "Locus::catalog");
  return src.catalogs_[static_cast<std::size_t>(idx)];
}

pluss::kindle::Catalog Locus::dloc(std::int64_t index) const { return catalog(index); }

std::optional<pluss::kindle::Catalog> Locus::get(std::string_view name) const {
  const auto& src = impl_ref_();
  const auto it = src.name_to_pos_.find(std::string(name));
  if (it == src.name_to_pos_.end()) return std::nullopt;
  return src.catalogs_[it->second];
}

Locus Locus::head(std::int64_t n) const {
  const auto& src = impl_ref_();
  const auto count = std::clamp<std::int64_t>(n, 0, ntables());
  return Locus(subset_impl(src, std::vector<std::string>(src.keys_.begin(), src.keys_.begin() + count)));
}

Locus Locus::tail(std::int64_t n) const {
  const auto& src = impl_ref_();
  const auto count = std::clamp<std::int64_t>(n, 0, ntables());
  return Locus(subset_impl(src, std::vector<std::string>(src.keys_.end() - count, src.keys_.end())));
}

Locus Locus::select(std::initializer_list<std::string_view> names) const {
  std::vector<std::string> owned;
  owned.reserve(names.size());
  for (const auto name : names) owned.emplace_back(name);
  return select(owned);
}

Locus Locus::select(const std::vector<std::string>& names) const {
  return Locus(subset_impl(impl_ref_(), names));
}

Locus& Locus::with_(std::string_view name, const pluss::kindle::Catalog& value) {
  PLUSS_CHECK(!name.empty(), "Locus::with: catalog name cannot be empty");
  PLUSS_CHECK(value.defined(), "Locus::with: catalog is undefined");
  auto& impl = impl_mut_();
  const std::string key(name);
  const auto it = impl.name_to_pos_.find(key);
  if (it != impl.name_to_pos_.end()) {
    impl.catalogs_[it->second] = value;
  } else {
    impl.name_to_pos_.emplace(key, impl.keys_.size());
    impl.keys_.push_back(key);
    impl.catalogs_.push_back(value);
  }
  return *this;
}

Locus Locus::with(std::string_view name, const pluss::kindle::Catalog& value) const {
  auto out = clone_();
  out.with_(name, value);
  return out;
}

Locus& Locus::without_(std::string_view name) {
  PLUSS_CHECK(has(name), "Locus::without: unknown catalog '" + std::string(name) + "'");
  std::vector<std::string> keep;
  for (const auto& key : impl_ref_().keys_) {
    if (key != name) keep.push_back(key);
  }
  impl_ = subset_impl(impl_ref_(), keep);
  return *this;
}

Locus Locus::without(std::string_view name) const {
  auto out = *this;
  out.without_(name);
  return out;
}

const std::vector<locus::Relationship>& Locus::relationships() const { return impl_ref_().relationships_; }

bool Locus::has_relationship(std::string_view name) const {
  return relationship(name).has_value();
}

std::optional<locus::Relationship> Locus::relationship(std::string_view name) const {
  for (const auto& rel : impl_ref_().relationships_) {
    if (rel.name == name) return rel;
  }
  return std::nullopt;
}

Locus& Locus::link_(const locus::Relationship& rel) {
  auto& impl = impl_mut_();
  PLUSS_CHECK(impl.name_to_pos_.count(rel.left_table) != 0,
              "Locus::link: unknown catalog '" + rel.left_table + "'");
  PLUSS_CHECK(impl.name_to_pos_.count(rel.right_table) != 0,
              "Locus::link: unknown catalog '" + rel.right_table + "'");
  PLUSS_CHECK(!rel.keys.empty(), "Locus::link: a link needs at least one key pair");
  const auto& left = impl.catalogs_[impl.name_to_pos_.at(rel.left_table)];
  const auto& right = impl.catalogs_[impl.name_to_pos_.at(rel.right_table)];
  for (const auto& [lk, rk] : rel.keys) {
    PLUSS_CHECK(left.has(lk), fmt::format("Locus::link: catalog '{}' has no key '{}'", rel.left_table, lk));
    PLUSS_CHECK(right.has(rk), fmt::format("Locus::link: catalog '{}' has no key '{}'", rel.right_table, rk));
  }

  auto stored = rel;
  const auto taken = [&](const std::string& name) {
    return std::any_of(impl.relationships_.begin(), impl.relationships_.end(),
                       [&](const Relationship& r) { return r.name == name; });
  };
  if (stored.name.empty()) {
    const auto base = rel.left_table + "->" + rel.right_table;
    stored.name = base;
    for (int n = 2; taken(stored.name); ++n) stored.name = base + "#" + std::to_string(n);
  } else {
    PLUSS_CHECK(!taken(stored.name), "Locus::link: duplicate relationship name '" + stored.name + "'");
  }
  impl.relationships_.push_back(std::move(stored));
  return *this;
}

Locus Locus::link(const locus::Relationship& rel) const {
  auto out = clone_();
  out.link_(rel);
  return out;
}

Locus Locus::link(std::string_view left,
                  std::string_view right,
                  std::string_view key,
                  locus::RelationshipCardinality cardinality) const {
  return link(left, key, right, key, cardinality);
}

Locus Locus::link(std::string_view left,
                  std::string_view left_key,
                  std::string_view right,
                  std::string_view right_key,
                  locus::RelationshipCardinality cardinality) const {
  Relationship rel;
  rel.left_table = std::string(left);
  rel.right_table = std::string(right);
  rel.keys.emplace_back(std::string(left_key), std::string(right_key));
  rel.cardinality = cardinality;
  return link(rel);
}

Locus& Locus::unlink_(std::string_view name) {
  auto& impl = impl_mut_();
  const auto it = std::find_if(impl.relationships_.begin(), impl.relationships_.end(),
                               [&](const Relationship& r) { return r.name == name; });
  PLUSS_CHECK(it != impl.relationships_.end(),
              "Locus::unlink: unknown relationship '" + std::string(name) + "'");
  impl.relationships_.erase(it);
  return *this;
}

Locus Locus::unlink(std::string_view name) const {
  auto out = clone_();
  out.unlink_(name);
  return out;
}

std::vector<std::string> Locus::neighbors(std::string_view table) const {
  const auto& src = impl_ref_();
  PLUSS_CHECK(has(table), "Locus::neighbors: unknown catalog '" + std::string(table) + "'");
  std::vector<std::string> out;
  const auto add = [&](const std::string& name) {
    if (std::find(out.begin(), out.end(), name) == out.end()) out.push_back(name);
  };
  for (const auto& rel : src.relationships_) {
    if (rel.left_table == table) add(rel.right_table);
    else if (rel.right_table == table) add(rel.left_table);
  }
  return out;
}

std::optional<locus::JoinPath> Locus::path(std::string_view from, std::string_view to) const {
  const auto& src = impl_ref_();
  PLUSS_CHECK(has(from), "Locus::path: unknown catalog '" + std::string(from) + "'");
  PLUSS_CHECK(has(to), "Locus::path: unknown catalog '" + std::string(to) + "'");
  return src.link_cache_.try_path(src.keys_, src.relationships_, from, to);
}

namespace {

LinkAudit audit_link(const detail::LocusImpl& src, const Relationship& rel, const locus::JoinOptions& options) {
  LinkAudit out;
  out.relationship = rel.name;
  out.declared = rel.cardinality;

  const auto invalid = [&](std::string message) {
    out.status = RelationshipValidationStatus::Invalid;
    out.message = std::move(message);
    return out;
  };

  const auto left_it = src.name_to_pos_.find(rel.left_table);
  const auto right_it = src.name_to_pos_.find(rel.right_table);
  if (left_it == src.name_to_pos_.end()) return invalid("unknown catalog '" + rel.left_table + "'");
  if (right_it == src.name_to_pos_.end()) return invalid("unknown catalog '" + rel.right_table + "'");
  const auto& left = src.catalogs_[left_it->second];
  const auto& right = src.catalogs_[right_it->second];

  std::vector<std::string> lk, rk;
  for (const auto& [a, b] : rel.keys) {
    lk.push_back(a);
    rk.push_back(b);
  }
  for (const auto& k : lk) {
    if (!left.has(k)) return invalid(fmt::format("catalog '{}' has no key '{}'", rel.left_table, k));
  }
  for (const auto& k : rk) {
    if (!right.has(k)) return invalid(fmt::format("catalog '{}' has no key '{}'", rel.right_table, k));
  }

  pluss::kindle::CatalogKeyAudit measured;
  try {
    measured = left.audit_keys(right, lk, rk, options.ragged);
  } catch (const std::exception& e) {
    return invalid(e.what());
  }
  out.left_rows = measured.left_records;
  out.right_rows = measured.right_records;
  out.left_unique = measured.left_unique;
  out.right_unique = measured.right_unique;
  out.left_orphans = measured.left_orphans;
  out.right_orphans = measured.right_orphans;

  std::string problem;
  switch (rel.cardinality) {
    case RelationshipCardinality::OneToOne:
      if (!out.left_unique || !out.right_unique) {
        problem = "declared one-to-one but keys repeat on " +
                  std::string(!out.left_unique && !out.right_unique ? "both sides"
                                                                     : (!out.left_unique ? "the left side"
                                                                                         : "the right side"));
      }
      break;
    case RelationshipCardinality::OneToMany:
      if (!out.left_unique) problem = "declared one-to-many but left keys repeat";
      break;
    case RelationshipCardinality::ManyToOne:
      if (!out.right_unique) problem = "declared many-to-one but right keys repeat";
      break;
    case RelationshipCardinality::ManyToMany:
      break;
  }
  if (!problem.empty()) return invalid(std::move(problem));

  // left_optional / right_optional: rows on that side may lack a counterpart.
  std::string warning;
  if (out.left_orphans > 0 && !rel.left_optional) {
    warning += fmt::format("{} left record(s) have no match on the right", out.left_orphans);
    if (measured.left_unkeyed > 0) warning += fmt::format(" ({} have no key)", measured.left_unkeyed);
  }
  if (out.right_orphans > 0 && !rel.right_optional) {
    if (!warning.empty()) warning += "; ";
    warning += fmt::format("{} right record(s) have no match on the left", out.right_orphans);
    if (measured.right_unkeyed > 0) warning += fmt::format(" ({} have no key)", measured.right_unkeyed);
  }
  if (!warning.empty()) {
    out.status = RelationshipValidationStatus::Warning;
    out.message = std::move(warning);
  } else {
    out.status = RelationshipValidationStatus::Validated;
  }
  return out;
}

} // namespace

std::vector<locus::LinkAudit> Locus::audit(const locus::JoinOptions& options) const {
  const auto& src = impl_ref_();
  std::vector<LinkAudit> out;
  out.reserve(src.relationships_.size());
  for (const auto& rel : src.relationships_) out.push_back(audit_link(src, rel, options));
  return out;
}
locus::LinkAudit Locus::audit(std::string_view name, const locus::JoinOptions& options) const {
  const auto& src = impl_ref_();
  for (const auto& rel : src.relationships_) {
    if (rel.name == name) return audit_link(src, rel, options);
  }
  pluss::codem::throw_error("Locus::audit: unknown relationship '" + std::string(name) + "'");
  return {};
}
void Locus::validate(const locus::JoinOptions& options) const {
  std::string report;
  for (const auto& entry : audit(options)) {
    if (entry.status == RelationshipValidationStatus::Invalid) {
      report += fmt::format("\n  {}: {}", entry.relationship, entry.message);
    }
  }
  PLUSS_CHECK(report.empty(), "Locus::validate: invalid relationship(s):" + report);
}
namespace {

pluss::kindle::CatalogJoinSpec catalog_spec(const relational::ResolvedJoin& resolved) {
  pluss::kindle::CatalogJoinSpec spec;
  spec.how = resolved.how;
  spec.left_keys = resolved.left_keys;
  spec.right_keys = resolved.right_keys;
  spec.validate = resolved.validate;
  spec.right_suffix = resolved.right_suffix;
  return spec;
}

}  // namespace

LocusJoinResult Locus::join_detailed(std::string_view left,
                                     std::string_view right,
                                     const locus::JoinSpec& spec,
                                     const locus::JoinOptions& options) const {
  const auto& src = impl_ref_();
  PLUSS_CHECK(has(left), "Locus::join: unknown catalog '" + std::string(left) + "'");
  PLUSS_CHECK(has(right), "Locus::join: unknown catalog '" + std::string(right) + "'");

  // Keys, contract and suffix are resolved by the logic Nexus uses; with no
  // keys and no relationship named, the single direct link supplies them. The
  // join itself is Catalog's.
  const auto resolved = relational::resolve_join(src.relationships_, left, right, spec, "Locus::join",
                                                 /*discover_link=*/true);
  const auto& left_catalog = src.catalogs_[src.name_to_pos_.at(std::string(left))];
  const auto& right_catalog = src.catalogs_[src.name_to_pos_.at(std::string(right))];
  return left_catalog.join_detailed(right_catalog, catalog_spec(resolved), options);
}

pluss::kindle::Catalog Locus::join(std::string_view left,
                                   std::string_view right,
                                   const locus::JoinSpec& spec,
                                   const locus::JoinOptions& options) const {
  return join_detailed(left, right, spec, options).catalog;
}

pluss::kindle::Catalog Locus::join(const locus::JoinPath& path,
                                   locus::JoinHow how,
                                   const locus::JoinOptions& options) const {
  const auto& src = impl_ref_();
  PLUSS_CHECK(has(path.from), "Locus::join: unknown catalog '" + path.from + "'");
  if (path.steps.empty()) return catalog(path.from);

  auto current = catalog(path.from);
  std::string at = path.from;
  for (const auto& step : path.steps) {
    PLUSS_CHECK(step.from_table == at,
                fmt::format("Locus::join: path is not contiguous at '{}' (step starts at '{}')", at,
                            step.from_table));
    PLUSS_CHECK(has(step.to_table), "Locus::join: unknown catalog '" + step.to_table + "'");

    // Each hop is the same resolved join a saved plan would run: the link's
    // keys and contract, a per-table suffix.
    locus::JoinSpec hop;
    hop.how = how;
    hop.relationship_name = step.relationship_name;
    const auto resolved = relational::resolve_join(src.relationships_, step.from_table, step.to_table, hop,
                                                   "Locus::join");
    for (const auto& k : resolved.left_keys) {
      PLUSS_CHECK(current.has(k), fmt::format("Locus::join: the accumulated result has no key '{}' for the hop "
                                              "from '{}' (a column may have been renamed by an earlier suffix)",
                                              k, step.from_table));
    }
    current = current.join(catalog(step.to_table), catalog_spec(resolved), options);
    at = step.to_table;
  }
  return current;
}

// ---------------------------------------------------------------------------
// Policy, aligned-Nexus bridge, and everything delegated through it.
// ---------------------------------------------------------------------------

namespace {

std::string policy_key(const TabelizePolicy& p) {
  return fmt::format("{}|{}|{}|{}|{}|{}|{}|{}|{}", static_cast<int>(p.align), static_cast<int>(p.align_by),
                     static_cast<int>(p.fill), p.fill_scalar.value_or(0.0), p.fill_scalar.has_value(),
                     static_cast<int>(p.dtype), static_cast<int>(p.collision), p.anchor.value_or(""),
                     p.include_label_feature ? p.label_feature : std::string());
}

Nexus build_nexus(const detail::LocusImpl& src, const TabelizePolicy& policy) {
  std::vector<std::pair<std::string, DataFrame>> tables;
  tables.reserve(src.keys_.size());
  const auto pkey = policy_key(policy);
  auto& cache = *src.frame_cache_;
  std::unordered_set<std::string> used;
  std::vector<std::size_t> todo;
  std::vector<DataFrame> frames(src.keys_.size());
  {
    std::lock_guard<std::mutex> lock(cache.mutex);
    for (std::size_t i = 0; i < src.keys_.size(); ++i) {
      const auto key = fmt::format("{}|{}", src.catalogs_[i].fingerprint(), pkey);
      used.insert(key);
      const auto it = cache.entries.find(key);
      if (it != cache.entries.end()) {
        frames[i] = it->second.frame;
        ++cache.hits;
      } else {
        todo.push_back(i);
      }
    }
  }
  // Misses are independent: align them in parallel (outside the lock).
  pluss::msharp::parallel_for(0, static_cast<std::int64_t>(todo.size()), 1, [&](std::int64_t a, std::int64_t b) {
    for (std::int64_t t = a; t < b; ++t) {
      const auto i = todo[static_cast<std::size_t>(t)];
      frames[i] = src.catalogs_[i].tabelize(policy);
    }
  });
  {
    std::lock_guard<std::mutex> lock(cache.mutex);
    for (const auto i : todo) {
      const auto key = fmt::format("{}|{}", src.catalogs_[i].fingerprint(), pkey);
      cache.entries.insert_or_assign(key, detail::FrameCache::Entry{src.catalogs_[i], frames[i]});
      ++cache.misses;
    }
    // Branches of one workspace share the cache; keep it bounded by dropping
    // entries this build did not touch only once it has grown well past the
    // working set.
    if (cache.entries.size() > std::max<std::size_t>(64, 4 * src.keys_.size())) {
      for (auto it = cache.entries.begin(); it != cache.entries.end();) {
        it = used.count(it->first) == 0 ? cache.entries.erase(it) : std::next(it);
      }
    }
  }
  for (std::size_t i = 0; i < src.keys_.size(); ++i) tables.emplace_back(src.keys_[i], std::move(frames[i]));
  Nexus out(std::move(tables));
  for (const auto& rel : src.relationships_) out.add_relationship_(rel);
  for (const auto& [name, meta] : src.table_meta_) out.set_table_meta_(name, meta);
  for (const auto& [name, plan] : src.plans_) out.store_(name, plan);
  return out;
}

Catalog kindled(const DataFrame& frame) { return frame.kindelize(); }

}  // namespace

std::pair<std::int64_t, std::int64_t> Locus::alignment_cache_stats() const {
  auto& cache = *impl_ref_().frame_cache_;
  std::lock_guard<std::mutex> lock(cache.mutex);
  return {cache.hits, cache.misses};
}

const TabelizePolicy& Locus::policy() const { return impl_ref_().policy_; }

Locus Locus::with_policy(const TabelizePolicy& policy) const {
  auto out = clone_();
  out.with_policy_(policy);
  return out;
}

Locus& Locus::with_policy_(const TabelizePolicy& policy) {
  impl_mut_().policy_ = policy;
  return *this;
}

Nexus Locus::nexus(const Policy& policy) const {
  const auto& src = impl_ref_();
  const auto effective = policy.value_or(src.policy_);
  const auto key = policy_key(effective);
  if (src.nexus_cache_.has_value() && src.nexus_cache_key_ == key) return *src.nexus_cache_;
  auto built = build_nexus(src, effective);
  src.nexus_cache_ = built;
  src.nexus_cache_key_ = key;
  return built;
}

Locus Locus::from_nexus(const Nexus& nexus) {
  PLUSS_CHECK(nexus.defined(), "Locus::from_nexus: undefined Nexus");
  Catalogs catalogs;
  catalogs.reserve(static_cast<std::size_t>(nexus.ntables()));
  for (std::size_t i = 0; i < nexus.keys().size(); ++i) {
    catalogs.emplace_back(nexus.keys()[i], nexus.frames()[i].kindelize());
  }
  Locus out(std::move(catalogs));
  for (const auto& rel : nexus.relationships()) out.link_(rel);
  for (const auto& key : nexus.keys()) {
    if (const auto meta = nexus.table_meta(key); meta.has_value()) out.set_table_meta_(key, *meta);
  }
  for (const auto& name : nexus.stores()) out.store_(name, *nexus.store(name));
  return out;
}

Locus Locus::from_archive(const pluss::kindle::Archive& archive) {
  PLUSS_CHECK(archive.defined(), "Locus::from_archive: undefined Archive");
  Catalogs catalogs;
  const auto depth = archive.depth();
  catalogs.reserve(static_cast<std::size_t>(depth));
  const bool labelled = archive.label().has();
  std::optional<pluss::kindle::Stream> labels;
  if (labelled) labels = archive.label().values();
  for (std::int64_t i = 0; i < depth; ++i) {
    std::string name = "catalog_" + std::to_string(i);
    if (labels.has_value() && labels->kind() == pluss::kindle::StreamKind::Note) {
      const auto text = std::string(labels->as_note().at(i));
      if (!text.empty()) name = text;
    }
    catalogs.emplace_back(std::move(name), archive.catalog(i));
  }
  return Locus(std::move(catalogs));
}

// -- catalogs ---------------------------------------------------------------

Locus Locus::drop(std::initializer_list<std::string_view> names) const {
  return drop(std::vector<std::string>(names.begin(), names.end()));
}

Locus Locus::drop(const std::vector<std::string>& names) const {
  auto out = *this;
  for (const auto& name : names) out.without_(name);
  return out;
}

Locus Locus::inject(std::string_view name, const Catalog& value) const { return with(name, value); }
Locus& Locus::inject_(std::string_view name, const Catalog& value) { return with_(name, value); }

Locus Locus::inject(std::initializer_list<std::pair<std::string_view, Catalog>> catalogs) const {
  auto out = clone_();
  out.inject_(catalogs);
  return out;
}

Locus& Locus::inject_(std::initializer_list<std::pair<std::string_view, Catalog>> catalogs) {
  for (const auto& [name, value] : catalogs) with_(name, value);
  return *this;
}

Locus Locus::inject(const pluss::kindle::Archive& archive) const {
  auto out = clone_();
  out.inject_(archive);
  return out;
}

Locus& Locus::inject_(const pluss::kindle::Archive& archive) {
  const auto staged = from_archive(archive);
  for (std::size_t i = 0; i < staged.keys().size(); ++i) with_(staged.keys()[i], staged.catalogs()[i]);
  return *this;
}

Locus& Locus::rename_(std::string_view from, std::string_view to) {
  PLUSS_CHECK(!from.empty() && !to.empty(), "Locus::rename: names cannot be empty");
  PLUSS_CHECK(from != to, "Locus::rename: source and target names must differ");
  auto& impl = impl_mut_();
  const std::string from_key(from), to_key(to);
  const auto it = impl.name_to_pos_.find(from_key);
  PLUSS_CHECK(it != impl.name_to_pos_.end(), "Locus::rename: unknown catalog '" + from_key + "'");
  PLUSS_CHECK(impl.name_to_pos_.count(to_key) == 0, "Locus::rename: target name already exists '" + to_key + "'");

  const auto pos = it->second;
  impl.name_to_pos_.erase(it);
  impl.keys_[pos] = to_key;
  impl.name_to_pos_.emplace(to_key, pos);
  if (const auto meta = impl.table_meta_.find(from_key); meta != impl.table_meta_.end()) {
    auto moved = std::move(meta->second);
    impl.table_meta_.erase(meta);
    impl.table_meta_.emplace(to_key, std::move(moved));
  }
  for (auto& rel : impl.relationships_) {
    if (rel.left_table == from_key) rel.left_table = to_key;
    if (rel.right_table == from_key) rel.right_table = to_key;
  }
  for (auto& [name, plan] : impl.plans_) {
    if (plan.root_table == from_key) plan.root_table = to_key;
    for (auto& step : plan.steps) {
      if (step.left_table == from_key) step.left_table = to_key;
      if (step.right_table == from_key) step.right_table = to_key;
    }
  }
  return *this;
}

Locus Locus::rename(std::string_view from, std::string_view to) const {
  auto out = clone_();
  out.rename_(from, to);
  return out;
}

Locus& Locus::apply_(std::string_view name, const CatalogFn& fn) {
  PLUSS_CHECK(static_cast<bool>(fn), "Locus::apply: callback is empty");
  auto& impl = impl_mut_();
  const auto it = impl.name_to_pos_.find(std::string(name));
  PLUSS_CHECK(it != impl.name_to_pos_.end(), "Locus::apply: unknown catalog '" + std::string(name) + "'");
  auto updated = fn(impl.catalogs_[it->second]);
  PLUSS_CHECK(updated.defined(), "Locus::apply: callback returned an undefined Catalog");
  impl.catalogs_[it->second] = std::move(updated);
  return *this;
}

Locus Locus::apply(std::string_view name, const CatalogFn& fn) const {
  auto out = clone_();
  out.apply_(name, fn);
  return out;
}

Locus Locus::apply_all(const CatalogFn& fn) const {
  auto out = clone_();
  for (const auto& name : keys()) out.apply_(name, fn);
  return out;
}

Locus Locus::map_catalogs(const CatalogFn& fn) const { return apply_all(fn); }

Locus Locus::flow(const std::function<Locus(const Locus&)>& fn) const {
  PLUSS_CHECK(static_cast<bool>(fn), "Locus::flow: callback is empty");
  auto out = fn(*this);
  PLUSS_CHECK(out.defined(), "Locus::flow: callback returned an undefined Locus");
  return out;
}

// -- table metadata ---------------------------------------------------------

bool Locus::has_table_meta(std::string_view name) const {
  return impl_ref_().table_meta_.count(std::string(name)) != 0;
}

std::optional<locus::TableMeta> Locus::table_meta(std::string_view name) const {
  const auto& meta = impl_ref_().table_meta_;
  const auto it = meta.find(std::string(name));
  if (it == meta.end()) return std::nullopt;
  return it->second;
}

Locus& Locus::set_table_meta_(std::string_view name, const locus::TableMeta& meta) {
  auto& impl = impl_mut_();
  PLUSS_CHECK(impl.name_to_pos_.count(std::string(name)) != 0,
              "Locus::set_table_meta: unknown catalog '" + std::string(name) + "'");
  impl.table_meta_[std::string(name)] = meta;
  return *this;
}

Locus Locus::set_table_meta(std::string_view name, const locus::TableMeta& meta) const {
  auto out = clone_();
  out.set_table_meta_(name, meta);
  return out;
}

std::optional<locus::TableMeta> Locus::meta(std::string_view name) const { return table_meta(name); }
Locus Locus::meta(std::string_view name, const locus::TableMeta& value) const { return set_table_meta(name, value); }
Locus& Locus::meta_(std::string_view name, const locus::TableMeta& value) { return set_table_meta_(name, value); }

// -- relationships (aliases) -------------------------------------------------

const std::vector<locus::Relationship>& Locus::links() const { return relationships(); }

Locus Locus::add_relationship(const locus::Relationship& rel) const { return link(rel); }
Locus& Locus::add_relationship_(const locus::Relationship& rel) { return link_(rel); }

Locus Locus::add_relationships(std::initializer_list<locus::Relationship> rels) const {
  auto out = clone_();
  out.add_relationships_(rels);
  return out;
}

Locus& Locus::add_relationships_(std::initializer_list<locus::Relationship> rels) {
  for (const auto& rel : rels) link_(rel);
  return *this;
}

// -- integrity and reports (delegated) ---------------------------------------

bool Locus::validate_keys(std::string_view table) const { return nexus().validate_keys(table); }

bool Locus::validate_keys(std::string_view table, std::initializer_list<std::string_view> keys) const {
  return nexus().validate_keys(table, keys);
}

Catalog Locus::duplicates(std::string_view table) const { return kindled(nexus().duplicates(table)); }

Catalog Locus::duplicates(std::string_view table, std::initializer_list<std::string_view> keys) const {
  return kindled(nexus().duplicates(table, keys));
}

Catalog Locus::detect_duplicates(std::string_view table) const { return duplicates(table); }

Catalog Locus::detect_duplicates(std::string_view table, std::initializer_list<std::string_view> keys) const {
  return duplicates(table, keys);
}

bool Locus::validate_relationship(std::string_view name) const { return nexus().validate_relationship(name); }

Catalog Locus::detect_orphans(std::string_view relationship_name, locus::RelationshipSide side) const {
  return kindled(nexus().detect_orphans(relationship_name, side));
}

bool Locus::check(std::string_view name) const { return nexus().check(name); }

DataFrame Locus::grain_audit(std::string_view table) const { return nexus().grain_audit(table); }
DataFrame Locus::relationship_audit(std::string_view name) const { return nexus().relationship_audit(name); }
DataFrame Locus::audit_report() const { return nexus().audit(); }
DataFrame Locus::summary() const { return nexus().summary(); }
DataFrame Locus::profile() const { return nexus().profile(); }
DataFrame Locus::scan() const { return nexus().scan(); }
DataFrame Locus::deduce(double min_confidence) const { return nexus().deduce(min_confidence); }
DataFrame Locus::schema() const { return nexus().schema(); }
DataFrame Locus::graph() const { return nexus().graph(); }
std::string Locus::fingerprint() const { return nexus().fingerprint(); }

// -- saved plans -------------------------------------------------------------

locus::JoinPlan Locus::plan(std::string_view from, std::string_view to, JoinHow how) const {
  const auto& src = impl_ref_();
  PLUSS_CHECK(has(from), "Locus::plan: unknown catalog '" + std::string(from) + "'");
  PLUSS_CHECK(has(to), "Locus::plan: unknown catalog '" + std::string(to) + "'");
  return src.link_cache_.plan(src.keys_, src.relationships_, from, to, how, "Locus::plan");
}

locus::JoinPlan Locus::plan(const JoinPath& path, JoinHow how) const { return relational::plan_from_path(path, how); }

Catalog Locus::materialize(const locus::JoinPlan& plan, const locus::JoinOptions& options) const {
  const auto& src = impl_ref_();
  PLUSS_CHECK(!plan.root_table.empty(), "Locus::materialize: root_table cannot be empty");
  PLUSS_CHECK(has(plan.root_table), "Locus::materialize: unknown catalog '" + plan.root_table + "'");
  auto current = catalog(plan.root_table);
  for (const auto& step : plan.steps) {
    PLUSS_CHECK(has(step.right_table), "Locus::materialize: unknown catalog '" + step.right_table + "'");
    // Each step is resolved exactly as Nexus resolves it (named link, contract,
    // per-table suffix); the join itself is Catalog's.
    const auto resolved = relational::resolve_join(src.relationships_, step.left_table, step.right_table,
                                                   step.spec, "Locus::materialize");
    if (resolved.how != JoinHow::Cross) {
      for (const auto& k : resolved.left_keys) {
        PLUSS_CHECK(current.has(k), fmt::format("Locus::materialize: the accumulated result has no key '{}' for "
                                                "the step from '{}' (a column may have been renamed by an "
                                                "earlier suffix)", k, step.left_table));
      }
    }
    current = current.join(catalog(step.right_table), catalog_spec(resolved), options);
  }
  return current;
}
bool Locus::has_store(std::string_view name) const { return impl_ref_().plans_.count(std::string(name)) != 0; }

std::optional<locus::JoinPlan> Locus::store(std::string_view name) const {
  const auto& plans = impl_ref_().plans_;
  const auto it = plans.find(std::string(name));
  if (it == plans.end()) return std::nullopt;
  return it->second;
}

Locus& Locus::store_(std::string_view name, const locus::JoinPlan& plan) {
  PLUSS_CHECK(!name.empty(), "Locus::store: plan name cannot be empty");
  auto& impl = impl_mut_();
  PLUSS_CHECK(plan_uses(plan, impl.name_to_pos_), "Locus::store: plan names a catalog this Locus does not have");
  impl.plans_[std::string(name)] = plan;
  return *this;
}

Locus Locus::store(std::string_view name, const locus::JoinPlan& plan) const {
  auto out = clone_();
  out.store_(name, plan);
  return out;
}

Catalog Locus::run(std::string_view name, const locus::JoinOptions& options) const {
  const auto& plans = impl_ref_().plans_;
  const auto it = plans.find(std::string(name));
  PLUSS_CHECK(it != plans.end(), "Locus::run: unknown saved plan '" + std::string(name) + "'");
  return materialize(it->second, options);
}
Catalog Locus::run(std::string_view name, const locus::JoinPlan& plan, const locus::JoinOptions& options) const {
  (void)name;  // an inline plan is run as given; the name only labels it
  return materialize(plan, options);
}
// -- builders ----------------------------------------------------------------

LocusQuery Locus::query() const { return LocusQuery(nexus().query()); }
LocusQuery Locus::query(pluss::table::DataFrame::SqlOptions options) const {
  return LocusQuery(nexus().query(std::move(options)));
}

LocusQueryPrepared::LocusQueryPrepared(NexusQueryPrepared inner) : inner_(std::move(inner)) {}
Catalog LocusQueryPrepared::execute() const { return kindled(inner_.execute()); }
Catalog LocusQueryPrepared::execute(Params params) const { return kindled(inner_.execute(std::move(params))); }
Catalog LocusQueryPrepared::execute(Params params,
                                    std::unordered_map<std::string, std::string> identifier_params) const {
  return kindled(inner_.execute(std::move(params), std::move(identifier_params)));
}

LocusQuery::LocusQuery(NexusQuery inner) : inner_(std::move(inner)) {}
Catalog LocusQuery::execute(std::string_view statement) const { return kindled(inner_.execute(statement)); }
LocusQueryPrepared LocusQuery::prepare(std::string_view statement) const {
  return LocusQueryPrepared(inner_.prepare(statement));
}

// -- persistence and display -------------------------------------------------

namespace {

std::string safe_catalog_dir(std::string_view key, std::size_t index) {
  std::string stem;
  for (const char c : key) {
    stem.push_back((std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-') ? c : '_');
  }
  if (stem.empty()) stem = "catalog";
  return std::to_string(index) + "_" + stem;
}

}  // namespace

std::vector<std::string> Locus::stores() const {
  std::vector<std::string> names;
  for (const auto& [name, plan] : impl_ref_().plans_) names.push_back(name);
  std::sort(names.begin(), names.end());
  return names;
}

void Locus::save(std::string_view directory, const locus::StoreOptions& options) const {
  namespace fs = std::filesystem;
  namespace snap = pluss::table::snapshot;
  const auto& src = impl_ref_();
  const fs::path root{std::string(directory)};
  const auto tables_dir = root / options.tables;
  fs::create_directories(tables_dir);

  const auto kind = relational::store_kind_string(options.kind);
  for (std::size_t i = 0; i < src.keys_.size(); ++i) {
    src.catalogs_[i].save_streams((tables_dir / safe_catalog_dir(src.keys_[i], i)).string(), kind);
  }

  std::unordered_map<std::string, locus::TableMeta> meta(src.table_meta_.begin(), src.table_meta_.end());
  bundle::store::meta_frame(src.keys_, meta).save_csv((root / options.schema).string());
  bundle::store::graph_frame(src.relationships_).save_csv((root / options.graph).string());
  {
    std::vector<std::pair<std::string, locus::JoinPlan>> plans;
    for (const auto& name : stores()) plans.emplace_back(name, src.plans_.at(name));
    bundle::store::views_frame(plans).save_csv((root / options.views).string());
  }
  auto text = [](const std::string& v) { return pluss::table::Slate(pluss::make_note(std::vector<std::string>{v})); };
  auto count = [](std::int64_t v) {
    return pluss::table::Slate(pluss::make_series(std::vector<std::int64_t>{v}, {1}));
  };
  pluss::table::DataFrame({
      {"kind", text(kind)},
      {"layout", text("streams")},
      {"fingerprint", text(fingerprint())},
      {"ntables", count(ntables())},
      {"nlinks", count(nrelationships())},
      {"tables", text(options.tables)},
      {"schema", text(options.schema)},
      {"graph", text(options.graph)},
      {"views", text(options.views)},
  }).save_csv((root / options.manifest).string());
}

Locus Locus::load(std::string_view directory, const locus::LoadOptions& options) {
  return from_nexus(Nexus::load(directory, options));
}

Locus Locus::read(std::string_view directory, const locus::StoreOptions& options) {
  namespace fs = std::filesystem;
  namespace snap = pluss::table::snapshot;
  const fs::path root{std::string(directory)};
  const auto manifest_path = root / options.manifest;
  PLUSS_CHECK(fs::exists(manifest_path),
              "Locus::read: manifest '" + manifest_path.string() + "' was not found for store restore");
  const auto manifest = snap::load_snapshot_csv(manifest_path.string());
  PLUSS_CHECK(manifest.ncases() == 1, "Locus::read: manifest must contain exactly one row");
  const bool streams = manifest.get("layout").has_value() && snap::note_cell(manifest, "layout", 0) == "streams";
  // A store written by Nexus keeps one rectangular table per file.
  if (!streams) return from_nexus(Nexus::read(directory, options));

  const auto tables_dir = snap::note_cell(manifest, "tables", 0);
  const auto schema = snap::load_snapshot_csv((root / snap::note_cell(manifest, "schema", 0)).string());
  Catalogs catalogs;
  for (std::int64_t i = 0; i < schema.ncases(); ++i) {
    const auto name = snap::note_cell(schema, "table", i);
    catalogs.emplace_back(
        name, Catalog::load_streams((root / tables_dir / safe_catalog_dir(name, static_cast<std::size_t>(i))).string()));
  }
  Locus out(std::move(catalogs));
  for (const auto& [table, meta] : bundle::store::parse_meta_frame(schema)) out.set_table_meta_(table, meta);
  for (const auto& rel : bundle::store::parse_graph_frame(
           snap::load_snapshot_csv((root / snap::note_cell(manifest, "graph", 0)).string()))) {
    out.link_(rel);
  }
  const auto views_file = root / snap::note_cell(manifest, "views", 0);
  if (fs::exists(views_file)) {
    for (const auto& [name, plan] : bundle::store::parse_views_frame(snap::load_snapshot_csv(views_file.string()))) {
      out.store_(name, plan);
    }
  }
  return out;
}

void Locus::show(std::ostream& os) const { os << to_string() << "\n"; }
void Locus::show() const { show(std::cout); }

std::string Locus::to_string() const {
  if (!defined()) return "Locus(undefined)";
  const auto& src = impl_ref_();
  std::ostringstream os;
  os << "Locus(" << ntables() << " catalogs, " << nrelationships() << " links)";
  for (std::size_t i = 0; i < src.keys_.size(); ++i) {
    const auto [rows, streams] = src.catalogs_[i].shape();
    os << "\n  " << src.keys_[i] << ": [" << streams << " streams x " << rows << " rows]";
  }
  static const char* const kCardinality[] = {"1:1", "1:N", "N:1", "N:N"};
  for (const auto& rel : src.relationships_) {
    os << "\n  " << rel.name << ": " << rel.left_table << " " << kCardinality[static_cast<int>(rel.cardinality)]
       << " " << rel.right_table << " on ";
    for (std::size_t i = 0; i < rel.keys.size(); ++i) {
      if (i) os << ", ";
      os << rel.keys[i].first << "=" << rel.keys[i].second;
    }
  }
  return os.str();
}

std::ostream& operator<<(std::ostream& os, const Locus& value) { return os << value.to_string(); }

} // namespace pluss::bundle
