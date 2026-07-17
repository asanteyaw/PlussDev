#pragma once

#include <cstdint>
#include <functional>
#include <initializer_list>
#include <iosfwd>
#include <optional>
#include <utility>
#include <string>
#include <string_view>
#include <vector>

#include "MSharp/IntrusivePtr.h"
#include "table/Atlas.h"
#include "table/DataFrame.h"
#include "table/display/Display.h"

namespace pluss::table {

namespace detail {
class NexusImpl;
}

class NexusQuery;

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
  std::string tables{"tables"};
};

class Nexus {
public:
  Nexus(std::vector<std::pair<std::string, DataFrame>> tables);
  Nexus(std::initializer_list<std::pair<std::string, DataFrame>> tables);
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
  const std::vector<DataFrame>& frames() const;

  bool has(std::string_view key) const;
  DataFrame frame(std::string_view key) const;
  DataFrame frame(std::int64_t index) const;
  DataFrame dloc(std::int64_t index) const;
  std::optional<DataFrame> get(std::string_view key) const;

  Nexus head(std::int64_t n = 5) const;
  Nexus tail(std::int64_t n = 5) const;
  Nexus select(std::initializer_list<std::string_view> keys) const;
  Nexus drop(std::initializer_list<std::string_view> keys) const;
  Nexus rename(std::string_view from, std::string_view to) const;
  Nexus& rename_(std::string_view from, std::string_view to);

  Nexus inject(std::string_view key, const DataFrame& frame) const;
  Nexus& inject_(std::string_view key, const DataFrame& frame);
  Nexus inject(std::initializer_list<std::pair<std::string_view, DataFrame>> tables) const;
  Nexus& inject_(std::initializer_list<std::pair<std::string_view, DataFrame>> tables);
  Nexus inject(const DataFrame& frame) const;
  Nexus& inject_(const DataFrame& frame);
  Nexus inject(std::initializer_list<DataFrame> tables) const;
  Nexus& inject_(std::initializer_list<DataFrame> tables);
  Nexus inject(const Atlas& atlas) const;
  Nexus& inject_(const Atlas& atlas);

  Nexus apply(std::string_view key, const std::function<DataFrame(const DataFrame&)>& fn) const;
  Nexus& apply_(std::string_view key, const std::function<DataFrame(const DataFrame&)>& fn);
  Nexus apply_all(const std::function<DataFrame(const DataFrame&)>& fn) const;
  Nexus map_frames(const std::function<DataFrame(const DataFrame&)>& fn) const;
  Nexus flow(const std::function<Nexus(const Nexus&)>& fn) const;

  DataFrame summary() const;

  bool has_table_meta(std::string_view key) const;
  std::optional<TableMeta> table_meta(std::string_view key) const;
  Nexus set_table_meta(std::string_view key, const TableMeta& meta) const;
  Nexus& set_table_meta_(std::string_view key, const TableMeta& meta);
  std::optional<TableMeta> meta(std::string_view key) const;
  Nexus meta(std::string_view key, const TableMeta& value) const;
  Nexus& meta_(std::string_view key, const TableMeta& value);

  std::int64_t nrelationships() const;
  const std::vector<Relationship>& relationships() const;
  bool has_relationship(std::string_view name) const;
  std::optional<Relationship> relationship(std::string_view name) const;
  Nexus add_relationship(const Relationship& rel) const;
  Nexus& add_relationship_(const Relationship& rel);
  Nexus add_relationships(std::initializer_list<Relationship> rels) const;
  Nexus& add_relationships_(std::initializer_list<Relationship> rels);
  const std::vector<Relationship>& links() const;
  Nexus links(const Relationship& rel) const;
  Nexus& links_(const Relationship& rel);
  Nexus links(std::initializer_list<Relationship> rels) const;
  Nexus& links_(std::initializer_list<Relationship> rels);

  bool validate_keys(std::string_view table) const;
  bool validate_keys(std::string_view table,
                     std::initializer_list<std::string_view> keys) const;
  DataFrame detect_duplicates(std::string_view table) const;
  DataFrame detect_duplicates(std::string_view table,
                              std::initializer_list<std::string_view> keys) const;
  DataFrame duplicates(std::string_view table) const;
  DataFrame duplicates(std::string_view table,
                       std::initializer_list<std::string_view> keys) const;
  DataFrame grain_audit(std::string_view table) const;

  bool validate_relationship(std::string_view name) const;
  DataFrame detect_orphans(std::string_view relationship_name,
                           RelationshipSide side = RelationshipSide::Left) const;
  DataFrame relationship_audit(std::string_view name) const;
  bool check(std::string_view name) const;
  DataFrame orphans(std::string_view relationship_name,
                    RelationshipSide side = RelationshipSide::Left) const;
  DataFrame audit(std::string_view name) const;

  DataFrame join(std::string_view left, std::string_view right, const JoinSpec& spec) const;
  JoinPath path(std::string_view from, std::string_view to) const;
  JoinPlan plan(const JoinPath& path, JoinHow how = JoinHow::Left) const;
  JoinPlan plan(std::string_view from, std::string_view to, JoinHow how = JoinHow::Left) const;
  DataFrame materialize(const JoinPlan& plan) const;
  NexusQuery from(std::string_view table) const;

  DataFrame schema() const;
  DataFrame graph() const;
  std::string fingerprint() const;
  void save(std::string_view directory, const StoreOptions& options = {}) const;
  static Nexus load(std::string_view directory, const StoreOptions& options = {});

  static Nexus from_atlas(const Atlas& atlas);
  static Nexus from_atlas(const Atlas& atlas, const Thread& labels);
  Atlas to_atlas(std::initializer_list<std::string_view> keys,
                 std::optional<Thread> labels = std::nullopt) const;

  std::string to_string(const display::DisplayOptions& options = {}) const;
  void show(const display::DisplayOptions& options = {}) const;
  void show(std::ostream& os, const display::DisplayOptions& options = {}) const;

private:
  friend class NexusQuery;

  explicit Nexus(pluss::msharp::intrusive_ptr<detail::NexusImpl> impl);

  const detail::NexusImpl& impl_ref_() const;
  detail::NexusImpl& mut_impl_();

  pluss::msharp::intrusive_ptr<detail::NexusImpl> impl_{nullptr};
};

class NexusQuery {
public:
  NexusQuery(Nexus nexus, std::string root_table);

  NexusQuery join(std::string_view right) const;
  NexusQuery join(std::string_view right, const JoinSpec& spec) const;
  NexusQuery via(std::string_view to, JoinHow how = JoinHow::Left) const;
  NexusQuery pick(std::initializer_list<std::string_view> features) const;
  NexusQuery drop(std::initializer_list<std::string_view> features) const;
  NexusQuery name(std::string_view value) const;
  DataFrame collect() const;

private:
  NexusQuery(Nexus nexus,
             JoinPlan plan,
             std::string anchor_table,
             std::optional<std::vector<std::string>> picked,
             std::optional<std::vector<std::string>> dropped,
             std::optional<std::string> title);

  Nexus nexus_;
  JoinPlan plan_{};
  std::string anchor_table_{};
  std::optional<std::vector<std::string>> picked_{};
  std::optional<std::vector<std::string>> dropped_{};
  std::optional<std::string> title_{};
};

std::ostream& operator<<(std::ostream& os, const Nexus& nexus);
void output_render(std::ostream& os, const Nexus& nexus);

} // namespace pluss::table
