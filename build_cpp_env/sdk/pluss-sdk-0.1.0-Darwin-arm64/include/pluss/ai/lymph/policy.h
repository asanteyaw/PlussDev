#pragma once

#include "pluss/ai/lymph/common.h"

#include <algorithm>
#include <initializer_list>
#include <string_view>
#include <string>
#include <utility>
#include <vector>

// policy.h -- raw-frame admission policy.
//
// Role: declarative, replayable raw-frame admission/schema policy. This is
// not a preprocessing substitute: imputation, scaling, encoding, feature
// generation, and model-shaped engineering belong to Workflow/Model stages.
// Policy decides what raw columns may enter the workflow and which rows are
// admissible before ML transformations begin.
//
// What lands here:
//   - policy::Policy: drop_columns, require_columns, forbid_columns,
//     reject_missing, allow_missing, and the legacy filter_nulls fields.
//   - Policy::apply(DataFrame) -> DataFrame.
//   - Policy method surface: drop(...), require(...), forbid(...),
//     reject_missing(...), allow_missing(...), reject_any_missing().
//   - lymph::Policy remains the ergonomic root alias; lymph::policy::Policy is
//     the explicit family-local spelling.
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

namespace policy {

struct Policy {
  // Columns stripped before modeling. Use for IDs, leakage columns, audit-only
  // columns, and raw fields that are not inference-time features.
  std::vector<std::string> drop_columns{};

  // Columns that must be present before any drop is applied. Use for raw schema
  // contracts such as labels, time keys, or features required by later Workflow
  // stages.
  std::vector<std::string> require_columns{};

  // Columns that must not appear in the raw frame, unless they are explicitly
  // also listed in drop_columns. Use for "this is leakage if it reaches here"
  // checks in ingestion code.
  std::vector<std::string> forbid_columns{};

  // Preferred row-admission surface: rows with nulls in these named columns
  // are rejected. This is intentionally explicit so users do not destroy
  // feature rows that should be handled by Workflow imputers.
  std::vector<std::string> reject_missing{};

  // Allows a caller to use filter_nulls for a broad admission check while
  // exempting feature columns that are meant to flow into an imputer. Ignored
  // when filter_nulls is false or null_filter_columns is non-empty.
  std::vector<std::string> allow_missing{};

  // Legacy compatibility: broad row filtering. Prefer reject_missing for new
  // code. If true with an empty null_filter_columns list, Policy filters over
  // every remaining column except allow_missing.
  bool filter_nulls = false;
  // Empty means "every remaining column" -- DataFrame::dropnull's own
  // default when no subset is given.
  std::vector<std::string> null_filter_columns{};
  // false drops a row if ANY of null_filter_columns is null, matching
  // DataFrame::dropnull's default. true drops only when ALL are null.
  bool drop_only_if_all_null = false;

  [[nodiscard]] bool empty() const noexcept {
    return drop_columns.empty() &&
           require_columns.empty() &&
           forbid_columns.empty() &&
           reject_missing.empty() &&
           allow_missing.empty() &&
           !filter_nulls;
  }

  Policy& drop(std::vector<std::string> names) {
    append_unique_(drop_columns, names);
    return *this;
  }

  Policy& drop(std::initializer_list<std::string> names) {
    return drop(std::vector<std::string>(names.begin(), names.end()));
  }

  Policy& require(std::vector<std::string> names) {
    append_unique_(require_columns, names);
    return *this;
  }

  Policy& require(std::initializer_list<std::string> names) {
    return require(std::vector<std::string>(names.begin(), names.end()));
  }

  Policy& forbid(std::vector<std::string> names) {
    append_unique_(forbid_columns, names);
    return *this;
  }

  Policy& forbid(std::initializer_list<std::string> names) {
    return forbid(std::vector<std::string>(names.begin(), names.end()));
  }

  Policy& reject(std::vector<std::string> names) {
    append_unique_(reject_missing, names);
    return *this;
  }

  Policy& reject(std::initializer_list<std::string> names) {
    return reject(std::vector<std::string>(names.begin(), names.end()));
  }

  Policy& allow(std::vector<std::string> names) {
    append_unique_(allow_missing, names);
    return *this;
  }

  Policy& allow(std::initializer_list<std::string> names) {
    return allow(std::vector<std::string>(names.begin(), names.end()));
  }

  Policy& reject_any_missing(std::vector<std::string> names) {
    null_filter_columns = std::move(names);
    drop_only_if_all_null = false;
    filter_nulls = true;
    return *this;
  }

  Policy& reject_any_missing(std::initializer_list<std::string> names) {
    return reject_any_missing(std::vector<std::string>(names.begin(), names.end()));
  }

  [[nodiscard]] pluss::table::DataFrame apply(const pluss::table::DataFrame& frame) const {
    validate_(frame);
    pluss::table::DataFrame out = frame;
    if (!drop_columns.empty()) {
      out = out.drop(drop_columns);
    }
    if (!reject_missing.empty()) {
      out = out.dropnull(reject_missing, false);
    }
    if (filter_nulls) {
      const std::vector<std::string> features = legacy_null_filter_features_(out);
      out = out.dropnull(features, drop_only_if_all_null);
    }
    return out;
  }

 private:
  [[nodiscard]] static bool contains_(const std::vector<std::string>& names, std::string_view name) noexcept {
    return std::find_if(names.begin(), names.end(), [name](const std::string& value) {
             return value == name;
           }) != names.end();
  }

  static void append_unique_(std::vector<std::string>& target, const std::vector<std::string>& names) {
    for (const std::string& name : names) {
      if (!name.empty() && !contains_(target, name)) target.push_back(name);
    }
  }

  static void require_present_(const pluss::table::DataFrame& frame,
                               const std::vector<std::string>& names,
                               const char* where) {
    const std::vector<std::string>& available = frame.features();
    for (const std::string& name : names) {
      if (!contains_(available, name)) {
        ::pluss::throw_error(std::string(where) + ": missing required raw column \"" + name + "\".");
      }
    }
  }

  void validate_(const pluss::table::DataFrame& frame) const {
    require_present_(frame, require_columns, "lymph::Policy::apply");
    require_present_(frame, drop_columns, "lymph::Policy::apply(drop_columns)");
    require_present_(frame, reject_missing, "lymph::Policy::apply(reject_missing)");
    require_present_(frame, allow_missing, "lymph::Policy::apply(allow_missing)");
    require_present_(frame, null_filter_columns, "lymph::Policy::apply(null_filter_columns)");

    const std::vector<std::string>& available = frame.features();
    for (const std::string& name : forbid_columns) {
      if (contains_(available, name) && !contains_(drop_columns, name)) {
        ::pluss::throw_error("lymph::Policy::apply: forbidden raw column \"" + name +
                             "\" is present; drop it explicitly if it is intentionally audit-only.");
      }
    }
  }

  [[nodiscard]] std::vector<std::string> legacy_null_filter_features_(const pluss::table::DataFrame& frame) const {
    if (!null_filter_columns.empty()) return null_filter_columns;
    if (allow_missing.empty()) return {};

    std::vector<std::string> out{};
    const std::vector<std::string>& available = frame.features();
    out.reserve(available.size());
    for (const std::string& name : available) {
      if (!contains_(allow_missing, name)) out.push_back(name);
    }
    return out;
  }
};

[[nodiscard]] inline Policy make(Policy options = {}) {
  return std::move(options);
}

} // namespace policy

using Policy = policy::Policy;

} // namespace pluss::ai::lymph
