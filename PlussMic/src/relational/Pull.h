#pragma once

// The pull grammar, shared by Nexus and Locus. Pure string logic: a field token
// such as "events.spend.sum_7d_root" is split into a table, a feature, an
// aggregate (op, optional time window, anchor) and an output alias. Nothing
// here knows what a table is.

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "MSharp/RangeView.h"
#include "relational/Types.h"

namespace pluss::relational {

struct PullField {
  std::string table{};
  std::string feature{};
  std::string token{};
  std::optional<std::string> aggregate{};
  std::optional<std::string> alias{};
  bool prefer_feature_alias{false};
};


enum class PullAggregateOp : std::uint8_t {
  Count = 0,
  Ndistinct = 1,
  Sum = 2,
  Mean = 3,
  Median = 4,
  Min = 5,
  Max = 6,
  Std = 7,
  Var = 8,
};


struct PullAggregate {
  enum class Anchor : std::uint8_t {
    Latest = 0,
    Root = 1,
  };

  PullAggregateOp op{PullAggregateOp::Count};
  std::string token{};
  std::optional<std::int64_t> window_value{};
  std::optional<std::string> window_unit{};
  Anchor anchor{Anchor::Latest};
};


std::string pull_field_key(std::string_view table, std::string_view feature);
std::string pull_request_key(const PullField& field);
std::optional<PullAggregate> parse_pull_aggregate(std::string_view token);
std::string pull_output_alias(std::string_view root, const PullField& field);
PullField parse_pull_field(std::string_view root, std::string_view token, std::string_view where);
PullField parse_pull_spec(std::string_view root, const PullSpec& spec, std::string_view where);
std::vector<PullField> parse_pull_fields(std::string_view root,
                                         pluss::msharp::RangeView::Of<std::string_view> fields,
                                         std::string_view where);
std::vector<PullField> parse_pull_fields(std::string_view root,
                                         const std::vector<PullSpec>& fields,
                                         std::string_view where);
std::string pull_op_token(PullOp op, std::string_view where);
// Window length of a windowed aggregate in seconds (s/m/h/d/w units).
std::int64_t pull_window_seconds(const PullAggregate& spec, std::string_view where);

} // namespace pluss::relational
