#include "relational/Pull.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <limits>
#include <unordered_set>

#include "CodeM/core/Errors.h"

namespace pluss::relational {

namespace {

bool is_pull_backtick_quoted(std::string_view token) {
  return token.size() >= 2 && token.front() == '`' && token.back() == '`';
}
std::string unquote_pull_token(std::string_view token, std::string_view where) {
  PLUSS_CHECK(is_pull_backtick_quoted(token),
              std::string(where) + ": malformed quoted field token '" + std::string(token) + "'");
  return std::string(token.substr(1, token.size() - 2));
}
std::string lower_ascii(std::string_view token) {
  std::string out(token);
  std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return out;
}
std::int64_t checked_window_scale(std::int64_t value,
                                  std::int64_t seconds_per_unit,
                                  std::string_view where) {
  PLUSS_CHECK(value > 0, std::string(where) + ": pull window value must be positive");
  PLUSS_CHECK(value <= std::numeric_limits<std::int64_t>::max() / seconds_per_unit,
              std::string(where) + ": pull window length exceeds the supported range");
  return value * seconds_per_unit;
}
std::string join_pull_parts(const std::vector<std::string>& parts,
                            std::size_t begin,
                            std::size_t end) {
  std::string out;
  for (std::size_t i = begin; i < end; ++i) {
    if (!out.empty()) {
      out += ".";
    }
    out += parts[i];
  }
  return out;
}
std::vector<std::string> split_pull_dots(std::string_view token) {
  std::vector<std::string> out;
  std::string current;
  bool quoted = false;
  for (const char ch : token) {
    if (ch == '`') {
      quoted = !quoted;
      current.push_back(ch);
      continue;
    }
    if (ch == '.' && !quoted) {
      out.push_back(current);
      current.clear();
      continue;
    }
    current.push_back(ch);
  }
  out.push_back(current);
  return out;
}
std::string pull_alias_piece(std::string_view token) {
  std::string out;
  out.reserve(token.size());
  bool underscore = false;
  for (const char ch : token) {
    if (std::isalnum(static_cast<unsigned char>(ch))) {
      out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
      underscore = false;
      continue;
    }
    if (!underscore && !out.empty()) {
      out.push_back('_');
      underscore = true;
    }
  }
  while (!out.empty() && out.back() == '_') {
    out.pop_back();
  }
  return out.empty() ? "field" : out;
}
std::string normalize_pull_feature_token(std::string_view token, std::string_view where) {
  return is_pull_backtick_quoted(token)
      ? unquote_pull_token(token, where)
      : std::string(token);
}
std::int64_t find_pull_first_dot(std::string_view token) {
  bool quoted = false;
  for (std::size_t i = 0; i < token.size(); ++i) {
    if (token[i] == '`') {
      quoted = !quoted;
      continue;
    }
    if (token[i] == '.' && !quoted) {
      return static_cast<std::int64_t>(i);
    }
  }
  PLUSS_CHECK(!quoted,
              "Nexus::pull: unmatched backtick in field token '" + std::string(token) + "'");
  return -1;
}
} // namespace

std::string pull_field_key(std::string_view table, std::string_view feature) {
  return std::string(table) + "." + std::string(feature);
}
std::string pull_request_key(const PullField& field) {
  auto out = pull_field_key(field.table, field.feature);
  if (field.aggregate.has_value()) {
    out += ".";
    out += *field.aggregate;
  }
  return out;
}
std::optional<PullAggregate> parse_pull_aggregate(std::string_view token) {
  const auto lower_token = lower_ascii(token);
  auto lower = lower_token;
  PullAggregate::Anchor anchor = PullAggregate::Anchor::Latest;
  constexpr std::string_view root_suffix = "_root";
  if (lower.size() > root_suffix.size() &&
      lower.ends_with(root_suffix)) {
    anchor = PullAggregate::Anchor::Root;
    lower.resize(lower.size() - root_suffix.size());
  }
  if (lower == "count") {
    return PullAggregate{
        .op = PullAggregateOp::Count,
        .token = std::string(token),
        .anchor = anchor,
    };
  }
  if (lower == "ndistinct") {
    return PullAggregate{
        .op = PullAggregateOp::Ndistinct,
        .token = std::string(token),
        .anchor = anchor,
    };
  }
  if (lower == "sum") {
    return PullAggregate{
        .op = PullAggregateOp::Sum,
        .token = std::string(token),
        .anchor = anchor,
    };
  }
  if (lower == "mean") {
    return PullAggregate{
        .op = PullAggregateOp::Mean,
        .token = std::string(token),
        .anchor = anchor,
    };
  }
  if (lower == "median") {
    return PullAggregate{
        .op = PullAggregateOp::Median,
        .token = std::string(token),
        .anchor = anchor,
    };
  }
  if (lower == "min") {
    return PullAggregate{
        .op = PullAggregateOp::Min,
        .token = std::string(token),
        .anchor = anchor,
    };
  }
  if (lower == "max") {
    return PullAggregate{
        .op = PullAggregateOp::Max,
        .token = std::string(token),
        .anchor = anchor,
    };
  }
  if (lower == "std") {
    return PullAggregate{
        .op = PullAggregateOp::Std,
        .token = std::string(token),
        .anchor = anchor,
    };
  }
  if (lower == "var") {
    return PullAggregate{
        .op = PullAggregateOp::Var,
        .token = std::string(token),
        .anchor = anchor,
    };
  }

  const auto split = lower.rfind('_');
  if (split == std::string::npos || split == 0 || split + 1 >= lower.size()) {
    return std::nullopt;
  }

  PullAggregate out;
  const auto head = lower.substr(0, split);
  if (head == "count") {
    out.op = PullAggregateOp::Count;
  } else if (head == "sum") {
    out.op = PullAggregateOp::Sum;
  } else if (head == "mean") {
    out.op = PullAggregateOp::Mean;
  } else if (head == "ndistinct") {
    out.op = PullAggregateOp::Ndistinct;
  } else if (head == "median") {
    out.op = PullAggregateOp::Median;
  } else if (head == "min") {
    out.op = PullAggregateOp::Min;
  } else if (head == "max") {
    out.op = PullAggregateOp::Max;
  } else if (head == "std") {
    out.op = PullAggregateOp::Std;
  } else if (head == "var") {
    out.op = PullAggregateOp::Var;
  } else {
    return std::nullopt;
  }

  const auto tail = lower.substr(split + 1);
  std::size_t digits_end = 0;
  while (digits_end < tail.size() && std::isdigit(static_cast<unsigned char>(tail[digits_end]))) {
    ++digits_end;
  }
  if (digits_end == 0 || digits_end == tail.size()) {
    return std::nullopt;
  }
  const auto value_text = tail.substr(0, digits_end);
  const auto unit = tail.substr(digits_end);
  auto value = std::int64_t{0};
  const auto* begin = value_text.data();
  const auto* end = value_text.data() + value_text.size();
  const auto result = std::from_chars(begin, end, value);
  if (result.ec != std::errc{} || result.ptr != end || value <= 0) {
    return std::nullopt;
  }
  out.token = std::string(token);
  out.window_value = value;
  out.window_unit = unit;
  out.anchor = anchor;
  return out;
}
std::string pull_output_alias(std::string_view root, const PullField& field) {
  if (field.alias.has_value()) {
    return *field.alias;
  }
  if (field.prefer_feature_alias) {
    return field.feature;
  }
  if (!field.aggregate.has_value()) {
    return field.table == root
        ? field.feature
        : pull_alias_piece(field.table) + "_" + pull_alias_piece(field.feature);
  }
  return pull_alias_piece(field.table) + "_" + pull_alias_piece(field.feature) + "_" +
      pull_alias_piece(*field.aggregate);
}
PullField parse_pull_field(std::string_view root,
                                std::string_view token,
                                std::string_view where) {
  PLUSS_CHECK(!token.empty(), std::string(where) + ": field token cannot be empty");
  const auto dot = find_pull_first_dot(token);
  if (dot < 0) {
    const auto feature = is_pull_backtick_quoted(token)
        ? unquote_pull_token(token, where)
        : std::string(token);
    return PullField{
        .table = std::string(root),
        .feature = std::move(feature),
        .token = std::string(token),
        .prefer_feature_alias = false,
    };
  }
  PLUSS_CHECK(dot > 0 && static_cast<std::size_t>(dot + 1) < token.size(),
              std::string(where) + ": malformed qualified field '" + std::string(token) + "'");
  const auto table = std::string(token.substr(0, static_cast<std::size_t>(dot)));
  const auto rest = token.substr(static_cast<std::size_t>(dot + 1));
  PLUSS_CHECK(!table.empty() && !rest.empty(),
              std::string(where) + ": malformed qualified field '" + std::string(token) + "'");

  if (is_pull_backtick_quoted(rest)) {
    return PullField{
        .table = table,
        .feature = unquote_pull_token(rest, where),
        .token = std::string(token),
        .prefer_feature_alias = false,
    };
  }

  const auto parts = split_pull_dots(rest);
  PLUSS_CHECK(!parts.empty(), std::string(where) + ": malformed qualified field '" + std::string(token) + "'");
  if (parts.size() == 1) {
    return PullField{
        .table = table,
        .feature = normalize_pull_feature_token(parts.front(), where),
        .token = std::string(token),
        .prefer_feature_alias = false,
    };
  }

  if (const auto aggregate = parse_pull_aggregate(parts.back()); aggregate.has_value()) {
    const auto feature = normalize_pull_feature_token(
        join_pull_parts(parts, 0, parts.size() - 1), where);
    return PullField{
        .table = table,
        .feature = feature,
        .token = std::string(token),
        .aggregate = aggregate->token,
        .prefer_feature_alias = false,
    };
  }

  return PullField{
      .table = table,
      .feature = normalize_pull_feature_token(join_pull_parts(parts, 0, parts.size()), where),
      .token = std::string(token),
      .prefer_feature_alias = false,
  };
}
std::string pull_op_token(PullOp op, std::string_view where) {
  switch (op) {
    case PullOp::Take:
      return {};
    case PullOp::Count:
      return "count";
    case PullOp::Ndistinct:
      return "ndistinct";
    case PullOp::Sum:
      return "sum";
    case PullOp::Mean:
      return "mean";
    case PullOp::Median:
      return "median";
    case PullOp::Min:
      return "min";
    case PullOp::Max:
      return "max";
    case PullOp::Std:
      return "std";
    case PullOp::Var:
      return "var";
  }
  PLUSS_FAIL(std::string(where) + ": unsupported PullOp");
}
PullField parse_pull_spec(std::string_view root,
                               const PullSpec& spec,
                               std::string_view where) {
  PLUSS_CHECK(!spec.source.empty(), std::string(where) + ": pull spec source cannot be empty");
  auto parsed = parse_pull_field(root, spec.source, where);
  parsed.alias = spec.alias;
  parsed.prefer_feature_alias = spec.prefer_feature_alias;
  if (spec.op == PullOp::Take) {
    PLUSS_CHECK(!spec.window.has_value(),
                std::string(where) + ": scalar take spec cannot be windowed");
    parsed.token = spec.source;
    return parsed;
  }

  auto aggregate = pull_op_token(spec.op, where);
  if (spec.window.has_value()) {
    PLUSS_CHECK(spec.window->value > 0,
                std::string(where) + ": pull window value must be positive");
    PLUSS_CHECK(!spec.window->unit.empty(),
                std::string(where) + ": pull window unit cannot be empty");
    aggregate += "_" + std::to_string(spec.window->value) + spec.window->unit;
    if (spec.window->anchor_root) {
      aggregate += "_root";
    }
  }
  parsed.aggregate = std::move(aggregate);
  parsed.token = spec.source + "." + *parsed.aggregate;
  return parsed;
}
std::int64_t pull_window_seconds(const PullAggregate& spec, std::string_view where) {
  PLUSS_CHECK(spec.window_value.has_value() && spec.window_unit.has_value(),
              std::string(where) + ": missing pull window specification");
  const auto value = *spec.window_value;
  const auto unit = lower_ascii(*spec.window_unit);
  if (unit == "s" || unit == "sec" || unit == "secs" || unit == "second" || unit == "seconds") {
    return checked_window_scale(value, 1, where);
  }
  if (unit == "m" || unit == "min" || unit == "mins" || unit == "minute" || unit == "minutes") {
    return checked_window_scale(value, 60LL, where);
  }
  if (unit == "h" || unit == "hr" || unit == "hrs" || unit == "hour" || unit == "hours") {
    return checked_window_scale(value, 60LL * 60LL, where);
  }
  if (unit == "d" || unit == "day" || unit == "days") {
    return checked_window_scale(value, 24LL * 60LL * 60LL, where);
  }
  if (unit == "w" || unit == "wk" || unit == "wks" || unit == "week" || unit == "weeks") {
    return checked_window_scale(value, 7LL * 24LL * 60LL * 60LL, where);
  }
  PLUSS_FAIL(std::string(where) + ": unsupported pull window unit '" + *spec.window_unit + "'");
}
std::vector<PullField> parse_pull_fields(std::string_view root,
                                              pluss::msharp::RangeView::Of<std::string_view> fields,
                                              std::string_view where) {
  PLUSS_CHECK(fields.size() > 0, std::string(where) + ": at least one field is required");
  std::vector<PullField> out;
  out.reserve(fields.size());
  std::unordered_set<std::string> seen;
  seen.reserve(fields.size() * 2 + 1);
  for (const auto field : fields) {
    auto parsed = parse_pull_field(root, field, where);
    const auto key = pull_request_key(parsed);
    PLUSS_CHECK(seen.insert(key).second,
                std::string(where) + ": duplicate requested field '" + key + "'");
    if (parsed.aggregate.has_value()) {
      PLUSS_CHECK(parsed.table != root,
                  std::string(where) + ": aggregate field '" + parsed.token +
                      "' must target a related table, not the root table");
      (void)parse_pull_aggregate(*parsed.aggregate);
    }
    out.push_back(std::move(parsed));
  }
  return out;
}
std::vector<PullField> parse_pull_fields(std::string_view root,
                                              const std::vector<PullSpec>& fields,
                                              std::string_view where) {
  PLUSS_CHECK(!fields.empty(), std::string(where) + ": at least one field is required");
  std::vector<PullField> out;
  out.reserve(fields.size());
  std::unordered_set<std::string> seen;
  seen.reserve(fields.size() * 2 + 1);
  for (const auto& field_spec : fields) {
    auto parsed = parse_pull_spec(root, field_spec, where);
    const auto key = pull_request_key(parsed);
    PLUSS_CHECK(seen.insert(key).second,
                std::string(where) + ": duplicate requested field '" + key + "'");
    if (parsed.aggregate.has_value()) {
      PLUSS_CHECK(parsed.table != root,
                  std::string(where) + ": aggregate field '" + parsed.token +
                      "' must target a related table, not the root table");
      (void)parse_pull_aggregate(*parsed.aggregate);
    }
    out.push_back(std::move(parsed));
  }
  return out;
}
} // namespace pluss::relational
