#include "table/Nexus.h"

#include <algorithm>
#include <charconv>
#include <cstdint>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "CodeM/core/Errors.h"
#include "loader/CsvLoader.h"

namespace pluss::table {

namespace detail {

struct NexusKeyAuditProfile {
  std::vector<std::string> keys{};
  std::int64_t total_rows{0};
  std::int64_t complete_rows{0};
  std::int64_t missing_rows{0};
  DataFrame complete_frame{};
  DataFrame duplicate_frame{};
};

struct NexusRelationshipAuditProfile {
  Relationship rel{};
  NexusKeyAuditProfile left{};
  NexusKeyAuditProfile right{};
  std::int64_t left_orphan_rows{0};
  std::int64_t right_orphan_rows{0};
  bool left_unique_required{false};
  bool right_unique_required{false};
  bool valid{false};
  RelationshipValidationStatus computed_status{RelationshipValidationStatus::Declared};
};

struct NexusAdjacencyLink {
  std::string next{};
  std::string relationship_name{};
  bool reversed{false};
};

class NexusImpl final : public pluss::msharp::RefCounted {
public:
  NexusImpl() = default;
  explicit NexusImpl(std::vector<std::pair<std::string, DataFrame>> tables);

  std::vector<std::string> keys_{};
  std::vector<DataFrame> frames_{};
  std::unordered_map<std::string, std::int64_t> name_to_pos_{};
  std::unordered_map<std::string, TableMeta> table_meta_{};
  std::vector<Relationship> relationships_{};
  std::unordered_map<std::string, std::int64_t> rel_name_to_pos_{};
  std::int64_t next_auto_id_{0};
  mutable std::unordered_map<std::string, NexusKeyAuditProfile> key_profile_cache_{};
  mutable std::unordered_map<std::string, NexusRelationshipAuditProfile> relationship_audit_cache_{};
  mutable std::unordered_map<std::string, JoinPath> path_cache_{};
  mutable bool adjacency_built_{false};
  mutable std::unordered_map<std::string, std::vector<NexusAdjacencyLink>> adjacency_{};
};

} // namespace detail

#include "table/ops/nexus/NexusInternals.inc"
#include "table/ops/nexus/NexusPerf.inc"
#include "table/ops/nexus/NexusLifecycleAndOps.inc"
#include "table/ops/nexus/NexusSchema.inc"
#include "table/ops/nexus/NexusAliases.inc"
#include "table/ops/nexus/NexusAudit.inc"
#include "table/ops/nexus/NexusRelational.inc"
#include "table/ops/nexus/NexusQuery.inc"
#include "table/ops/nexus/NexusStore.inc"
#include "table/ops/nexus/NexusInterop.inc"
#include "table/ops/nexus/NexusRender.inc"

} // namespace pluss::table
