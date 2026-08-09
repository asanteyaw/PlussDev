#pragma once

#include "pluss/ai/lymph/common.h"

#include <string>
#include <utility>
#include <vector>

// policy.h -- raw-frame cleanup policy.
//
// Role: declarative, replayable raw-frame cleanup -- which columns never
// reach a model (ID/leakage columns) and which columns must be non-null for
// a row to survive. Construction is through lymph::policy(...), per the
// construction-vs-method rule.
//
// What lands here:
//   - struct Policy: drop_columns, filter_nulls, null_filter_columns,
//     drop_only_if_all_null.
//   - Policy::apply(DataFrame) -> DataFrame.
//   - lymph::policy({...}) -- a single free function, not a
//     lymph::policy:: namespace: Policy has only one construction shape, no
//     sub-variants to distinguish (unlike ColumnRouter/Model/Search, which
//     each have several named factories).
//
// What does NOT live here: save_policy/load_policy (the bundle sidecar
// persistence) -- those are Workflow concerns, since only Workflow::save/load
// needs to round-trip a policy next to a trained model bundle. Policy itself
// has no persistence of its own.
//
// Depends on: DataFrame::drop()/dropnull() only (pluss::table) -- nothing
// NodeJ-level.
//
// Consumed by: workflow.h (Workflow = Model + Policy).

namespace pluss::ai::lymph {

struct Policy {
  std::vector<std::string> drop_columns{};
  bool filter_nulls = false;
  // Empty means "every remaining column" -- DataFrame::dropnull's own
  // default when no subset is given.
  std::vector<std::string> null_filter_columns{};
  // false drops a row if ANY of null_filter_columns is null, matching
  // DataFrame::dropnull's default. true drops only when ALL are null.
  bool drop_only_if_all_null = false;

  [[nodiscard]] bool empty() const noexcept { return drop_columns.empty() && !filter_nulls; }

  [[nodiscard]] pluss::table::DataFrame apply(const pluss::table::DataFrame& frame) const {
    pluss::table::DataFrame out = frame;
    if (!drop_columns.empty()) {
      out = out.drop(drop_columns);
    }
    if (filter_nulls) {
      out = out.dropnull(null_filter_columns, drop_only_if_all_null);
    }
    return out;
  }
};

[[nodiscard]] inline Policy policy(Policy options = {}) {
  return std::move(options);
}

} // namespace pluss::ai::lymph
