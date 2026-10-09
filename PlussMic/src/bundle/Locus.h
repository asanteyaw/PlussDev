#pragma once

#include <cstdint>
#include <functional>
#include <initializer_list>
#include <iosfwd>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "MSharp/IntrusivePtr.h"
#include "bundle/Nexus.h"
#include "relational/Match.h"
#include "kindle/Archive.h"
#include "kindle/catalog.h"
#include "table/DataFrame.h"

namespace pluss::bundle {

namespace detail {
class LocusImpl;
}

class Locus;
class LocusPull;
class LocusPullAgg;
class LocusTraverse;
class LocusQuery;
class LocusQueryPrepared;

namespace locus {

// The relational vocabulary lives in the shared core (src/relational) and is
// the same one Nexus uses: a link between two Catalogs is the same declaration
// as a link between two DataFrames.
using relational::JoinHow;
using relational::JoinPath;
using relational::JoinPathStep;
using relational::JoinPlan;
using relational::JoinPlanStep;
using relational::JoinSpec;
using relational::LoadOptions;
using relational::MatchReport;
using relational::PullSpec;
using relational::Relationship;
using relational::RelationshipCardinality;
using relational::RelationshipSide;
using relational::RelationshipValidationStatus;
using relational::StoreOptions;
using relational::TableMeta;

// Outcome of checking one declared link against the data it connects.
struct LinkAudit {
  std::string relationship{};
  RelationshipCardinality declared{RelationshipCardinality::ManyToOne};
  RelationshipValidationStatus status{RelationshipValidationStatus::Declared};
  std::int64_t left_rows{0};
  std::int64_t right_rows{0};
  bool left_unique{false};
  bool right_unique{false};
  // Rows whose key has no counterpart on the other side.
  std::int64_t left_orphans{0};
  std::int64_t right_orphans{0};
  std::string message{};
};

// Joins and audits are executed by Catalog; Locus only resolves the links. The
// raggedness policy (kindle::RaggedPolicy, Relational defaults: keep every
// record, nulls where a row has no partner, losses counted) is carried in the
// options, and the report says what was matched, left without a partner, or omitted.
using JoinOptions = pluss::kindle::CatalogJoinOptions;
using JoinReport = pluss::kindle::CatalogJoinReport;
using pluss::kindle::OpClass;
using pluss::kindle::RaggedAbsent;
using pluss::kindle::RaggedAlignment;
using pluss::kindle::RaggedDomain;
using pluss::kindle::RaggedLoss;
using pluss::kindle::RaggedMaterialize;
using pluss::kindle::RaggedNulls;
using pluss::kindle::RaggedPolicy;

// Options of a pull. The raggedness policy decides how the root and the
// related tables are read (Relational defaults: nothing dropped, nulls where a
// root record has no related record).
struct PullOptions {
  RaggedPolicy ragged{RaggedPolicy::defaults(OpClass::Relational)};
  // Aggregating fields group related records under a root record, which is only
  // well defined when the root's declared primary key is complete and unique
  // (Locus::meta). Turn this off to group by root record regardless.
  bool require_primary_key{true};
};

} // namespace locus

using LocusJoinResult = pluss::kindle::CatalogJoinResult;

// A named, heterogeneous workspace of Catalogs with declared relationships:
// the Catalog counterpart of Nexus (which does the same for DataFrames).
//
// Catalogs hold ragged Streams, so anything relational first aligns each
// catalog into a rectangular table with the Locus's TabelizePolicy. The
// relational work itself (joins, pulls, traversal, SQL, audits, saved plans,
// persistence) is done by Nexus over those aligned tables, so Locus carries no
// second copy of that engine; data results come back as Catalogs and reports as
// DataFrames. Alignment must not lose data silently: Catalog's own default
// (Align::Inner) drops every record whose label is missing from any stream, so
// a Locus defaults to Align::Outer with null fill. Use with_policy() to change
// it (Align::Strict makes ragged input an error), or pass a policy to a call
// that accepts one.
//
// Locus values are immutable handles; mutators return a new Locus and the
// trailing-underscore forms update in place.
class Locus {
public:
  using Catalogs = std::vector<std::pair<std::string, pluss::kindle::Catalog>>;
  using Policy = std::optional<pluss::kindle::TabelizePolicy>;
  using CatalogFn = std::function<pluss::kindle::Catalog(const pluss::kindle::Catalog&)>;

  Locus();
  explicit Locus(Catalogs catalogs);
  Locus(std::initializer_list<std::pair<std::string, pluss::kindle::Catalog>> catalogs);
  Locus(const Locus& other);
  Locus(Locus&& other) noexcept;
  Locus& operator=(const Locus& other);
  Locus& operator=(Locus&& other) noexcept;
  ~Locus();

  // One catalog per depth slice; named by the archive's labels when they are
  // text, otherwise "catalog_<i>".
  static Locus from_archive(const pluss::kindle::Archive& archive);
  // Each DataFrame becomes a Catalog (kindelize); relationships, table
  // metadata and saved plans are carried over.
  static Locus from_nexus(const Nexus& nexus);

  bool defined() const noexcept;
  bool unique() const noexcept;
  bool empty() const;

  std::int64_t size() const;
  std::int64_t ntables() const;
  std::int64_t nrelationships() const;
  std::pair<std::int64_t, std::int64_t> shape() const;  // {tables, relationships}

  // Alignment used by every relational operation that is not given its own.
  static pluss::kindle::TabelizePolicy default_policy();
  const pluss::kindle::TabelizePolicy& policy() const;
  Locus with_policy(const pluss::kindle::TabelizePolicy& policy) const;
  Locus& with_policy_(const pluss::kindle::TabelizePolicy& policy);
  // The aligned DataFrame workspace everything relational is delegated to
  // (cached until the Locus changes).
  Nexus nexus(const Policy& policy = std::nullopt) const;

  // Catalogs.
  const std::vector<std::string>& keys() const;
  const std::vector<std::string>& names() const;
  const std::vector<pluss::kindle::Catalog>& catalogs() const;
  bool has(std::string_view name) const;
  pluss::kindle::Catalog catalog(std::string_view name) const;
  pluss::kindle::Catalog catalog(std::int64_t index) const;
  pluss::kindle::Catalog dloc(std::int64_t index) const;
  std::optional<pluss::kindle::Catalog> get(std::string_view name) const;

  Locus head(std::int64_t n = 5) const;
  Locus tail(std::int64_t n = 5) const;
  Locus select(std::initializer_list<std::string_view> names) const;
  Locus select(const std::vector<std::string>& names) const;
  Locus drop(std::initializer_list<std::string_view> names) const;
  Locus drop(const std::vector<std::string>& names) const;
  // Add a catalog, or replace the one with the same name. Links that name it
  // stay in place and are re-checked against the new data by audit().
  Locus with(std::string_view name, const pluss::kindle::Catalog& value) const;
  Locus& with_(std::string_view name, const pluss::kindle::Catalog& value);
  Locus inject(std::string_view name, const pluss::kindle::Catalog& value) const;
  Locus& inject_(std::string_view name, const pluss::kindle::Catalog& value);
  Locus inject(std::initializer_list<std::pair<std::string_view, pluss::kindle::Catalog>> catalogs) const;
  Locus& inject_(std::initializer_list<std::pair<std::string_view, pluss::kindle::Catalog>> catalogs);
  Locus inject(const pluss::kindle::Archive& archive) const;
  Locus& inject_(const pluss::kindle::Archive& archive);
  // Remove a catalog together with every link, metadata entry and saved plan
  // that touches it.
  Locus without(std::string_view name) const;
  Locus& without_(std::string_view name);
  // Re-key a catalog; links, metadata and saved plans follow.
  Locus rename(std::string_view from, std::string_view to) const;
  Locus& rename_(std::string_view from, std::string_view to);
  Locus apply(std::string_view name, const CatalogFn& fn) const;
  Locus& apply_(std::string_view name, const CatalogFn& fn);
  Locus apply_all(const CatalogFn& fn) const;
  Locus map_catalogs(const CatalogFn& fn) const;
  Locus flow(const std::function<Locus(const Locus&)>& fn) const;

  // Table metadata (grain, keys, time field, provenance...), shared with Nexus.
  bool has_table_meta(std::string_view name) const;
  std::optional<locus::TableMeta> table_meta(std::string_view name) const;
  Locus set_table_meta(std::string_view name, const locus::TableMeta& meta) const;
  Locus& set_table_meta_(std::string_view name, const locus::TableMeta& meta);
  std::optional<locus::TableMeta> meta(std::string_view name) const;
  Locus meta(std::string_view name, const locus::TableMeta& value) const;
  Locus& meta_(std::string_view name, const locus::TableMeta& value);

  // Relationships.
  const std::vector<locus::Relationship>& relationships() const;
  const std::vector<locus::Relationship>& links() const;
  bool has_relationship(std::string_view name) const;
  std::optional<locus::Relationship> relationship(std::string_view name) const;
  // The relationship's tables and key columns must exist. An empty name is
  // filled in as "left->right".
  Locus link(const locus::Relationship& rel) const;
  Locus& link_(const locus::Relationship& rel);
  Locus link(std::string_view left,
             std::string_view right,
             std::string_view key,
             locus::RelationshipCardinality cardinality = locus::RelationshipCardinality::ManyToOne) const;
  Locus link(std::string_view left,
             std::string_view left_key,
             std::string_view right,
             std::string_view right_key,
             locus::RelationshipCardinality cardinality = locus::RelationshipCardinality::ManyToOne) const;
  Locus add_relationship(const locus::Relationship& rel) const;
  Locus& add_relationship_(const locus::Relationship& rel);
  Locus add_relationships(std::initializer_list<locus::Relationship> rels) const;
  Locus& add_relationships_(std::initializer_list<locus::Relationship> rels);
  Locus unlink(std::string_view name) const;
  Locus& unlink_(std::string_view name);
  // Tables directly linked to `table`, in declaration order.
  std::vector<std::string> neighbors(std::string_view table) const;
  // Shortest chain of links from one table to another (fewest hops, ties
  // broken by declaration order). Empty steps when from == to.
  std::optional<locus::JoinPath> path(std::string_view from, std::string_view to) const;

  // Integrity. Link audits measure unique keys, cardinality and orphan rows on
  // the aligned tables.
  std::vector<locus::LinkAudit> audit(const locus::JoinOptions& options = {}) const;
  locus::LinkAudit audit(std::string_view relationship, const locus::JoinOptions& options = {}) const;
  // Throws if any link is Invalid (missing key, or cardinality contradicted).
  void validate(const locus::JoinOptions& options = {}) const;
  // Per-table key checks: are the key columns present and unique?
  bool validate_keys(std::string_view table) const;
  bool validate_keys(std::string_view table, std::initializer_list<std::string_view> keys) const;
  // Records whose key (the table's declared primary key unless given) repeats.
  pluss::kindle::Catalog duplicates(std::string_view table) const;
  pluss::kindle::Catalog duplicates(std::string_view table, std::initializer_list<std::string_view> keys) const;
  pluss::kindle::Catalog detect_duplicates(std::string_view table) const;
  pluss::kindle::Catalog detect_duplicates(std::string_view table,
                                           std::initializer_list<std::string_view> keys) const;
  bool validate_relationship(std::string_view name) const;
  // Records on one side of a link that find no partner on the other.
  pluss::kindle::Catalog detect_orphans(
      std::string_view relationship_name,
      locus::RelationshipSide side = locus::RelationshipSide::Left) const;
  bool check(std::string_view name) const;
  // DataFrame reports (workspace-level, not catalog data).
  pluss::table::DataFrame grain_audit(std::string_view table) const;
  pluss::table::DataFrame relationship_audit(std::string_view name) const;
  pluss::table::DataFrame audit_report() const;
  pluss::table::DataFrame summary() const;
  pluss::table::DataFrame profile() const;
  pluss::table::DataFrame scan() const;
  pluss::table::DataFrame deduce(double min_confidence = 0.5) const;
  pluss::table::DataFrame schema() const;
  pluss::table::DataFrame graph() const;
  // Stable digest of the aligned data, links and metadata.
  std::string fingerprint() const;

  // Joins. With no keys in `spec` the declared link between the two catalogs
  // supplies them (an error if there are several), and its cardinality is
  // enforced. Matching is done on the key streams alone and every other stream
  // is rebuilt through the matches, so ragged catalogs stay ragged; see
  // kindle::RaggedPolicy for the knobs and JoinReport for what was matched, left
  // without a partner, or omitted.
  pluss::kindle::Catalog join(std::string_view left,
                              std::string_view right,
                              const locus::JoinSpec& spec = {},
                              const locus::JoinOptions& options = {}) const;
  LocusJoinResult join_detailed(std::string_view left,
                                std::string_view right,
                                const locus::JoinSpec& spec = {},
                                const locus::JoinOptions& options = {}) const;
  // Join along a chain of links, accumulating columns table by table (each hop
  // enforces its link's cardinality, as a saved plan would).
  pluss::kindle::Catalog join(const locus::JoinPath& path,
                              locus::JoinHow how = locus::JoinHow::Left,
                              const locus::JoinOptions& options = {}) const;
  // Join plans: the shortest path between two catalogs as a plan, and plans run
  // natively, one Catalog join per step.
  locus::JoinPlan plan(std::string_view from, std::string_view to,
                       locus::JoinHow how = locus::JoinHow::Left) const;
  locus::JoinPlan plan(const locus::JoinPath& path, locus::JoinHow how = locus::JoinHow::Left) const;
  pluss::kindle::Catalog materialize(const locus::JoinPlan& plan, const locus::JoinOptions& options = {}) const;
  bool has_store(std::string_view name) const;
  // Names of saved plans, sorted.
  std::vector<std::string> stores() const;
  std::optional<locus::JoinPlan> store(std::string_view name) const;
  Locus store(std::string_view name, const locus::JoinPlan& plan) const;
  Locus& store_(std::string_view name, const locus::JoinPlan& plan);
  pluss::kindle::Catalog run(std::string_view name, const locus::JoinOptions& options = {}) const;
  pluss::kindle::Catalog run(std::string_view name, const locus::JoinPlan& plan,
                             const locus::JoinOptions& options = {}) const;

  // Builders: gather columns across links, walk joins from a table, run SQL.
  LocusPull pull(std::string_view root) const;
  LocusTraverse from(std::string_view table) const;
  LocusQuery query() const;
  LocusQuery query(pluss::table::DataFrame::SqlOptions options) const;

  // Persistence is ragged-native: each catalog is stored stream by stream
  // (Catalog::save_streams), so extents, nulls, kinds and labels round-trip
  // exactly. Links, table metadata and saved plans round-trip too. read() also
  // accepts a store written by Nexus (rectangular tables). load() reads a
  // directory of loose table files through the DataFrame loaders.
  // (hits, misses) of the per-catalog alignment cache behind nexus(): a catalog is
  // padded once per policy and reused by every Locus derived from this one until
  // that catalog itself changes.
  std::pair<std::int64_t, std::int64_t> alignment_cache_stats() const;
  void save(std::string_view directory, const locus::StoreOptions& options = {}) const;
  static Locus load(std::string_view directory, const locus::LoadOptions& options = {});
  static Locus read(std::string_view directory, const locus::StoreOptions& options = {});

  std::string to_string() const;
  void show(std::ostream& os) const;
  void show() const;
  friend std::ostream& operator<<(std::ostream& os, const Locus& value);

private:
  explicit Locus(pluss::msharp::intrusive_ptr<detail::LocusImpl> impl);
  Locus clone_() const;
  const detail::LocusImpl& impl_ref_() const;
  detail::LocusImpl& impl_mut_();

  pluss::msharp::intrusive_ptr<detail::LocusImpl> impl_{nullptr};
};

// Gathers columns from tables reachable from `root` (take) or aggregates them
// per root record (count, sum, mean...), following the declared links.
// Sources are written "table.feature" or "table.table.feature" for a longer path.
// Aggregating pulls (count, sum, mean...) need the root table's primary key
// declared first (Locus::meta), since it defines what one output record is.
class LocusPull {
public:
  LocusPull(Locus locus, std::string root);

  LocusPull& options(const locus::PullOptions& options);
  LocusPull& add(const locus::PullSpec& spec);
  LocusPull& add(std::initializer_list<locus::PullSpec> specs);
  LocusPull& take(std::initializer_list<std::string_view> sources);
  LocusPull& take(std::initializer_list<std::pair<std::string_view, std::string_view>> fields);
  LocusPull& take(std::string_view table,
                  std::initializer_list<std::pair<std::string_view, std::string_view>> fields);
  LocusPull& take(std::string_view source, std::string_view alias = {});
  LocusPull& count(std::string_view source, std::string_view alias = {});
  LocusPull& ndistinct(std::string_view source, std::string_view alias = {});
  LocusPull& sum(std::string_view source, std::string_view alias = {});
  LocusPull& mean(std::string_view source, std::string_view alias = {});
  LocusPull& median(std::string_view source, std::string_view alias = {});
  LocusPull& min(std::string_view source, std::string_view alias = {});
  LocusPull& max(std::string_view source, std::string_view alias = {});
  LocusPull& std(std::string_view source, std::string_view alias = {});
  LocusPull& var(std::string_view source, std::string_view alias = {});
  // Scope the following aggregates to one table; end() returns to the pull.
  LocusPullAgg agg(std::string_view table);

  const std::string& root() const noexcept;
  const std::vector<locus::PullSpec>& specs() const noexcept;
  // One record per root record. Plain fields follow root-preserving links
  // (many-to-one / one-to-one) by composing the matches, so no intermediate table
  // is built; aggregates reduce the related records of each root record, over a
  // time window when the token asks for one ("sum_7d", "mean_30d_root").
  pluss::kindle::Catalog collect() const;

private:
  Locus locus_;
  std::string root_;
  std::vector<locus::PullSpec> specs_{};
  locus::PullOptions options_{};
};

class LocusPullAgg {
public:
  LocusPullAgg(LocusPull& pull, std::string table);

  LocusPullAgg& count(std::string_view feature, std::string_view alias = {});
  LocusPullAgg& count(std::initializer_list<std::pair<std::string_view, std::string_view>> fields);
  LocusPullAgg& ndistinct(std::string_view feature, std::string_view alias = {});
  LocusPullAgg& ndistinct(std::initializer_list<std::pair<std::string_view, std::string_view>> fields);
  LocusPullAgg& sum(std::string_view feature, std::string_view alias = {});
  LocusPullAgg& sum(std::initializer_list<std::pair<std::string_view, std::string_view>> fields);
  LocusPullAgg& mean(std::string_view feature, std::string_view alias = {});
  LocusPullAgg& mean(std::initializer_list<std::pair<std::string_view, std::string_view>> fields);
  LocusPullAgg& median(std::string_view feature, std::string_view alias = {});
  LocusPullAgg& median(std::initializer_list<std::pair<std::string_view, std::string_view>> fields);
  LocusPullAgg& min(std::string_view feature, std::string_view alias = {});
  LocusPullAgg& min(std::initializer_list<std::pair<std::string_view, std::string_view>> fields);
  LocusPullAgg& max(std::string_view feature, std::string_view alias = {});
  LocusPullAgg& max(std::initializer_list<std::pair<std::string_view, std::string_view>> fields);
  LocusPullAgg& std(std::string_view feature, std::string_view alias = {});
  LocusPullAgg& std(std::initializer_list<std::pair<std::string_view, std::string_view>> fields);
  LocusPullAgg& var(std::string_view feature, std::string_view alias = {});
  LocusPullAgg& var(std::initializer_list<std::pair<std::string_view, std::string_view>> fields);
  LocusPull& end() const;

private:
  LocusPull* pull_{nullptr};
  std::string table_{};
};

// Walks joins outward from a root table, then picks or drops columns.
class LocusTraverse {
public:
  LocusTraverse(Locus locus, std::string root);

  LocusTraverse options(const locus::JoinOptions& options) const;
  LocusTraverse join(std::string_view right) const;
  LocusTraverse join(std::string_view right, const locus::JoinSpec& spec) const;
  LocusTraverse via(std::string_view to, locus::JoinHow how = locus::JoinHow::Left) const;
  LocusTraverse pick(const std::vector<std::string>& features) const;
  LocusTraverse pick(std::initializer_list<std::string_view> features) const;
  LocusTraverse drop(const std::vector<std::string>& features) const;
  LocusTraverse drop(std::initializer_list<std::string_view> features) const;
  pluss::kindle::Catalog collect() const;

private:
  LocusTraverse(Locus locus, locus::JoinPlan plan, std::string anchor, locus::JoinOptions options,
                std::optional<std::vector<std::string>> picked, std::optional<std::vector<std::string>> dropped);

  Locus locus_;
  locus::JoinPlan plan_{};
  std::string anchor_{};
  locus::JoinOptions options_{};
  std::optional<std::vector<std::string>> picked_{};
  std::optional<std::vector<std::string>> dropped_{};
};

class LocusQueryPrepared {
public:
  using Params = std::unordered_map<std::string, pluss::table::DataFrame::SqlOptions::SqlParam>;

  LocusQueryPrepared(NexusQueryPrepared inner);

  pluss::kindle::Catalog execute() const;
  pluss::kindle::Catalog execute(Params params) const;
  pluss::kindle::Catalog execute(Params params, std::unordered_map<std::string, std::string> identifier_params) const;

private:
  NexusQueryPrepared inner_;
};

// SQL over the aligned tables: every catalog is a table named after it.
class LocusQuery {
public:
  LocusQuery(NexusQuery inner);

  pluss::kindle::Catalog execute(std::string_view statement) const;
  LocusQueryPrepared prepare(std::string_view statement) const;

private:
  NexusQuery inner_;
};

} // namespace pluss::bundle
