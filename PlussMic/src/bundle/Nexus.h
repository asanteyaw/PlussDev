#pragma once

#include <cstdint>
#include <functional>
#include <initializer_list>
#include <iosfwd>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "MSharp/IntrusivePtr.h"
#include "table/Atlas.h"
#include "table/DataFrame.h"
#include "table/display/Display.h"

namespace pluss::bundle {

namespace detail {
class NexusImpl;

template <class NameRange>
concept StringViewNameRange = pluss::table::detail::StringViewNameRange<NameRange>;

inline auto stack_name_view(const StackArray<std::string_view>& names) {
  return pluss::table::detail::stack_name_view(names);
}

template <StringViewNameRange NameRange, class Fn>
decltype(auto) with_stack_name_view(const NameRange& names, Fn&& fn) {
  return pluss::table::detail::with_stack_name_view(names, std::forward<Fn>(fn));
}
}

class NexusTraverse;
class NexusPull;
class NexusPullAgg;
class NexusQuery;
class NexusQueryPrepared;

namespace nexus {

enum class CaseLabelPolicy : std::uint8_t {
  Unknown = 0,
  ImplicitRange = 1,
  ExternalAxis = 2,
  FeatureDerived = 3,
  Structured = 4,
};

enum class RelationshipCardinality : std::uint8_t {
  OneToOne = 0,
  OneToMany = 1,
  ManyToOne = 2,
  ManyToMany = 3,
};

enum class RelationshipValidationStatus : std::uint8_t {
  Declared = 0,
  Validated = 1,
  Warning = 2,
  Invalid = 3,
};

enum class RelationshipSide : std::uint8_t {
  Left = 0,
  Right = 1,
};

enum class JoinHow : std::uint8_t {
  Inner = 0,
  Left = 1,
  Right = 2,
  Outer = 3,
  Cross = 4,
  LeftOnly = 5,
  RightOnly = 6,
};

enum class StoreKind : std::uint8_t {
  Csv = 0,
  Json = 1,
  Parquet = 2,
};

enum class PullOp : std::uint8_t {
  Take = 0,
  Count = 1,
  Ndistinct = 2,
  Sum = 3,
  Mean = 4,
  Median = 5,
  Min = 6,
  Max = 7,
  Std = 8,
  Var = 9,
};

struct TableMeta {
  std::optional<std::string> grain{};
  std::vector<std::string> primary_key{};
  std::vector<std::vector<std::string>> candidate_keys{};
  std::optional<std::string> time_field{};
  CaseLabelPolicy case_labels{CaseLabelPolicy::Unknown};
  std::optional<std::string> source{};
  std::optional<std::string> provenance{};
  std::optional<std::string> note{};
};

struct Relationship {
  std::string name{};
  std::string left_table{};
  std::string right_table{};
  std::vector<std::pair<std::string, std::string>> keys{};
  RelationshipCardinality cardinality{RelationshipCardinality::ManyToOne};
  bool left_optional{false};
  bool right_optional{false};
  double confidence{1.0};
  RelationshipValidationStatus status{RelationshipValidationStatus::Declared};
  std::optional<std::string> note{};
};

struct JoinSpec {
  std::vector<std::pair<std::string, std::string>> keys{};
  JoinHow how{JoinHow::Inner};
  std::string right_suffix{};
  std::optional<RelationshipCardinality> validate{};
  std::optional<std::string> relationship_name{};
  std::optional<std::string> result_name{};
};

struct JoinPathStep {
  std::string relationship_name{};
  std::string from_table{};
  std::string to_table{};
  bool reversed{false};
};

struct JoinPath {
  std::string from{};
  std::string to{};
  std::vector<JoinPathStep> steps{};
};

struct JoinPlanStep {
  std::string left_table{};
  std::string right_table{};
  JoinSpec spec{};
};

struct JoinPlan {
  std::string root_table{};
  std::string result_name{};
  std::vector<JoinPlanStep> steps{};
};

struct StoreOptions {
  StoreKind kind{StoreKind::Parquet};
  std::string manifest{"manifest.csv"};
  std::string schema{"schema.csv"};
  std::string graph{"graph.csv"};
  std::string views{"views.csv"};
  std::string tables{"tables"};
};

struct LoadOptions {
  std::vector<std::string> load{};
  std::vector<std::string> ignore{};
  std::vector<std::string> formats{"csv", "txt", "json", "parquet"};
  bool recursive{false};
  bool keep_extension{false};
  bool parallel{true};
  std::optional<std::size_t> max_workers{};
  pluss::loader::Config config{};
};

struct PullWindow {
  std::int64_t value{0};
  std::string unit{};
  bool anchor_root{false};
};

struct PullSpec {
  std::string source{};
  PullOp op{PullOp::Take};
  std::optional<std::string> alias{};
  std::optional<PullWindow> window{};
  bool prefer_feature_alias{false};

  PullSpec named(std::string_view value) const;
  PullSpec windowed(std::int64_t value, std::string_view unit) const;
  PullSpec root_windowed(std::int64_t value, std::string_view unit) const;
};

PullSpec take(std::string_view source, std::string_view alias = {});
PullSpec count(std::string_view source, std::string_view alias = {});
PullSpec ndistinct(std::string_view source, std::string_view alias = {});
PullSpec sum(std::string_view source, std::string_view alias = {});
PullSpec mean(std::string_view source, std::string_view alias = {});
PullSpec median(std::string_view source, std::string_view alias = {});
PullSpec min(std::string_view source, std::string_view alias = {});
PullSpec max(std::string_view source, std::string_view alias = {});
PullSpec std(std::string_view source, std::string_view alias = {});
PullSpec var(std::string_view source, std::string_view alias = {});

} // namespace nexus

class Nexus {
public:
  Nexus(std::vector<std::pair<std::string, pluss::table::DataFrame>> tables);
  Nexus(std::initializer_list<std::pair<std::string, pluss::table::DataFrame>> tables);
  Nexus(const Nexus& other);
  Nexus(Nexus&& other) noexcept;
  Nexus& operator=(const Nexus& other);
  Nexus& operator=(Nexus&& other) noexcept;
  ~Nexus();

  bool defined() const noexcept;
  bool unique() const noexcept;
  bool empty() const;

  std::int64_t size() const;
  std::int64_t ntables() const;

  const std::vector<std::string>& keys() const;
  const std::vector<std::string>& names() const;
  const std::vector<pluss::table::DataFrame>& frames() const;

  bool has(std::string_view key) const;
  pluss::table::DataFrame frame(std::string_view key) const;
  pluss::table::DataFrame frame(std::int64_t index) const;
  pluss::table::DataFrame dloc(std::int64_t index) const;
  std::optional<pluss::table::DataFrame> get(std::string_view key) const;

  Nexus head(std::int64_t n = 5) const;
  Nexus tail(std::int64_t n = 5) const;
  Nexus select(std::initializer_list<std::string_view> keys) const;
  Nexus select(pluss::msharp::RangeView::Of<std::string_view> keys) const;
  template <detail::StringViewNameRange NameRange>
  Nexus select(const NameRange& keys) const {
    return detail::with_stack_name_view(keys, [&](auto names) { return select(names); });
  }
  Nexus drop(std::initializer_list<std::string_view> keys) const;
  Nexus drop(pluss::msharp::RangeView::Of<std::string_view> keys) const;
  template <detail::StringViewNameRange NameRange>
  Nexus drop(const NameRange& keys) const {
    return detail::with_stack_name_view(keys, [&](auto names) { return drop(names); });
  }
  Nexus rename(std::string_view from, std::string_view to) const;
  Nexus& rename_(std::string_view from, std::string_view to);

  Nexus inject(std::string_view key, const pluss::table::DataFrame& frame) const;
  Nexus& inject_(std::string_view key, const pluss::table::DataFrame& frame);
  Nexus inject(std::initializer_list<std::pair<std::string_view, pluss::table::DataFrame>> tables) const;
  Nexus& inject_(std::initializer_list<std::pair<std::string_view, pluss::table::DataFrame>> tables);
  Nexus inject(const pluss::table::DataFrame& frame) const;
  Nexus& inject_(const pluss::table::DataFrame& frame);
  Nexus inject(std::initializer_list<pluss::table::DataFrame> tables) const;
  Nexus& inject_(std::initializer_list<pluss::table::DataFrame> tables);
  Nexus inject(const pluss::table::Atlas& atlas) const;
  Nexus& inject_(const pluss::table::Atlas& atlas);

  Nexus apply(std::string_view key, const std::function<pluss::table::DataFrame(const pluss::table::DataFrame&)>& fn) const;
  Nexus& apply_(std::string_view key, const std::function<pluss::table::DataFrame(const pluss::table::DataFrame&)>& fn);
  Nexus apply_all(const std::function<pluss::table::DataFrame(const pluss::table::DataFrame&)>& fn) const;
  Nexus map_frames(const std::function<pluss::table::DataFrame(const pluss::table::DataFrame&)>& fn) const;
  Nexus flow(const std::function<Nexus(const Nexus&)>& fn) const;

  pluss::table::DataFrame summary() const;
  pluss::table::DataFrame profile() const;
  pluss::table::DataFrame scan() const;
  pluss::table::DataFrame deduce(double min_confidence = 0.5) const;

  bool has_table_meta(std::string_view key) const;
  std::optional<nexus::TableMeta> table_meta(std::string_view key) const;
  Nexus set_table_meta(std::string_view key, const nexus::TableMeta& meta) const;
  Nexus& set_table_meta_(std::string_view key, const nexus::TableMeta& meta);
  std::optional<nexus::TableMeta> meta(std::string_view key) const;
  Nexus meta(std::string_view key, const nexus::TableMeta& value) const;
  Nexus meta(std::initializer_list<std::pair<std::string_view, nexus::TableMeta>> values) const;
  template <class Key, class Compare, class Alloc>
  Nexus meta(const std::map<Key, nexus::TableMeta, Compare, Alloc>& values) const {
    auto out = *this;
    out.meta_(values);
    return out;
  }
  template <class Key, class Hash, class Eq, class Alloc>
  Nexus meta(const std::unordered_map<Key, nexus::TableMeta, Hash, Eq, Alloc>& values) const {
    auto out = *this;
    out.meta_(values);
    return out;
  }
  Nexus& meta_(std::string_view key, const nexus::TableMeta& value);
  Nexus& meta_(std::initializer_list<std::pair<std::string_view, nexus::TableMeta>> values);
  template <class Key, class Compare, class Alloc>
  Nexus& meta_(const std::map<Key, nexus::TableMeta, Compare, Alloc>& values) {
    for (const auto& [key, value] : values) {
      meta_(std::string_view(key), value);
    }
    return *this;
  }
  template <class Key, class Hash, class Eq, class Alloc>
  Nexus& meta_(const std::unordered_map<Key, nexus::TableMeta, Hash, Eq, Alloc>& values) {
    for (const auto& [key, value] : values) {
      meta_(std::string_view(key), value);
    }
    return *this;
  }

  std::int64_t nrelationships() const;
  const std::vector<nexus::Relationship>& relationships() const;
  bool has_relationship(std::string_view name) const;
  std::optional<nexus::Relationship> relationship(std::string_view name) const;
  Nexus add_relationship(const nexus::Relationship& rel) const;
  Nexus& add_relationship_(const nexus::Relationship& rel);
  Nexus add_relationships(std::initializer_list<nexus::Relationship> rels) const;
  Nexus& add_relationships_(std::initializer_list<nexus::Relationship> rels);
  const std::vector<nexus::Relationship>& links() const;
  Nexus links(const nexus::Relationship& rel) const;
  Nexus& links_(const nexus::Relationship& rel);
  Nexus links(std::initializer_list<nexus::Relationship> rels) const;
  Nexus& links_(std::initializer_list<nexus::Relationship> rels);

  bool validate_keys(std::string_view table) const;
  bool validate_keys(std::string_view table,
                     std::initializer_list<std::string_view> keys) const;
  bool validate_keys(std::string_view table,
                     pluss::msharp::RangeView::Of<std::string_view> keys) const;
  template <detail::StringViewNameRange NameRange>
  bool validate_keys(std::string_view table, const NameRange& keys) const {
    return detail::with_stack_name_view(keys, [&](auto names) { return validate_keys(table, names); });
  }
  pluss::table::DataFrame detect_duplicates(std::string_view table) const;
  pluss::table::DataFrame detect_duplicates(std::string_view table,
                              std::initializer_list<std::string_view> keys) const;
  pluss::table::DataFrame detect_duplicates(std::string_view table,
                              pluss::msharp::RangeView::Of<std::string_view> keys) const;
  template <detail::StringViewNameRange NameRange>
  pluss::table::DataFrame detect_duplicates(std::string_view table, const NameRange& keys) const {
    return detail::with_stack_name_view(keys, [&](auto names) { return detect_duplicates(table, names); });
  }
  pluss::table::DataFrame duplicates(std::string_view table) const;
  pluss::table::DataFrame duplicates(std::string_view table,
                       std::initializer_list<std::string_view> keys) const;
  pluss::table::DataFrame duplicates(std::string_view table,
                       pluss::msharp::RangeView::Of<std::string_view> keys) const;
  template <detail::StringViewNameRange NameRange>
  pluss::table::DataFrame duplicates(std::string_view table, const NameRange& keys) const {
    return detail::with_stack_name_view(keys, [&](auto names) { return duplicates(table, names); });
  }
  pluss::table::DataFrame grain_audit(std::string_view table) const;

  bool validate_relationship(std::string_view name) const;
  pluss::table::DataFrame detect_orphans(std::string_view relationship_name,
                           nexus::RelationshipSide side = nexus::RelationshipSide::Left) const;
  pluss::table::DataFrame relationship_audit(std::string_view name) const;
  bool check(std::string_view name) const;
  pluss::table::DataFrame audit() const;
  pluss::table::DataFrame orphans(std::string_view relationship_name,
                    nexus::RelationshipSide side = nexus::RelationshipSide::Left) const;
  pluss::table::DataFrame audit(std::string_view name) const;

  pluss::table::DataFrame join(std::string_view left, std::string_view right, const nexus::JoinSpec& spec) const;
  pluss::table::DataFrame neighbors(std::string_view table) const;
  NexusPull pull(std::string_view root) const;
  pluss::table::DataFrame pull(std::string_view root,
                 std::initializer_list<std::string_view> fields) const;
  pluss::table::DataFrame pull(std::string_view root,
                 pluss::msharp::RangeView::Of<std::string_view> fields) const;
  template <detail::StringViewNameRange NameRange>
  pluss::table::DataFrame pull(std::string_view root, const NameRange& fields) const {
    return detail::with_stack_name_view(fields, [&](auto names) { return pull(root, names); });
  }
  pluss::table::DataFrame pull(std::string_view root,
                 std::initializer_list<nexus::PullSpec> fields) const;
  pluss::table::DataFrame pull(std::string_view root,
                 const std::vector<nexus::PullSpec>& fields) const;
  nexus::JoinPath path(std::string_view from, std::string_view to) const;
  nexus::JoinPlan plan(const nexus::JoinPath& path,
                       nexus::JoinHow how = nexus::JoinHow::Left) const;
  nexus::JoinPlan plan(std::string_view from,
                       std::string_view to,
                       nexus::JoinHow how = nexus::JoinHow::Left) const;
  pluss::table::DataFrame materialize(const nexus::JoinPlan& plan) const;
  bool has_store(std::string_view name) const;
  std::optional<nexus::JoinPlan> store(std::string_view name) const;
  Nexus store(std::string_view name, const nexus::JoinPlan& plan) const;
  Nexus& store_(std::string_view name, const nexus::JoinPlan& plan);
  pluss::table::DataFrame run(std::string_view name) const;
  pluss::table::DataFrame run(std::string_view name, const nexus::JoinPlan& plan) const;
  NexusTraverse from(std::string_view table) const;
  NexusQuery query(pluss::table::DataFrame::SqlOptions options) const;
  NexusQuery query() const;

  pluss::table::DataFrame schema() const;
  pluss::table::DataFrame graph() const;
  std::string fingerprint() const;
  void save(std::string_view directory, const nexus::StoreOptions& options = {}) const;
  static Nexus load(std::string_view directory, const nexus::LoadOptions& options = {});
  static Nexus read(std::string_view directory, const nexus::StoreOptions& options = {});

  static Nexus from_atlas(const pluss::table::Atlas& atlas);
  static Nexus from_atlas(const pluss::table::Atlas& atlas, const pluss::table::Thread& labels);
  pluss::table::Atlas to_atlas(std::initializer_list<std::string_view> keys,
                 std::optional<pluss::table::Thread> labels = std::nullopt) const;
  pluss::table::Atlas to_atlas(pluss::msharp::RangeView::Of<std::string_view> keys,
                 std::optional<pluss::table::Thread> labels = std::nullopt) const;
  template <detail::StringViewNameRange NameRange>
  pluss::table::Atlas to_atlas(const NameRange& keys, std::optional<pluss::table::Thread> labels = std::nullopt) const {
    return detail::with_stack_name_view(keys, [&](auto names) { return to_atlas(names, std::move(labels)); });
  }

  std::string to_string(const pluss::table::display::DisplayOptions& options = {}) const;
  void show(const pluss::table::display::DisplayOptions& options = {}) const;
  void show(std::ostream& os, const pluss::table::display::DisplayOptions& options = {}) const;

private:
  friend class NexusPull;
  friend class NexusTraverse;
  friend class NexusQuery;

  explicit Nexus(pluss::msharp::intrusive_ptr<detail::NexusImpl> impl);

  const detail::NexusImpl& impl_ref_() const;
  detail::NexusImpl& mut_impl_();

  pluss::msharp::intrusive_ptr<detail::NexusImpl> impl_{nullptr};
};

class NexusPull {
public:
  NexusPull(Nexus nexus, std::string root_table);

  NexusPull& add(const nexus::PullSpec& spec);
  NexusPull& add(std::initializer_list<nexus::PullSpec> specs);

  NexusPull& take(pluss::msharp::RangeView::Of<std::string_view> sources);
  NexusPull& take(StackArray<std::string_view> sources) {
    return take(detail::stack_name_view(sources));
  }
  template <detail::StringViewNameRange NameRange>
  NexusPull& take(const NameRange& sources) {
    return take(StackArray<std::string_view>(sources));
  }
  NexusPull& take(std::initializer_list<std::string_view> sources) {
    return take(pluss::msharp::RangeView::Of<std::string_view>(sources));
  }
  NexusPull& take(pluss::msharp::RangeView::Of<std::pair<std::string_view, std::string_view>> fields);
  NexusPull& take(std::string_view table,
                  pluss::msharp::RangeView::Of<std::pair<std::string_view, std::string_view>> fields);
  NexusPull& take(std::initializer_list<std::pair<std::string_view, std::string_view>> fields) {
    return take(pluss::msharp::RangeView::Of<std::pair<std::string_view, std::string_view>>(fields));
  }
  template <class Key, class Value, class Compare, class Alloc>
  NexusPull& take(const std::map<Key, Value, Compare, Alloc>& fields) {
    for (const auto& [source, alias] : fields) {
      this->take(std::string_view(source), std::string_view(alias));
    }
    return *this;
  }
  template <class Key, class Value, class Hash, class Eq, class Alloc>
  NexusPull& take(const std::unordered_map<Key, Value, Hash, Eq, Alloc>& fields) {
    for (const auto& [source, alias] : fields) {
      this->take(std::string_view(source), std::string_view(alias));
    }
    return *this;
  }
  NexusPull& take(std::string_view table,
                  std::initializer_list<std::pair<std::string_view, std::string_view>> fields) {
    return take(table,
                pluss::msharp::RangeView::Of<std::pair<std::string_view, std::string_view>>(fields));
  }
  template <class Key, class Value, class Compare, class Alloc>
  NexusPull& take(std::string_view table, const std::map<Key, Value, Compare, Alloc>& fields) {
    for (const auto& [source, alias] : fields) {
      this->take(table, {{std::string_view(source), std::string_view(alias)}});
    }
    return *this;
  }
  template <class Key, class Value, class Hash, class Eq, class Alloc>
  NexusPull& take(std::string_view table, const std::unordered_map<Key, Value, Hash, Eq, Alloc>& fields) {
    for (const auto& [source, alias] : fields) {
      this->take(table, {{std::string_view(source), std::string_view(alias)}});
    }
    return *this;
  }
  NexusPull& take(std::string_view source, std::string_view alias = {});
  NexusPull& count(std::string_view source, std::string_view alias = {});
  NexusPull& ndistinct(std::string_view source, std::string_view alias = {});
  NexusPull& sum(std::string_view source, std::string_view alias = {});
  NexusPull& mean(std::string_view source, std::string_view alias = {});
  NexusPull& median(std::string_view source, std::string_view alias = {});
  NexusPull& min(std::string_view source, std::string_view alias = {});
  NexusPull& max(std::string_view source, std::string_view alias = {});
  NexusPull& std(std::string_view source, std::string_view alias = {});
  NexusPull& var(std::string_view source, std::string_view alias = {});
  NexusPullAgg agg(std::string_view table);

  const std::string& root() const noexcept;
  const std::vector<nexus::PullSpec>& specs() const noexcept;
  pluss::table::DataFrame collect() const;

private:
  Nexus nexus_{};
  std::string root_table_{};
  std::vector<nexus::PullSpec> specs_{};
};

class NexusPullAgg {
public:
  NexusPullAgg(NexusPull& pull, std::string table_name);

  NexusPullAgg& count(std::string_view feature, std::string_view alias = {});
  NexusPullAgg& count(pluss::msharp::RangeView::Of<std::pair<std::string_view, std::string_view>> fields);
  NexusPullAgg& count(std::initializer_list<std::pair<std::string_view, std::string_view>> fields) {
    return count(pluss::msharp::RangeView::Of<std::pair<std::string_view, std::string_view>>(fields));
  }
  NexusPullAgg& ndistinct(std::string_view feature, std::string_view alias = {});
  NexusPullAgg& ndistinct(pluss::msharp::RangeView::Of<std::pair<std::string_view, std::string_view>> fields);
  NexusPullAgg& ndistinct(std::initializer_list<std::pair<std::string_view, std::string_view>> fields) {
    return ndistinct(pluss::msharp::RangeView::Of<std::pair<std::string_view, std::string_view>>(fields));
  }
  NexusPullAgg& sum(std::string_view feature, std::string_view alias = {});
  NexusPullAgg& sum(pluss::msharp::RangeView::Of<std::pair<std::string_view, std::string_view>> fields);
  NexusPullAgg& sum(std::initializer_list<std::pair<std::string_view, std::string_view>> fields) {
    return sum(pluss::msharp::RangeView::Of<std::pair<std::string_view, std::string_view>>(fields));
  }
  NexusPullAgg& mean(std::string_view feature, std::string_view alias = {});
  NexusPullAgg& mean(pluss::msharp::RangeView::Of<std::pair<std::string_view, std::string_view>> fields);
  NexusPullAgg& mean(std::initializer_list<std::pair<std::string_view, std::string_view>> fields) {
    return mean(pluss::msharp::RangeView::Of<std::pair<std::string_view, std::string_view>>(fields));
  }
  NexusPullAgg& median(std::string_view feature, std::string_view alias = {});
  NexusPullAgg& median(pluss::msharp::RangeView::Of<std::pair<std::string_view, std::string_view>> fields);
  NexusPullAgg& median(std::initializer_list<std::pair<std::string_view, std::string_view>> fields) {
    return median(pluss::msharp::RangeView::Of<std::pair<std::string_view, std::string_view>>(fields));
  }
  NexusPullAgg& min(std::string_view feature, std::string_view alias = {});
  NexusPullAgg& min(pluss::msharp::RangeView::Of<std::pair<std::string_view, std::string_view>> fields);
  NexusPullAgg& min(std::initializer_list<std::pair<std::string_view, std::string_view>> fields) {
    return min(pluss::msharp::RangeView::Of<std::pair<std::string_view, std::string_view>>(fields));
  }
  NexusPullAgg& max(std::string_view feature, std::string_view alias = {});
  NexusPullAgg& max(pluss::msharp::RangeView::Of<std::pair<std::string_view, std::string_view>> fields);
  NexusPullAgg& max(std::initializer_list<std::pair<std::string_view, std::string_view>> fields) {
    return max(pluss::msharp::RangeView::Of<std::pair<std::string_view, std::string_view>>(fields));
  }
  NexusPullAgg& std(std::string_view feature, std::string_view alias = {});
  NexusPullAgg& std(pluss::msharp::RangeView::Of<std::pair<std::string_view, std::string_view>> fields);
  NexusPullAgg& std(std::initializer_list<std::pair<std::string_view, std::string_view>> fields) {
    return std(pluss::msharp::RangeView::Of<std::pair<std::string_view, std::string_view>>(fields));
  }
  NexusPullAgg& var(std::string_view feature, std::string_view alias = {});
  NexusPullAgg& var(pluss::msharp::RangeView::Of<std::pair<std::string_view, std::string_view>> fields);
  NexusPullAgg& var(std::initializer_list<std::pair<std::string_view, std::string_view>> fields) {
    return var(pluss::msharp::RangeView::Of<std::pair<std::string_view, std::string_view>>(fields));
  }
  template <class Key, class Value, class Compare, class Alloc>
  NexusPullAgg& count(const std::map<Key, Value, Compare, Alloc>& fields) {
    for (const auto& [feature, alias] : fields) {
      this->count(std::string_view(feature), std::string_view(alias));
    }
    return *this;
  }
  template <class Key, class Value, class Hash, class Eq, class Alloc>
  NexusPullAgg& count(const std::unordered_map<Key, Value, Hash, Eq, Alloc>& fields) {
    for (const auto& [feature, alias] : fields) {
      this->count(std::string_view(feature), std::string_view(alias));
    }
    return *this;
  }
  template <class Key, class Value, class Compare, class Alloc>
  NexusPullAgg& ndistinct(const std::map<Key, Value, Compare, Alloc>& fields) {
    for (const auto& [feature, alias] : fields) {
      this->ndistinct(std::string_view(feature), std::string_view(alias));
    }
    return *this;
  }
  template <class Key, class Value, class Hash, class Eq, class Alloc>
  NexusPullAgg& ndistinct(const std::unordered_map<Key, Value, Hash, Eq, Alloc>& fields) {
    for (const auto& [feature, alias] : fields) {
      this->ndistinct(std::string_view(feature), std::string_view(alias));
    }
    return *this;
  }
  template <class Key, class Value, class Compare, class Alloc>
  NexusPullAgg& sum(const std::map<Key, Value, Compare, Alloc>& fields) {
    for (const auto& [feature, alias] : fields) {
      this->sum(std::string_view(feature), std::string_view(alias));
    }
    return *this;
  }
  template <class Key, class Value, class Hash, class Eq, class Alloc>
  NexusPullAgg& sum(const std::unordered_map<Key, Value, Hash, Eq, Alloc>& fields) {
    for (const auto& [feature, alias] : fields) {
      this->sum(std::string_view(feature), std::string_view(alias));
    }
    return *this;
  }
  template <class Key, class Value, class Compare, class Alloc>
  NexusPullAgg& mean(const std::map<Key, Value, Compare, Alloc>& fields) {
    for (const auto& [feature, alias] : fields) {
      this->mean(std::string_view(feature), std::string_view(alias));
    }
    return *this;
  }
  template <class Key, class Value, class Hash, class Eq, class Alloc>
  NexusPullAgg& mean(const std::unordered_map<Key, Value, Hash, Eq, Alloc>& fields) {
    for (const auto& [feature, alias] : fields) {
      this->mean(std::string_view(feature), std::string_view(alias));
    }
    return *this;
  }
  template <class Key, class Value, class Compare, class Alloc>
  NexusPullAgg& median(const std::map<Key, Value, Compare, Alloc>& fields) {
    for (const auto& [feature, alias] : fields) {
      this->median(std::string_view(feature), std::string_view(alias));
    }
    return *this;
  }
  template <class Key, class Value, class Hash, class Eq, class Alloc>
  NexusPullAgg& median(const std::unordered_map<Key, Value, Hash, Eq, Alloc>& fields) {
    for (const auto& [feature, alias] : fields) {
      this->median(std::string_view(feature), std::string_view(alias));
    }
    return *this;
  }
  template <class Key, class Value, class Compare, class Alloc>
  NexusPullAgg& min(const std::map<Key, Value, Compare, Alloc>& fields) {
    for (const auto& [feature, alias] : fields) {
      this->min(std::string_view(feature), std::string_view(alias));
    }
    return *this;
  }
  template <class Key, class Value, class Hash, class Eq, class Alloc>
  NexusPullAgg& min(const std::unordered_map<Key, Value, Hash, Eq, Alloc>& fields) {
    for (const auto& [feature, alias] : fields) {
      this->min(std::string_view(feature), std::string_view(alias));
    }
    return *this;
  }
  template <class Key, class Value, class Compare, class Alloc>
  NexusPullAgg& max(const std::map<Key, Value, Compare, Alloc>& fields) {
    for (const auto& [feature, alias] : fields) {
      this->max(std::string_view(feature), std::string_view(alias));
    }
    return *this;
  }
  template <class Key, class Value, class Hash, class Eq, class Alloc>
  NexusPullAgg& max(const std::unordered_map<Key, Value, Hash, Eq, Alloc>& fields) {
    for (const auto& [feature, alias] : fields) {
      this->max(std::string_view(feature), std::string_view(alias));
    }
    return *this;
  }
  template <class Key, class Value, class Compare, class Alloc>
  NexusPullAgg& std(const std::map<Key, Value, Compare, Alloc>& fields) {
    for (const auto& [feature, alias] : fields) {
      this->std(std::string_view(feature), std::string_view(alias));
    }
    return *this;
  }
  template <class Key, class Value, class Hash, class Eq, class Alloc>
  NexusPullAgg& std(const std::unordered_map<Key, Value, Hash, Eq, Alloc>& fields) {
    for (const auto& [feature, alias] : fields) {
      this->std(std::string_view(feature), std::string_view(alias));
    }
    return *this;
  }
  template <class Key, class Value, class Compare, class Alloc>
  NexusPullAgg& var(const std::map<Key, Value, Compare, Alloc>& fields) {
    for (const auto& [feature, alias] : fields) {
      this->var(std::string_view(feature), std::string_view(alias));
    }
    return *this;
  }
  template <class Key, class Value, class Hash, class Eq, class Alloc>
  NexusPullAgg& var(const std::unordered_map<Key, Value, Hash, Eq, Alloc>& fields) {
    for (const auto& [feature, alias] : fields) {
      this->var(std::string_view(feature), std::string_view(alias));
    }
    return *this;
  }

  NexusPull& end() const;

private:
  NexusPullAgg& add_scoped(nexus::PullOp op, std::string_view feature, std::string_view alias);
  NexusPullAgg& add_scoped(
      nexus::PullOp op,
      pluss::msharp::RangeView::Of<std::pair<std::string_view, std::string_view>> fields);

  NexusPull* pull_{nullptr};
  std::string table_name_{};
};

class NexusTraverse {
public:
  NexusTraverse(Nexus nexus, std::string root_table);

  NexusTraverse join(std::string_view right) const;
  NexusTraverse join(std::string_view right, const nexus::JoinSpec& spec) const;
  NexusTraverse via(std::string_view to, nexus::JoinHow how = nexus::JoinHow::Left) const;
  NexusTraverse pick(std::initializer_list<std::string_view> features) const;
  NexusTraverse pick(pluss::msharp::RangeView::Of<std::string_view> features) const;
  template <detail::StringViewNameRange NameRange>
  NexusTraverse pick(const NameRange& features) const {
    return detail::with_stack_name_view(features, [&](auto names) { return pick(names); });
  }
  NexusTraverse drop(std::initializer_list<std::string_view> features) const;
  NexusTraverse drop(pluss::msharp::RangeView::Of<std::string_view> features) const;
  template <detail::StringViewNameRange NameRange>
  NexusTraverse drop(const NameRange& features) const {
    return detail::with_stack_name_view(features, [&](auto names) { return drop(names); });
  }
  NexusTraverse name(std::string_view value) const;
  pluss::table::DataFrame collect() const;

private:
  NexusTraverse(Nexus nexus,
                nexus::JoinPlan plan,
                std::string anchor_table,
                std::optional<std::vector<std::string>> picked,
                std::optional<std::vector<std::string>> dropped,
                std::optional<std::string> title);

  Nexus nexus_;
  nexus::JoinPlan plan_{};
  std::string anchor_table_{};
  std::optional<std::vector<std::string>> picked_{};
  std::optional<std::vector<std::string>> dropped_{};
  std::optional<std::string> title_{};
};

class NexusQueryPrepared {
public:
  NexusQueryPrepared(Nexus nexus,
                     pluss::table::DataFrame::SqlOptions options,
                     std::string statement_template);

  pluss::table::DataFrame execute() const;
  pluss::table::DataFrame execute(
      std::unordered_map<std::string, pluss::table::DataFrame::SqlOptions::SqlParam> params) const;
  pluss::table::DataFrame execute(
      std::unordered_map<std::string, pluss::table::DataFrame::SqlOptions::SqlParam> params,
      std::unordered_map<std::string, std::string> identifier_params) const;

private:
  Nexus nexus_{};
  pluss::table::DataFrame::SqlOptions options_{};
  std::string statement_template_{};
};

class NexusQuery {
public:
  NexusQuery(Nexus nexus, pluss::table::DataFrame::SqlOptions options = {});

  pluss::table::DataFrame execute(std::string_view statement) const;
  NexusQueryPrepared prepare(std::string_view statement) const;

private:
  Nexus nexus_{};
  pluss::table::DataFrame::SqlOptions options_{};
};

std::ostream& operator<<(std::ostream& os, const Nexus& nexus);
void output_render(std::ostream& os, const Nexus& nexus);

} // namespace pluss::bundle
