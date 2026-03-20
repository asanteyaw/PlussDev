#include "table/sql/DataFrameSql.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <limits>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "CodeM/core/Errors.h"
#include <fmt/format.h>

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

struct SqlSourceRef {
  std::string name{};
  std::string alias{};
};

struct SqlJoinSpec {
  SqlSourceRef source{};
  std::string on_left{};
  std::string on_right{};
};

struct SqlSelectSpec {
  bool select_all{false};
  SqlSourceRef primary_source{};
  std::vector<SqlJoinSpec> joins{};
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

SqlProjection parse_projection(std::string_view item_text) {
  const auto item = trim_ascii(item_text);
  PLUSS_CHECK(!item.empty(), "DataFrame::sql().execute: empty projection item");
  const auto toks = tokenize_sql(item);
  PLUSS_CHECK(!toks.empty(), "DataFrame::sql().execute: empty projection item");

  if (toks.size() == 1 && toks[0].kind == SqlTokenKind::Word && toks[0].text == "*") {
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

std::string escape_sql_string_literal(std::string_view s) {
  std::string out;
  out.reserve(s.size() + 8);
  for (const auto ch : s) {
    out.push_back(ch);
    if (ch == '\'') {
      out.push_back('\'');
    }
  }
  return out;
}

std::string escape_sql_identifier(std::string_view s) {
  std::string out;
  out.reserve(s.size() + 8);
  for (const auto ch : s) {
    out.push_back(ch);
    if (ch == '`') {
      out.push_back('`');
    }
  }
  return out;
}

bool is_safe_identifier_char(const char ch) {
  return std::isalnum(static_cast<unsigned char>(ch)) != 0 || ch == '_' || ch == '.';
}

void validate_identifier_value(std::string_view key, std::string_view ident) {
  PLUSS_CHECK(!ident.empty(),
              "DataFrame::sql().execute: identifier param '" + std::string(key) + "' cannot be empty");
  for (const auto ch : ident) {
    PLUSS_CHECK(is_safe_identifier_char(ch),
                "DataFrame::sql().execute: identifier param '" + std::string(key) + "' has invalid character");
  }
}

std::string sql_param_token(const DataFrame::SqlOptions::SqlParam& value) {
  if (const auto* p = std::get_if<std::nullptr_t>(&value)) {
    (void)p;
    return "null";
  }
  if (const auto* p = std::get_if<bool>(&value)) {
    return *p ? "true" : "false";
  }
  if (const auto* p = std::get_if<std::int64_t>(&value)) {
    return std::to_string(*p);
  }
  if (const auto* p = std::get_if<double>(&value)) {
    if (!std::isfinite(*p)) {
      return "null";
    }
    std::ostringstream oss;
    oss << std::setprecision(17) << *p;
    return oss.str();
  }
  const auto& p = std::get<std::string>(value);
  return "'" + escape_sql_string_literal(p) + "'";
}

std::string apply_sql_params(std::string_view statement, const DataFrame::SqlOptions& options) {
  if (options.params.empty() && options.identifier_params.empty()) {
    return std::string(statement);
  }

  std::unordered_set<std::string> used_scalar;
  std::unordered_set<std::string> used_identifier;
  std::string out;
  out.reserve(statement.size() + 32);
  for (std::size_t i = 0; i < statement.size();) {
    const auto ch = statement[i];
    if (ch == '{') {
      if (i + 1 < statement.size() && statement[i + 1] == '{') {
        out.push_back('{');
        i += 2;
        continue;
      }
      const auto close = statement.find('}', i + 1);
      PLUSS_CHECK(close != std::string_view::npos,
                  "DataFrame::sql().execute: unterminated '{' in SQL params template");
      const auto key = trim_ascii(statement.substr(i + 1, close - (i + 1)));
      PLUSS_CHECK(!key.empty(), "DataFrame::sql().execute: empty SQL params placeholder '{}'");

      if (key.size() > 1 && key[0] == '@') {
        const auto ident_key = trim_ascii(std::string_view(key).substr(1));
        PLUSS_CHECK(!ident_key.empty(),
                    "DataFrame::sql().execute: empty SQL identifier placeholder '{@...}'");
        const auto ident_it = options.identifier_params.find(ident_key);
        if (ident_it == options.identifier_params.end()) {
          if (options.missing_param_policy == DataFrame::SqlOptions::MissingParamPolicy::Null) {
            out += "null";
          } else {
            PLUSS_FAIL("DataFrame::sql().execute: SQL identifier params missing key '" + ident_key + "'");
          }
        } else {
          validate_identifier_value(ident_key, ident_it->second);
          used_identifier.insert(ident_key);
          out += "`" + escape_sql_identifier(ident_it->second) + "`";
        }
      } else {
        const auto it = options.params.find(key);
        if (it == options.params.end()) {
          if (options.missing_param_policy == DataFrame::SqlOptions::MissingParamPolicy::Null) {
            out += "null";
          } else {
            PLUSS_FAIL("DataFrame::sql().execute: SQL params missing key '" + key + "'");
          }
        } else {
          used_scalar.insert(key);
          out += sql_param_token(it->second);
        }
      }

      i = close + 1;
      continue;
    }
    if (ch == '}') {
      if (i + 1 < statement.size() && statement[i + 1] == '}') {
        out.push_back('}');
        i += 2;
        continue;
      }
      PLUSS_FAIL("DataFrame::sql().execute: unmatched '}' in SQL params template");
    }
    out.push_back(ch);
    ++i;
  }

  if (options.reject_unused_params) {
    for (const auto& [k, _] : options.params) {
      PLUSS_CHECK(used_scalar.find(k) != used_scalar.end(),
                  "DataFrame::sql().execute: unused SQL param key '" + k + "'");
    }
    for (const auto& [k, _] : options.identifier_params) {
      PLUSS_CHECK(used_identifier.find(k) != used_identifier.end(),
                  "DataFrame::sql().execute: unused SQL identifier param key '" + k + "'");
    }
  }

  return out;
}

bool is_identifier_token(const SqlToken& tok) {
  return tok.kind == SqlTokenKind::Word || tok.kind == SqlTokenKind::Backtick;
}

bool is_join_boundary_keyword(const SqlToken& tok) {
  return token_is_word(tok, "join") || token_is_word(tok, "inner") || token_is_word(tok, "left")
         || token_is_word(tok, "on");
}

SqlSourceRef parse_source_ref(const std::vector<SqlToken>& toks,
                              std::size_t* idx,
                              std::string_view context) {
  PLUSS_CHECK(idx != nullptr, std::string(context) + ": parser index cannot be null");
  PLUSS_CHECK(*idx < toks.size(), std::string(context) + ": missing source name");

  SqlSourceRef out;
  out.name = parse_identifier_token(toks[*idx], context);
  out.alias = out.name;
  ++(*idx);

  if (*idx < toks.size() && token_is_word(toks[*idx], "as")) {
    ++(*idx);
    PLUSS_CHECK(*idx < toks.size(), std::string(context) + ": missing source alias after AS");
    out.alias = parse_identifier_token(toks[*idx], context);
    ++(*idx);
    return out;
  }

  if (*idx < toks.size() && is_identifier_token(toks[*idx]) && !is_join_boundary_keyword(toks[*idx])) {
    out.alias = parse_identifier_token(toks[*idx], context);
    ++(*idx);
  }
  return out;
}

void parse_from_segment(std::string_view table_segment, SqlSelectSpec* spec) {
  PLUSS_CHECK(spec != nullptr, "DataFrame::sql().execute: internal parser state is null");
  const auto table_toks = tokenize_sql(table_segment);
  PLUSS_CHECK(!table_toks.empty(), "DataFrame::sql().execute: invalid FROM clause");

  std::size_t idx = 0;
  spec->primary_source = parse_source_ref(table_toks, &idx, "DataFrame::sql().execute FROM");
  spec->joins.clear();

  while (idx < table_toks.size()) {
    bool saw_left = false;
    if (token_is_word(table_toks[idx], "inner")) {
      ++idx;
    } else if (token_is_word(table_toks[idx], "left")) {
      saw_left = true;
      ++idx;
    }

    PLUSS_CHECK(idx < table_toks.size() && token_is_word(table_toks[idx], "join"),
                "DataFrame::sql().execute: expected JOIN in FROM clause");
    PLUSS_CHECK(!saw_left, "DataFrame::sql().execute: LEFT JOIN is not supported yet (use INNER JOIN)");
    ++idx;

    SqlJoinSpec join;
    join.source = parse_source_ref(table_toks, &idx, "DataFrame::sql().execute JOIN");

    PLUSS_CHECK(idx < table_toks.size() && token_is_word(table_toks[idx], "on"),
                "DataFrame::sql().execute: JOIN must include ON");
    ++idx;

    PLUSS_CHECK(idx < table_toks.size(), "DataFrame::sql().execute: JOIN ON missing left expression");
    join.on_left = parse_identifier_token(table_toks[idx], "DataFrame::sql().execute JOIN ON");
    ++idx;

    PLUSS_CHECK(idx < table_toks.size() && table_toks[idx].kind == SqlTokenKind::Word
                    && table_toks[idx].text == "=",
                "DataFrame::sql().execute: JOIN ON currently supports equality (`=`) only");
    ++idx;

    PLUSS_CHECK(idx < table_toks.size(), "DataFrame::sql().execute: JOIN ON missing right expression");
    join.on_right = parse_identifier_token(table_toks[idx], "DataFrame::sql().execute JOIN ON");
    ++idx;

    spec->joins.push_back(std::move(join));
  }
}

SqlSelectSpec parse_select_sql(std::string_view statement) {
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
    projections.push_back(parse_projection(item));
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

  SqlSelectSpec spec;
  spec.select_all = has_star;
  spec.projections = std::move(projections);
  parse_from_segment(table_segment, &spec);

  std::unordered_set<std::string> seen_aliases;
  auto alias_key = [&](const std::string& s) {
    std::string low;
    low.reserve(s.size());
    for (const auto ch : s) {
      low.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
    }
    return low;
  };
  PLUSS_CHECK(seen_aliases.insert(alias_key(spec.primary_source.alias)).second,
              "DataFrame::sql().execute: duplicate source alias '" + spec.primary_source.alias + "'");
  for (const auto& join : spec.joins) {
    PLUSS_CHECK(seen_aliases.insert(alias_key(join.source.alias)).second,
                "DataFrame::sql().execute: duplicate source alias '" + join.source.alias + "'");
  }

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

std::string explain_spec(const SqlSelectSpec& spec) {
  std::vector<std::string> lines;
  lines.push_back("SQL Plan:");
  lines.push_back(fmt::format("  from: {} as {}", spec.primary_source.name, spec.primary_source.alias));
  for (const auto& join : spec.joins) {
    lines.push_back(fmt::format("  join: {} as {} on {} = {}",
                                join.source.name,
                                join.source.alias,
                                join.on_left,
                                join.on_right));
  }
  if (spec.select_all) {
    lines.push_back("  select: *");
  } else {
    std::vector<std::string> pieces;
    pieces.reserve(spec.projections.size());
    for (const auto& p : spec.projections) {
      if (p.output == p.source) {
        pieces.push_back(p.source);
      } else {
        pieces.push_back(fmt::format("{} AS {}", p.source, p.output));
      }
    }
    std::string joined;
    for (std::size_t i = 0; i < pieces.size(); ++i) {
      joined += pieces[i];
      if (i + 1 < pieces.size()) joined += ", ";
    }
    lines.push_back("  select: " + joined);
  }
  if (spec.where.has_value()) lines.push_back("  where: " + *spec.where);
  if (spec.order.has_value()) {
    lines.push_back(fmt::format("  order: {} {} {}",
                                spec.order->feature,
                                spec.order->ascending ? "asc" : "desc",
                                spec.order->nulls_last ? "nulls last" : "nulls first"));
  }
  if (spec.offset.has_value()) lines.push_back(fmt::format("  offset: {}", *spec.offset));
  if (spec.limit.has_value()) lines.push_back(fmt::format("  limit: {}", *spec.limit));

  std::string out;
  for (std::size_t i = 0; i < lines.size(); ++i) {
    out += lines[i];
    if (i + 1 < lines.size()) out.push_back('\n');
  }
  return out;
}

using PlanCacheMap = std::unordered_map<std::string, SqlSelectSpec>;

PlanCacheMap& sql_plan_cache() {
  static PlanCacheMap cache;
  return cache;
}

std::vector<std::string>& sql_plan_cache_order() {
  static std::vector<std::string> order;
  return order;
}

std::mutex& sql_plan_cache_mutex() {
  static std::mutex m;
  return m;
}

SqlSelectSpec parse_select_sql_cached(std::string_view statement, const DataFrame::SqlOptions& options) {
  if (!options.enable_plan_cache || options.plan_cache_capacity == 0) {
    return parse_select_sql(statement);
  }

  const auto key = std::string(statement);
  {
    std::lock_guard<std::mutex> lock(sql_plan_cache_mutex());
    const auto it = sql_plan_cache().find(key);
    if (it != sql_plan_cache().end()) {
      return it->second;
    }
  }

  auto parsed = parse_select_sql(statement);
  {
    std::lock_guard<std::mutex> lock(sql_plan_cache_mutex());
    auto& cache = sql_plan_cache();
    auto& order = sql_plan_cache_order();
    auto [it, inserted] = cache.emplace(key, parsed);
    if (inserted) {
      order.push_back(key);
      while (cache.size() > options.plan_cache_capacity && !order.empty()) {
        cache.erase(order.front());
        order.erase(order.begin());
      }
    } else {
      it->second = parsed;
    }
  }
  return parsed;
}

bool is_case_projection(std::string_view source_name, const DataFrame::SqlOptions& options) {
  for (const auto& alias : options.case_axis_aliases) {
    if (iequals(source_name, alias)) {
      return true;
    }
  }
  return false;
}

std::string lower_ascii_copy(std::string_view s) {
  std::string out;
  out.reserve(s.size());
  for (const auto ch : s) {
    out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
  }
  return out;
}

std::pair<std::optional<std::string>, std::string> split_qualified_identifier(std::string_view ident) {
  const auto dot = ident.find('.');
  if (dot == std::string_view::npos) {
    return {std::nullopt, std::string(ident)};
  }
  PLUSS_CHECK(dot > 0 && dot + 1 < ident.size(),
              "DataFrame::sql().execute: invalid qualified identifier '" + std::string(ident) + "'");
  return {std::string(ident.substr(0, dot)), std::string(ident.substr(dot + 1))};
}

std::string resolve_feature_reference(std::string_view ident,
                                      const std::unordered_map<std::string, std::string>& source_prefixes) {
  const auto [prefix, feature] = split_qualified_identifier(ident);
  if (!prefix.has_value()) {
    return std::string(ident);
  }
  const auto it = source_prefixes.find(lower_ascii_copy(*prefix));
  if (it == source_prefixes.end()) {
    return std::string(ident);
  }
  if (it->second.empty()) {
    return feature;
  }
  return it->second + "." + feature;
}

using SqlClock = std::chrono::steady_clock;

std::int64_t elapsed_ms(const SqlClock::time_point& start) {
  return static_cast<std::int64_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(SqlClock::now() - start).count());
}

void enforce_timeout(const DataFrame::SqlOptions& options, const SqlClock::time_point& start, std::string_view stage) {
  if (!options.timeout_ms.has_value()) {
    return;
  }
  PLUSS_CHECK(*options.timeout_ms >= 0, "DataFrame::sql().execute: timeout_ms must be >= 0");
  PLUSS_CHECK(elapsed_ms(start) <= *options.timeout_ms,
              "DataFrame::sql().execute: timeout exceeded at stage '" + std::string(stage) + "'");
}

void enforce_row_limit(const DataFrame::SqlOptions& options,
                       const DataFrame& frame,
                       std::optional<std::int64_t> limit,
                       std::string_view stage) {
  if (!limit.has_value()) {
    return;
  }
  PLUSS_CHECK(*limit >= 0, "DataFrame::sql().execute: row limit must be >= 0");
  PLUSS_CHECK(frame.ncases() <= *limit,
              "DataFrame::sql().execute: " + std::string(stage) + " rows exceed configured limit");
}

std::int64_t estimate_frame_bytes(const DataFrame& frame) {
  constexpr std::int64_t kCellHint = 16;
  const auto cells = frame.ncases() * frame.nfeats();
  if (cells < 0) {
    return 0;
  }
  if (cells > std::numeric_limits<std::int64_t>::max() / kCellHint) {
    return std::numeric_limits<std::int64_t>::max();
  }
  return cells * kCellHint;
}

void enforce_memory_budget(const DataFrame::SqlOptions& options, const DataFrame& frame, std::string_view stage) {
  if (!options.memory_budget_mb.has_value()) {
    return;
  }
  PLUSS_CHECK(*options.memory_budget_mb > 0, "DataFrame::sql().execute: memory_budget_mb must be > 0");
  const auto budget_bytes = *options.memory_budget_mb * 1024LL * 1024LL;
  PLUSS_CHECK(estimate_frame_bytes(frame) <= budget_bytes,
              "DataFrame::sql().execute: estimated memory exceeds budget at stage '" + std::string(stage) + "'");
}

const DataFrame& resolve_source(std::string_view source_name,
                                const DataFrame& primary_frame,
                                const DataFrame::SqlOptions& options) {
  if (iequals(source_name, "df")) {
    return primary_frame;
  }
  for (const auto& binding : options.sources) {
    if (iequals(source_name, binding.name)) {
      PLUSS_CHECK(binding.frame != nullptr,
                  "DataFrame::sql().execute: source '" + binding.name + "' is bound to null DataFrame");
      return *binding.frame;
    }
  }
  PLUSS_FAIL("DataFrame::sql().execute: unknown source '" + std::string(source_name) + "'");
}

DataFrame with_prefixed_columns(const DataFrame& frame, std::string_view prefix) {
  std::vector<std::pair<std::string, Thread>> cols;
  const auto names = frame.feature_names();
  const auto values = frame.features();
  cols.reserve(names.size());
  for (std::size_t i = 0; i < names.size(); ++i) {
    cols.emplace_back(std::string(prefix) + names[i], values[i]);
  }
  return DataFrame(std::move(cols), frame.cases());
}

std::pair<std::string, std::string> resolve_join_keys(const DataFrame& left_frame,
                                                      const DataFrame& right_frame,
                                                      const std::string& right_source_name,
                                                      const std::string& right_alias,
                                                      std::string_view on_left,
                                                      std::string_view on_right,
                                                      const std::unordered_map<std::string, std::string>&
                                                          source_prefixes) {
  const auto [l_prefix, l_name] = split_qualified_identifier(on_left);
  const auto [r_prefix, r_name] = split_qualified_identifier(on_right);
  const auto right_alias_low = lower_ascii_copy(right_alias);
  const auto right_source_low = lower_ascii_copy(right_source_name);

  auto belongs_to_right = [&](const std::optional<std::string>& prefix) {
    if (!prefix.has_value()) {
      return false;
    }
    const auto low = lower_ascii_copy(*prefix);
    return low == right_alias_low || low == right_source_low;
  };

  const bool left_is_right = belongs_to_right(l_prefix);
  const bool right_is_right = belongs_to_right(r_prefix);
  PLUSS_CHECK(left_is_right != right_is_right,
              "DataFrame::sql().execute: JOIN ON must reference the joined source on exactly one side");

  std::string left_feature;
  std::string right_feature;
  if (left_is_right) {
    right_feature = l_name;
    left_feature = resolve_feature_reference(on_right, source_prefixes);
  } else {
    right_feature = r_name;
    left_feature = resolve_feature_reference(on_left, source_prefixes);
  }

  PLUSS_CHECK(left_frame.has_feature(left_feature),
              "DataFrame::sql().execute: JOIN left key not found '" + left_feature + "'");
  PLUSS_CHECK(right_frame.has_feature(right_feature),
              "DataFrame::sql().execute: JOIN right key not found '" + right_feature + "'");
  return {left_feature, right_feature};
}

DataFrame project_frame(const DataFrame& frame,
                        const SqlSelectSpec& spec,
                        const DataFrame::SqlOptions& options,
                        const std::unordered_map<std::string, std::string>& source_prefixes) {
  if (spec.select_all) {
    return frame;
  }

  std::vector<std::pair<std::string, Thread>> cols;
  std::unordered_map<std::string, std::size_t> name_to_idx;
  name_to_idx.reserve(spec.projections.size());
  cols.reserve(spec.projections.size());
  for (const auto& proj : spec.projections) {
    PLUSS_CHECK(!proj.star, "DataFrame::sql().execute: invalid '*' projection state");
    const auto resolved_source = resolve_feature_reference(proj.source, source_prefixes);
    Thread col;
    if (is_case_projection(proj.source, options) && !frame.has_feature(resolved_source)) {
      col = frame.cases();
    } else {
      col = frame.features(resolved_source);
    }
    auto out_name = proj.output;
    const auto it = name_to_idx.find(out_name);
    if (it == name_to_idx.end()) {
      name_to_idx.emplace(out_name, cols.size());
      cols.emplace_back(std::move(out_name), std::move(col));
      continue;
    }

    switch (options.collision_policy) {
      case DataFrame::SqlOptions::CollisionPolicy::Error:
        PLUSS_FAIL("DataFrame::sql().execute: duplicate projected column name '" + out_name + "'");
      case DataFrame::SqlOptions::CollisionPolicy::Overwrite:
        cols[it->second].second = std::move(col);
        break;
      case DataFrame::SqlOptions::CollisionPolicy::Suffix: {
        std::size_t k = 2;
        std::string candidate = fmt::format("{}_{}", out_name, k);
        while (name_to_idx.find(candidate) != name_to_idx.end()) {
          ++k;
          candidate = fmt::format("{}_{}", out_name, k);
        }
        name_to_idx.emplace(candidate, cols.size());
        cols.emplace_back(std::move(candidate), std::move(col));
        break;
      }
    }
  }

  return DataFrame(std::move(cols), frame.cases());
}

} // namespace

DataFrameSqlAccessor::DataFrameSqlAccessor(DataFrame frame, DataFrame::SqlOptions options)
    : frame_(std::move(frame)), options_(std::move(options)) {}

DataFrame DataFrameSqlAccessor::execute(std::string_view statement) const {
  const auto t0 = SqlClock::now();
  const auto rendered_statement = apply_sql_params(statement, options_);
  const auto t1 = SqlClock::now();
  const auto spec = parse_select_sql_cached(rendered_statement, options_);
  const auto t2 = SqlClock::now();

  if (options_.diagnostics_trace_tokens) {
    const auto toks = tokenize_sql(rendered_statement);
    std::string token_dump;
    token_dump.reserve(toks.size() * 12);
    for (std::size_t i = 0; i < toks.size(); ++i) {
      token_dump += toks[i].text;
      if (i + 1 < toks.size()) token_dump += " | ";
    }
    ::pluss::codem::warn("sql.trace.tokens: " + token_dump);
  }
  if (options_.diagnostics_explain || options_.diagnostics_trace_plan) {
    ::pluss::codem::warn(explain_spec(spec));
  }

  const auto& primary = resolve_source(spec.primary_source.name, frame_, options_);
  DataFrame out = primary;
  enforce_row_limit(options_, out, options_.max_intermediate_rows, "intermediate");
  enforce_memory_budget(options_, out, "initial");

  std::unordered_map<std::string, std::string> source_prefixes;
  source_prefixes.emplace(lower_ascii_copy(spec.primary_source.name), "");
  source_prefixes.emplace(lower_ascii_copy(spec.primary_source.alias), "");

  bool includes_calling_frame = (&primary == &frame_);
  for (const auto& join : spec.joins) {
    enforce_timeout(options_, t0, "before_join");
    const auto& right_src = resolve_source(join.source.name, frame_, options_);
    includes_calling_frame = includes_calling_frame || (&right_src == &frame_);

    auto [left_key, right_key] =
        resolve_join_keys(out, right_src, join.source.name, join.source.alias, join.on_left, join.on_right, source_prefixes);
    const auto right_prefixed = with_prefixed_columns(right_src, join.source.alias + ".");
    out = out.stitch_on(left_key, right_prefixed, join.source.alias + "." + right_key, "_" + join.source.alias);

    source_prefixes[lower_ascii_copy(join.source.name)] = join.source.alias;
    source_prefixes[lower_ascii_copy(join.source.alias)] = join.source.alias;
    enforce_row_limit(options_, out, options_.max_intermediate_rows, "intermediate");
    enforce_memory_budget(options_, out, "join");
  }

  PLUSS_CHECK(includes_calling_frame,
              "DataFrame::sql().execute: calling dataframe source is not part of FROM/JOIN sources");

  enforce_timeout(options_, t0, "before_where");
  if (spec.where.has_value()) {
    out = out.query(*spec.where);
    enforce_row_limit(options_, out, options_.max_intermediate_rows, "intermediate");
    enforce_memory_budget(options_, out, "where");
  }
  enforce_timeout(options_, t0, "before_order");
  if (spec.order.has_value()) {
    auto order_feature = resolve_feature_reference(spec.order->feature, source_prefixes);
    if (iequals(order_feature, "case") && !out.has_feature("case")) {
      out = out.sort_cases(spec.order->ascending, spec.order->nulls_last);
    } else {
      out = out.sort_values({
          DataFrame::ArrangeSpec{order_feature, spec.order->ascending, spec.order->nulls_last},
      });
    }
    enforce_row_limit(options_, out, options_.max_intermediate_rows, "intermediate");
    enforce_memory_budget(options_, out, "order");
  }

  enforce_timeout(options_, t0, "before_projection");
  out = project_frame(out, spec, options_, source_prefixes);
  enforce_row_limit(options_, out, options_.max_intermediate_rows, "intermediate");
  enforce_memory_budget(options_, out, "projection");

  const auto nrows = out.ncases();
  auto start = spec.offset.value_or(0);
  if (start > nrows) {
    start = nrows;
  }

  std::int64_t stop = nrows;
  if (spec.limit.has_value()) {
    if (*spec.limit > std::numeric_limits<std::int64_t>::max() - start) {
      stop = nrows;
    } else {
      stop = start + *spec.limit;
      if (stop > nrows) {
        stop = nrows;
      }
    }
  }

  if (start != 0 || stop != nrows) {
    out = out.slice(start, stop);
  }

  if (options_.max_result_rows.has_value() && out.ncases() > *options_.max_result_rows) {
    out = out.slice(0, *options_.max_result_rows);
  }
  if (options_.result_mode == DataFrame::SqlOptions::ResultMode::Chunked) {
    ::pluss::codem::warn(fmt::format("sql.result_mode=Chunked requested (chunk_rows={}) but execute() "
                                     "returns a materialized DataFrame",
                                     options_.chunk_rows));
  }
  enforce_memory_budget(options_, out, "result");

  enforce_timeout(options_, t0, "finalize");
  const auto t3 = SqlClock::now();
  if (options_.diagnostics_profile) {
    const auto render_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    const auto parse_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
    const auto exec_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t3 - t2).count();
    const auto total_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t3 - t0).count();
    ::pluss::codem::warn(
        fmt::format("sql.profile: render={}ms parse={}ms exec={}ms total={}ms rows={}",
                    render_ms,
                    parse_ms,
                    exec_ms,
                    total_ms,
                    out.ncases()));
  }

  return out;
}

DataFrameSqlPrepared DataFrameSqlAccessor::prepare(std::string_view statement) const {
  return DataFrameSqlPrepared(frame_, options_, std::string(statement));
}

DataFrameSqlPrepared::DataFrameSqlPrepared(DataFrame frame,
                                           DataFrame::SqlOptions options,
                                           std::string statement_template)
    : frame_(std::move(frame)),
      options_(std::move(options)),
      statement_template_(std::move(statement_template)) {}

DataFrame DataFrameSqlPrepared::execute() const {
  return DataFrameSqlAccessor(frame_, options_).execute(statement_template_);
}

DataFrame DataFrameSqlPrepared::execute(
    std::unordered_map<std::string, DataFrame::SqlOptions::SqlParam> params) const {
  auto opts = options_;
  for (auto& [k, v] : params) {
    opts.params[k] = std::move(v);
  }
  return DataFrameSqlAccessor(frame_, std::move(opts)).execute(statement_template_);
}

DataFrame DataFrameSqlPrepared::execute(
    std::unordered_map<std::string, DataFrame::SqlOptions::SqlParam> params,
    std::unordered_map<std::string, std::string> identifier_params) const {
  auto opts = options_;
  for (auto& [k, v] : params) {
    opts.params[k] = std::move(v);
  }
  for (auto& [k, v] : identifier_params) {
    opts.identifier_params[k] = std::move(v);
  }
  return DataFrameSqlAccessor(frame_, std::move(opts)).execute(statement_template_);
}

DataFrameSqlAccessor DataFrame::sql(SqlOptions options) const {
  return DataFrameSqlAccessor(*this, std::move(options));
}

DataFrameSqlAccessor DataFrame::sql() const {
  return DataFrameSqlAccessor(*this, SqlOptions{});
}

DataFrameSqlPrepared DataFrame::prepare_sql(std::string_view statement) const {
  return DataFrameSqlAccessor(*this, SqlOptions{}).prepare(statement);
}

DataFrameSqlPrepared DataFrame::prepare_sql(std::string_view statement, SqlOptions options) const {
  return DataFrameSqlAccessor(*this, std::move(options)).prepare(statement);
}

} // namespace pluss::table
