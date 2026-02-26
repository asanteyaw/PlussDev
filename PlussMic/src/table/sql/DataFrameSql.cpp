#include "table/sql/DataFrameSql.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

#include "CodeM/core/Errors.h"

namespace pluss::table {

namespace {

enum class SqlTokenKind : std::uint8_t { Word = 0, String = 1, Backtick = 2, Punct = 3 };

struct SqlToken {
  SqlTokenKind kind{SqlTokenKind::Word};
  std::string text{};
  std::size_t begin{0};
  std::size_t end{0};
};

enum class SqlClauseKind : std::uint8_t { Where = 0, OrderBy = 1, Limit = 2, Offset = 3 };

struct SqlClause {
  SqlClauseKind kind{SqlClauseKind::Where};
  std::size_t token_index{0};
  std::size_t keyword_tokens{1};
};

struct SqlProjection {
  bool star{false};
  std::string source{};
  std::string output{};
};

struct SqlOrderSpec {
  std::string feature{};
  bool ascending{true};
  bool nulls_last{true};
};

struct SqlSelectSpec {
  bool select_all{false};
  std::vector<SqlProjection> projections{};
  std::optional<std::string> where{};
  std::optional<SqlOrderSpec> order{};
  std::optional<std::int64_t> limit{};
  std::optional<std::int64_t> offset{};
};

bool is_punct_char(const char ch) {
  return ch == '(' || ch == ')' || ch == ',' || ch == ';';
}

std::string trim_ascii(std::string_view s) {
  std::size_t b = 0;
  std::size_t e = s.size();
  while (b < e && std::isspace(static_cast<unsigned char>(s[b])) != 0) {
    ++b;
  }
  while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1])) != 0) {
    --e;
  }
  return std::string(s.substr(b, e - b));
}

bool iequals(std::string_view a, std::string_view b) {
  if (a.size() != b.size()) {
    return false;
  }
  for (std::size_t i = 0; i < a.size(); ++i) {
    const auto lhs = static_cast<unsigned char>(a[i]);
    const auto rhs = static_cast<unsigned char>(b[i]);
    if (std::tolower(lhs) != std::tolower(rhs)) {
      return false;
    }
  }
  return true;
}

std::vector<SqlToken> tokenize_sql(std::string_view sql) {
  std::vector<SqlToken> out;
  std::size_t i = 0;
  while (i < sql.size()) {
    const auto ch = sql[i];
    if (std::isspace(static_cast<unsigned char>(ch)) != 0) {
      ++i;
      continue;
    }

    if (ch == '\'') {
      const auto start = i++;
      std::string body;
      bool closed = false;
      while (i < sql.size()) {
        const auto c = sql[i];
        if (c == '\'') {
          if (i + 1 < sql.size() && sql[i + 1] == '\'') {
            body.push_back('\'');
            i += 2;
            continue;
          }
          ++i;
          closed = true;
          break;
        }
        body.push_back(c);
        ++i;
      }
      PLUSS_CHECK(closed, "DataFrame::sql().execute: unterminated SQL string literal");
      out.push_back(SqlToken{SqlTokenKind::String, std::move(body), start, i});
      continue;
    }

    if (ch == '`') {
      const auto start = i++;
      std::string body;
      bool closed = false;
      while (i < sql.size()) {
        const auto c = sql[i];
        if (c == '`') {
          ++i;
          closed = true;
          break;
        }
        body.push_back(c);
        ++i;
      }
      PLUSS_CHECK(closed, "DataFrame::sql().execute: unterminated backtick identifier");
      out.push_back(SqlToken{SqlTokenKind::Backtick, std::move(body), start, i});
      continue;
    }

    if (is_punct_char(ch)) {
      out.push_back(SqlToken{SqlTokenKind::Punct, std::string(1, ch), i, i + 1});
      ++i;
      continue;
    }

    const auto start = i;
    while (i < sql.size()) {
      const auto c = sql[i];
      if (std::isspace(static_cast<unsigned char>(c)) != 0 || c == '\'' || c == '`' || is_punct_char(c)) {
        break;
      }
      ++i;
    }
    out.push_back(SqlToken{SqlTokenKind::Word, std::string(sql.substr(start, i - start)), start, i});
  }
  return out;
}

bool token_is_word(const SqlToken& tok, std::string_view text) {
  return tok.kind == SqlTokenKind::Word && iequals(tok.text, text);
}

std::vector<std::string> split_top_level_commas(std::string_view text) {
  std::vector<std::string> out;
  std::int64_t depth = 0;
  bool in_single = false;
  bool in_backtick = false;
  bool in_double = false;
  std::size_t part_start = 0;

  for (std::size_t i = 0; i < text.size(); ++i) {
    const auto ch = text[i];
    if (in_single) {
      if (ch == '\'') {
        if (i + 1 < text.size() && text[i + 1] == '\'') {
          ++i;
        } else {
          in_single = false;
        }
      }
      continue;
    }
    if (in_backtick) {
      if (ch == '`') {
        in_backtick = false;
      }
      continue;
    }
    if (in_double) {
      if (ch == '"') {
        if (i + 1 < text.size() && text[i + 1] == '"') {
          ++i;
        } else {
          in_double = false;
        }
      }
      continue;
    }

    if (ch == '\'') {
      in_single = true;
      continue;
    }
    if (ch == '`') {
      in_backtick = true;
      continue;
    }
    if (ch == '"') {
      in_double = true;
      continue;
    }
    if (ch == '(') {
      ++depth;
      continue;
    }
    if (ch == ')') {
      --depth;
      PLUSS_CHECK(depth >= 0, "DataFrame::sql().execute: unbalanced parentheses");
      continue;
    }
    if (ch == ',' && depth == 0) {
      out.push_back(trim_ascii(text.substr(part_start, i - part_start)));
      part_start = i + 1;
    }
  }
  PLUSS_CHECK(!in_single && !in_backtick && !in_double, "DataFrame::sql().execute: unterminated quoted token");
  PLUSS_CHECK(depth == 0, "DataFrame::sql().execute: unbalanced parentheses");
  out.push_back(trim_ascii(text.substr(part_start)));
  return out;
}

std::string parse_identifier_token(const SqlToken& tok, std::string_view context) {
  if (tok.kind == SqlTokenKind::Word || tok.kind == SqlTokenKind::Backtick) {
    PLUSS_CHECK(!tok.text.empty(), std::string(context) + ": identifier cannot be empty");
    return tok.text;
  }
  PLUSS_FAIL(std::string(context) + ": expected identifier");
}

SqlProjection parse_projection(std::string_view item_text, const DataFrame::SqlOptions& options) {
  const auto item = trim_ascii(item_text);
  PLUSS_CHECK(!item.empty(), "DataFrame::sql().execute: empty projection item");
  const auto toks = tokenize_sql(item);
  PLUSS_CHECK(!toks.empty(), "DataFrame::sql().execute: empty projection item");

  if (toks.size() == 1 && toks[0].kind == SqlTokenKind::Word && toks[0].text == "*") {
    PLUSS_CHECK(options.allow_select_star, "DataFrame::sql().execute: SELECT * is disabled by SqlOptions");
    return SqlProjection{true, "", ""};
  }

  if (toks.size() == 1) {
    const auto ident = parse_identifier_token(toks[0], "DataFrame::sql().execute SELECT");
    return SqlProjection{false, ident, ident};
  }

  if (toks.size() == 3 && token_is_word(toks[1], "as")) {
    const auto src = parse_identifier_token(toks[0], "DataFrame::sql().execute SELECT");
    const auto alias = parse_identifier_token(toks[2], "DataFrame::sql().execute SELECT AS");
    return SqlProjection{false, src, alias};
  }

  PLUSS_FAIL("DataFrame::sql().execute: unsupported projection item '" + item + "'");
}

SqlOrderSpec parse_order_clause(std::string_view clause_text) {
  const auto item = trim_ascii(clause_text);
  PLUSS_CHECK(!item.empty(), "DataFrame::sql().execute: ORDER BY clause cannot be empty");
  const auto parts = split_top_level_commas(item);
  PLUSS_CHECK(parts.size() == 1,
              "DataFrame::sql().execute: ORDER BY currently supports one feature key");

  const auto toks = tokenize_sql(parts[0]);
  PLUSS_CHECK(!toks.empty(), "DataFrame::sql().execute: ORDER BY clause cannot be empty");

  SqlOrderSpec spec;
  spec.feature = parse_identifier_token(toks[0], "DataFrame::sql().execute ORDER BY");

  bool seen_dir = false;
  bool seen_nulls = false;
  for (std::size_t i = 1; i < toks.size();) {
    const auto& tok = toks[i];
    PLUSS_CHECK(tok.kind == SqlTokenKind::Word,
                "DataFrame::sql().execute: unsupported ORDER BY token '" + tok.text + "'");
    if (token_is_word(tok, "asc")) {
      PLUSS_CHECK(!seen_dir, "DataFrame::sql().execute: duplicate ASC/DESC in ORDER BY");
      spec.ascending = true;
      seen_dir = true;
      ++i;
      continue;
    }
    if (token_is_word(tok, "desc")) {
      PLUSS_CHECK(!seen_dir, "DataFrame::sql().execute: duplicate ASC/DESC in ORDER BY");
      spec.ascending = false;
      seen_dir = true;
      ++i;
      continue;
    }
    if (token_is_word(tok, "nulls")) {
      PLUSS_CHECK(!seen_nulls, "DataFrame::sql().execute: duplicate NULLS FIRST/LAST in ORDER BY");
      PLUSS_CHECK(i + 1 < toks.size(), "DataFrame::sql().execute: NULLS must be followed by FIRST or LAST");
      if (token_is_word(toks[i + 1], "first")) {
        spec.nulls_last = false;
      } else if (token_is_word(toks[i + 1], "last")) {
        spec.nulls_last = true;
      } else {
        PLUSS_FAIL("DataFrame::sql().execute: NULLS must be followed by FIRST or LAST");
      }
      seen_nulls = true;
      i += 2;
      continue;
    }
    PLUSS_FAIL("DataFrame::sql().execute: unsupported ORDER BY token '" + tok.text + "'");
  }
  return spec;
}

std::int64_t parse_non_negative_int(std::string_view text, std::string_view context) {
  const auto t = trim_ascii(text);
  PLUSS_CHECK(!t.empty(), std::string(context) + " cannot be empty");
  std::size_t idx = 0;
  std::int64_t value = 0;
  try {
    value = std::stoll(t, &idx);
  } catch (...) {
    PLUSS_FAIL(std::string(context) + " must be an integer");
  }
  PLUSS_CHECK(idx == t.size(), std::string(context) + " must be an integer");
  PLUSS_CHECK(value >= 0, std::string(context) + " must be non-negative");
  return value;
}

bool source_is_accepted(std::string_view source, const DataFrame::SqlOptions& options) {
  if (options.accepted_sources.empty()) {
    return true;
  }
  return std::any_of(options.accepted_sources.begin(),
                     options.accepted_sources.end(),
                     [&](const std::string& name) { return iequals(source, name); });
}

SqlSelectSpec parse_select_sql(std::string_view statement, const DataFrame::SqlOptions& options) {
  auto sql = trim_ascii(statement);
  while (!sql.empty() && sql.back() == ';') {
    sql.pop_back();
    sql = trim_ascii(sql);
  }
  PLUSS_CHECK(!sql.empty(), "DataFrame::sql().execute: SQL statement cannot be empty");

  const auto toks = tokenize_sql(sql);
  PLUSS_CHECK(!toks.empty(), "DataFrame::sql().execute: SQL statement cannot be empty");
  PLUSS_CHECK(token_is_word(toks.front(), "select"),
              "DataFrame::sql().execute currently supports only SELECT statements");

  std::int64_t depth = 0;
  std::optional<std::size_t> from_idx;
  for (std::size_t i = 0; i < toks.size(); ++i) {
    const auto& tok = toks[i];
    if (tok.kind == SqlTokenKind::Punct && tok.text == "(") {
      ++depth;
      continue;
    }
    if (tok.kind == SqlTokenKind::Punct && tok.text == ")") {
      --depth;
      PLUSS_CHECK(depth >= 0, "DataFrame::sql().execute: unbalanced parentheses");
      continue;
    }
    if (depth == 0 && i > 0 && token_is_word(tok, "from")) {
      from_idx = i;
      break;
    }
  }
  PLUSS_CHECK(from_idx.has_value(), "DataFrame::sql().execute: SELECT statement must include FROM clause");

  const auto select_segment = trim_ascii(sql.substr(toks[0].end, toks[*from_idx].begin - toks[0].end));
  PLUSS_CHECK(!select_segment.empty(), "DataFrame::sql().execute: SELECT projection list cannot be empty");

  std::vector<SqlProjection> projections;
  for (const auto& item : split_top_level_commas(select_segment)) {
    projections.push_back(parse_projection(item, options));
  }
  PLUSS_CHECK(!projections.empty(), "DataFrame::sql().execute: SELECT projection list cannot be empty");
  const bool has_star = std::any_of(projections.begin(), projections.end(), [](const SqlProjection& p) {
    return p.star;
  });
  PLUSS_CHECK(!has_star || projections.size() == 1,
              "DataFrame::sql().execute: '*' cannot be mixed with named projections");

  depth = 0;
  std::vector<SqlClause> clauses;
  clauses.reserve(4);
  std::unordered_set<std::uint8_t> seen_clause;
  for (std::size_t i = *from_idx + 1; i < toks.size(); ++i) {
    const auto& tok = toks[i];
    if (tok.kind == SqlTokenKind::Punct && tok.text == "(") {
      ++depth;
      continue;
    }
    if (tok.kind == SqlTokenKind::Punct && tok.text == ")") {
      --depth;
      PLUSS_CHECK(depth >= 0, "DataFrame::sql().execute: unbalanced parentheses");
      continue;
    }
    if (depth != 0) {
      continue;
    }

    auto push_clause = [&](const SqlClauseKind kind, const std::size_t idx, const std::size_t kw_tokens) {
      const auto key = static_cast<std::uint8_t>(kind);
      PLUSS_CHECK(seen_clause.insert(key).second, "DataFrame::sql().execute: duplicate SQL clause");
      clauses.push_back(SqlClause{kind, idx, kw_tokens});
    };

    if (token_is_word(tok, "where")) {
      push_clause(SqlClauseKind::Where, i, 1);
      continue;
    }
    if (token_is_word(tok, "limit")) {
      push_clause(SqlClauseKind::Limit, i, 1);
      continue;
    }
    if (token_is_word(tok, "offset")) {
      push_clause(SqlClauseKind::Offset, i, 1);
      continue;
    }
    if (token_is_word(tok, "order")) {
      PLUSS_CHECK(i + 1 < toks.size() && token_is_word(toks[i + 1], "by"),
                  "DataFrame::sql().execute: ORDER clause must be followed by BY");
      push_clause(SqlClauseKind::OrderBy, i, 2);
      ++i;
      continue;
    }
  }

  std::sort(clauses.begin(), clauses.end(), [](const SqlClause& a, const SqlClause& b) {
    return a.token_index < b.token_index;
  });

  const auto table_end = clauses.empty() ? sql.size() : toks[clauses.front().token_index].begin;
  const auto table_segment = trim_ascii(sql.substr(toks[*from_idx].end, table_end - toks[*from_idx].end));
  PLUSS_CHECK(!table_segment.empty(), "DataFrame::sql().execute: FROM clause must include a source name");

  const auto table_toks = tokenize_sql(table_segment);
  PLUSS_CHECK(!table_toks.empty(), "DataFrame::sql().execute: invalid FROM clause");
  PLUSS_CHECK(table_toks.size() <= 2, "DataFrame::sql().execute: FROM clause supports one source with optional alias");
  PLUSS_CHECK(table_toks[0].kind == SqlTokenKind::Word || table_toks[0].kind == SqlTokenKind::Backtick,
              "DataFrame::sql().execute: invalid source identifier in FROM clause");
  PLUSS_CHECK(source_is_accepted(table_toks[0].text, options),
              "DataFrame::sql().execute: FROM source '" + table_toks[0].text + "' is not allowed by SqlOptions");
  if (table_toks.size() == 2) {
    PLUSS_CHECK(table_toks[1].kind == SqlTokenKind::Word || table_toks[1].kind == SqlTokenKind::Backtick,
                "DataFrame::sql().execute: invalid source alias in FROM clause");
  }

  SqlSelectSpec spec;
  spec.select_all = has_star;
  spec.projections = std::move(projections);

  for (std::size_t i = 0; i < clauses.size(); ++i) {
    const auto clause = clauses[i];
    const auto clause_keyword_end = toks[clause.token_index + clause.keyword_tokens - 1].end;
    const auto next_clause_start = (i + 1 < clauses.size()) ? toks[clauses[i + 1].token_index].begin : sql.size();
    const auto clause_body = trim_ascii(sql.substr(clause_keyword_end, next_clause_start - clause_keyword_end));
    PLUSS_CHECK(!clause_body.empty(), "DataFrame::sql().execute: SQL clause cannot be empty");

    switch (clause.kind) {
      case SqlClauseKind::Where:
        spec.where = clause_body;
        break;
      case SqlClauseKind::OrderBy:
        spec.order = parse_order_clause(clause_body);
        break;
      case SqlClauseKind::Limit:
        spec.limit = parse_non_negative_int(clause_body, "DataFrame::sql().execute LIMIT");
        break;
      case SqlClauseKind::Offset:
        spec.offset = parse_non_negative_int(clause_body, "DataFrame::sql().execute OFFSET");
        break;
    }
  }

  return spec;
}

bool is_case_projection(std::string_view source_name) {
  return iequals(source_name, "case");
}

DataFrame project_frame(const DataFrame& frame, const SqlSelectSpec& spec, const DataFrame::SqlOptions& options) {
  if (spec.select_all) {
    return frame;
  }

  std::vector<std::pair<std::string, Thread>> cols;
  cols.reserve(spec.projections.size());
  for (const auto& proj : spec.projections) {
    PLUSS_CHECK(!proj.star, "DataFrame::sql().execute: invalid '*' projection state");
    Thread col;
    if (is_case_projection(proj.source) && !frame.has_feature(proj.source)) {
      PLUSS_CHECK(options.allow_case_projection,
                  "DataFrame::sql().execute: case-axis projection is disabled by SqlOptions");
      col = frame.cases();
    } else {
      col = frame.column(proj.source);
    }
    cols.emplace_back(proj.output, std::move(col));
  }

  return DataFrame(std::move(cols), frame.cases());
}

} // namespace

DataFrameSqlAccessor::DataFrameSqlAccessor(DataFrame frame, DataFrame::SqlOptions options)
    : frame_(std::move(frame)), options_(std::move(options)) {}

DataFrame DataFrameSqlAccessor::execute(std::string_view statement) const {
  const auto spec = parse_select_sql(statement, options_);

  DataFrame out = frame_;
  if (spec.where.has_value()) {
    out = out.query(*spec.where);
  }
  if (spec.order.has_value()) {
    const auto& order = *spec.order;
    if (iequals(order.feature, "case") && !out.has_feature("case")) {
      out = out.sort_cases(order.ascending, order.nulls_last);
    } else {
      out = out.sort_values({
          DataFrame::ArrangeSpec{order.feature, order.ascending, order.nulls_last},
      });
    }
  }

  out = project_frame(out, spec, options_);

  const auto nrows = out.nrows();
  auto start = spec.offset.value_or(0);
  if (start > nrows) {
    start = nrows;
  }

  auto effective_limit = spec.limit;
  if (!effective_limit.has_value() && options_.default_limit.has_value()) {
    effective_limit = options_.default_limit;
  }
  if (effective_limit.has_value() && options_.max_limit.has_value()
      && *effective_limit > *options_.max_limit) {
    if (options_.clamp_limit_to_max) {
      effective_limit = options_.max_limit;
    } else {
      PLUSS_FAIL("DataFrame::sql().execute: LIMIT exceeds SqlOptions.max_limit");
    }
  }

  std::int64_t stop = nrows;
  if (effective_limit.has_value()) {
    if (*effective_limit > std::numeric_limits<std::int64_t>::max() - start) {
      stop = nrows;
    } else {
      stop = start + *effective_limit;
      if (stop > nrows) {
        stop = nrows;
      }
    }
  }

  if (start != 0 || stop != nrows) {
    out = out.slice(start, stop);
  }

  return out;
}

DataFrameSqlAccessor DataFrame::sql(SqlOptions options) const {
  return DataFrameSqlAccessor(*this, std::move(options));
}

DataFrameSqlAccessor DataFrame::sql() const {
  return DataFrameSqlAccessor(*this, SqlOptions{});
}

} // namespace pluss::table
