#include "bundle/Nexus.h"

#include <algorithm>
#include <charconv>
#include <cstdint>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <future>
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
#include <thread>

#include <fmt/format.h>

#include "CodeM/core/Errors.h"
#include "bundle/WorkspaceStore.h"
#include "relational/Codec.h"
#include "relational/Links.h"
#include "relational/Pull.h"
#include "loader/CsvLoader.h"
#include "table/io/SnapshotCsv.h"

namespace pluss::bundle {

using namespace pluss::bundle::nexus;

namespace detail {

struct NexusKeyAuditProfile {
  std::vector<std::string> keys{};
  std::int64_t total_rows{0};
  std::int64_t complete_rows{0};
  std::int64_t missing_rows{0};
  pluss::table::DataFrame complete_frame{};
  pluss::table::DataFrame duplicate_frame{};
};

struct NexusRelationshipAuditProfile {
  Relationship rel{};
  NexusKeyAuditProfile left{};
  NexusKeyAuditProfile right{};
  std::int64_t left_orphan_rows{0};
  std::int64_t right_orphan_rows{0};
  pluss::table::DataFrame left_orphans{};
  pluss::table::DataFrame right_orphans{};
  bool left_unique_required{false};
  bool right_unique_required{false};
  bool valid{false};
  RelationshipValidationStatus computed_status{RelationshipValidationStatus::Declared};
};

class NexusImpl final : public pluss::msharp::RefCounted {
public:
  NexusImpl() = default;
  explicit NexusImpl(std::vector<std::pair<std::string, pluss::table::DataFrame>> tables);

  std::vector<std::string> keys_{};
  std::vector<pluss::table::DataFrame> frames_{};
  std::unordered_map<std::string, std::int64_t> name_to_pos_{};
  std::unordered_map<std::string, TableMeta> table_meta_{};
  std::vector<Relationship> relationships_{};
  std::unordered_map<std::string, std::int64_t> rel_name_to_pos_{};
  std::unordered_map<std::string, JoinPlan> saved_views_{};
  std::int64_t next_auto_id_{0};
  mutable std::unordered_map<std::string, NexusKeyAuditProfile> key_profile_cache_{};
  mutable std::unordered_map<std::string, NexusRelationshipAuditProfile> relationship_audit_cache_{};
  // Adjacency, paths and plans over relationships_ (shared with Locus).
  relational::LinkCache link_cache_{};
  mutable std::unordered_map<std::string, pluss::table::DataFrame> neighbors_cache_{};
  mutable std::optional<pluss::table::DataFrame> profile_cache_{};
  mutable std::optional<pluss::table::DataFrame> scan_cache_{};
  mutable std::optional<pluss::table::DataFrame> deduce_cache_{};
  mutable bool feature_owners_built_{false};
  mutable std::unordered_map<std::string, std::vector<std::size_t>> feature_owners_{};
};

} // namespace detail

#include "bundle/ops/nexus/NexusInternals.inc"
#include "bundle/ops/nexus/NexusPerf.inc"
#include "bundle/ops/nexus/NexusLifecycleAndOps.inc"
#include "bundle/ops/nexus/NexusSchema.inc"
#include "bundle/ops/nexus/NexusAliases.inc"
#include "bundle/ops/nexus/NexusAudit.inc"
#include "bundle/ops/nexus/NexusRelational.inc"
#include "bundle/ops/nexus/NexusQuery.inc"
#include "bundle/ops/nexus/NexusStore.inc"
#include "bundle/ops/nexus/NexusExplore.inc"
#include "bundle/ops/nexus/NexusViews.inc"
#include "bundle/ops/nexus/NexusInterop.inc"
#include "bundle/ops/nexus/NexusRender.inc"

} // namespace pluss::bundle
