#include "table/sql/DataFrameSql.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <limits>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

#include <sqlite3.h>

#include "CodeM/core/Errors.h"
#include <fmt/format.h>

namespace pluss::table {

namespace {

constexpr std::string_view kPrimarySourceName = "df";
constexpr std::string_view kInternalCaseColumn = "__pluss_case__";

enum class SqlTokenKind : std::uint8_t { Word = 0, String = 1, Backtick = 2, Punct = 3 };

struct SqlToken {
  SqlTokenKind kind{SqlTokenKind::Word};
  std::string text{};
  std::size_t begin{0};
  std::size_t end{0};
};

struct ProjectionHints {
  bool parsed{false};
  bool star_only{false};
  std::vector<std::string> output_names{};
};

struct TimeoutContext {
  std::chrono::steady_clock::time_point start{};
  const DataFrame::SqlOptions* options{nullptr};
};

using SqlCell = std::variant<std::monostate, std::int64_t, double, std::string>;

struct ColumnCollector {
  std::string name{};
  std::optional<std::string> declared_type{};
  std::vector<SqlCell> values{};
};

struct SqliteDbCloser {
  void operator()(sqlite3* db) const noexcept {
    if (db != nullptr) {
      sqlite3_close(db);
    }
  }
};

struct SqliteStmtFinalizer {
  void operator()(sqlite3_stmt* stmt) const noexcept {
    if (stmt != nullptr) {
      sqlite3_finalize(stmt);
    }
  }
};

using SqliteDb = std::unique_ptr<sqlite3, SqliteDbCloser>;
using SqliteStmt = std::unique_ptr<sqlite3_stmt, SqliteStmtFinalizer>;

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

std::string lower_ascii_copy(std::string_view s) {
  std::string out;
  out.reserve(s.size());
  for (const auto ch : s) {
    out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
  }
  return out;
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

bool is_punct_char(const char ch) {
  return ch == '(' || ch == ')' || ch == ',' || ch == ';';
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
    if (tok.text.size() >= 2) {
      const auto first = tok.text.front();
      const auto last = tok.text.back();
      if ((first == '"' && last == '"') || (first == '`' && last == '`')) {
        return tok.text.substr(1, tok.text.size() - 2);
      }
    }
    return tok.text;
  }
  PLUSS_FAIL(std::string(context) + ": expected identifier");
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
    if (ch == '"') {
      out.push_back('"');
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
          out += "`" + ident_it->second + "`";
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

bool is_simple_identifier(std::string_view text) {
  if (text.empty()) {
    return false;
  }
  for (const auto ch : text) {
    if (!is_safe_identifier_char(ch)) {
      return false;
    }
  }
  return true;
}

std::string maybe_expand_table_name(std::string_view statement, const DataFrame::SqlOptions& options) {
  const auto trimmed = trim_ascii(statement);
  if (trimmed.empty()) {
    return trimmed;
  }
  if (!is_simple_identifier(trimmed)) {
    return trimmed;
  }

  if (iequals(trimmed, kPrimarySourceName)) {
    return "SELECT * FROM df";
  }
  for (const auto& alias : options.primary_source_names) {
    if (iequals(trimmed, alias)) {
      return "SELECT * FROM \"" + escape_sql_identifier(alias) + "\"";
    }
  }
  for (const auto& binding : options.sources) {
    if (iequals(trimmed, binding.name)) {
      return "SELECT * FROM \"" + escape_sql_identifier(binding.name) + "\"";
    }
  }
  return trimmed;
}

ProjectionHints parse_projection_hints(std::string_view statement) {
  ProjectionHints hints;
  const auto sql = trim_ascii(statement);
  if (sql.empty()) {
    return hints;
  }

  const auto toks = tokenize_sql(sql);
  if (toks.empty()) {
    return hints;
  }

  std::int64_t depth = 0;
  std::optional<std::size_t> select_idx;
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
    if (depth == 0 && !select_idx.has_value() && token_is_word(tok, "select")) {
      select_idx = i;
      continue;
    }
    if (depth == 0 && select_idx.has_value() && i > *select_idx && token_is_word(tok, "from")) {
      from_idx = i;
      break;
    }
  }
  if (!select_idx.has_value() || !from_idx.has_value()) {
    return hints;
  }

  const auto select_segment =
      trim_ascii(sql.substr(toks[*select_idx].end, toks[*from_idx].begin - toks[*select_idx].end));
  if (select_segment.empty()) {
    return hints;
  }

  hints.parsed = true;
  const auto items = split_top_level_commas(select_segment);
  hints.star_only = !items.empty();
  for (const auto& item_raw : items) {
    const auto item = trim_ascii(item_raw);
    if (item == "*" || (item.size() > 2 && item.substr(item.size() - 2) == ".*")) {
      continue;
    }
    hints.star_only = false;
    const auto ptoks = tokenize_sql(item);
    if (ptoks.size() == 1 && (ptoks[0].kind == SqlTokenKind::Word || ptoks[0].kind == SqlTokenKind::Backtick)) {
      hints.output_names.push_back(parse_identifier_token(ptoks[0], "DataFrame::sql().execute SELECT"));
      continue;
    }
    if (ptoks.size() >= 3 && token_is_word(ptoks[ptoks.size() - 2], "as")
        && (ptoks.back().kind == SqlTokenKind::Word || ptoks.back().kind == SqlTokenKind::Backtick)) {
      hints.output_names.push_back(parse_identifier_token(ptoks.back(), "DataFrame::sql().execute SELECT AS"));
      continue;
    }
    hints.output_names.clear();
    return hints;
  }

  if (items.size() == 1 && (trim_ascii(items.front()) == "*" || trim_ascii(items.front()).ends_with(".*"))) {
    hints.star_only = true;
  }
  return hints;
}

std::string format_explain_query_plan(sqlite3* db, const std::string& statement) {
  sqlite3_stmt* raw = nullptr;
  const auto explain_sql = "EXPLAIN QUERY PLAN " + statement;
  const auto rc = sqlite3_prepare_v2(db, explain_sql.c_str(), -1, &raw, nullptr);
  PLUSS_CHECK(rc == SQLITE_OK,
              "DataFrame::sql().execute: failed to explain SQL plan: " + std::string(sqlite3_errmsg(db)));
  SqliteStmt stmt(raw);

  std::vector<std::string> lines;
  while ((sqlite3_step(stmt.get())) == SQLITE_ROW) {
    const auto* detail = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 3));
    lines.push_back(detail != nullptr ? std::string(detail) : std::string());
  }

  std::string out = "SQL Plan:";
  for (const auto& line : lines) {
    out += "\n  " + line;
  }
  return out;
}

std::string sql_identifier(std::string_view value) {
  return "\"" + escape_sql_identifier(value) + "\"";
}

std::string source_internal_table_name(std::size_t index) {
  return fmt::format("__pluss_src_{}__", index);
}

bool has_feature_named(const DataFrame& frame, std::string_view name) {
  return frame.get(name).has_value();
}

std::vector<std::string> visible_case_aliases(const DataFrame& frame,
                                              const std::vector<std::string>& aliases) {
  std::vector<std::string> out;
  out.reserve(aliases.size());
  for (const auto& alias : aliases) {
    if (!has_feature_named(frame, alias)) {
      out.push_back(alias);
    }
  }
  return out;
}

std::string series_scalar_text(const pluss::Series& s, std::int64_t row) {
  const auto* base = s.raw_data();
  switch (s.scalar_type()) {
    case pluss::codem::ScalarType::Bool:
      return reinterpret_cast<const bool*>(base)[row] ? "true" : "false";
    case pluss::codem::ScalarType::Int8:
      return fmt::format("{}", static_cast<int>(reinterpret_cast<const std::int8_t*>(base)[row]));
    case pluss::codem::ScalarType::Int16:
      return fmt::format("{}", reinterpret_cast<const std::int16_t*>(base)[row]);
    case pluss::codem::ScalarType::Int32:
      return fmt::format("{}", reinterpret_cast<const std::int32_t*>(base)[row]);
    case pluss::codem::ScalarType::Int64:
      return fmt::format("{}", reinterpret_cast<const std::int64_t*>(base)[row]);
    case pluss::codem::ScalarType::UInt8:
      return fmt::format("{}", static_cast<unsigned>(reinterpret_cast<const std::uint8_t*>(base)[row]));
    case pluss::codem::ScalarType::UInt16:
      return fmt::format("{}", reinterpret_cast<const std::uint16_t*>(base)[row]);
    case pluss::codem::ScalarType::UInt32:
      return fmt::format("{}", reinterpret_cast<const std::uint32_t*>(base)[row]);
    case pluss::codem::ScalarType::UInt64:
      return fmt::format("{}", reinterpret_cast<const std::uint64_t*>(base)[row]);
    case pluss::codem::ScalarType::Float16:
      return fmt::format("{:.6g}",
                         pluss::codem::float16_to_float(reinterpret_cast<const pluss::codem::Float16*>(base)[row]));
    case pluss::codem::ScalarType::BFloat16:
      return fmt::format("{:.6g}",
                         pluss::codem::bfloat16_to_float(reinterpret_cast<const pluss::codem::BFloat16*>(base)[row]));
    case pluss::codem::ScalarType::Float32:
      return fmt::format("{:.8g}", reinterpret_cast<const float*>(base)[row]);
    case pluss::codem::ScalarType::Float64:
      return fmt::format("{:.12g}", reinterpret_cast<const double*>(base)[row]);
    case pluss::codem::ScalarType::Complex64: {
      const auto v = reinterpret_cast<const pluss::codem::Complex64*>(base)[row];
      return fmt::format("({:.6g}{:+.6g}j)", v.real(), v.imag());
    }
    case pluss::codem::ScalarType::Complex128: {
      const auto v = reinterpret_cast<const pluss::codem::Complex128*>(base)[row];
      return fmt::format("({:.6g}{:+.6g}j)", v.real(), v.imag());
    }
    default:
      return "";
  }
}

bool row_is_valid(const Thread& thread, std::int64_t row) {
  if (detail::has_validity_mask(thread) &&
      !detail::validity_mask(thread)->to_vec<bool>()[static_cast<std::size_t>(row)]) {
    return false;
  }
  return thread.kind() != ThreadKind::Duration ||
         thread.as_duration().host()[static_cast<std::size_t>(row)] !=
             pluss::Duration::kNaT;
}

void bind_sqlite_null(sqlite3_stmt* stmt, const int index) {
  PLUSS_CHECK(sqlite3_bind_null(stmt, index) == SQLITE_OK,
              "DataFrame::sql().execute: sqlite bind null failed");
}

void bind_sqlite_text(sqlite3_stmt* stmt, const int index, const std::string& text) {
  PLUSS_CHECK(sqlite3_bind_text(stmt, index, text.c_str(), static_cast<int>(text.size()), SQLITE_TRANSIENT) == SQLITE_OK,
              "DataFrame::sql().execute: sqlite bind text failed");
}

void bind_thread_value(sqlite3_stmt* stmt, const int index, const Thread& thread, const std::int64_t row) {
  if (!row_is_valid(thread, row)) {
    bind_sqlite_null(stmt, index);
    return;
  }

  switch (thread.kind()) {
    case ThreadKind::Series: {
      const auto& series = thread.as_series();
      const auto* base = series.raw_data();
      switch (series.scalar_type()) {
        case pluss::codem::ScalarType::Bool:
          PLUSS_CHECK(sqlite3_bind_int64(stmt, index, reinterpret_cast<const bool*>(base)[row] ? 1 : 0) == SQLITE_OK,
                      "DataFrame::sql().execute: sqlite bind bool failed");
          return;
        case pluss::codem::ScalarType::Int8:
          PLUSS_CHECK(sqlite3_bind_int64(stmt, index, reinterpret_cast<const std::int8_t*>(base)[row]) == SQLITE_OK,
                      "DataFrame::sql().execute: sqlite bind int8 failed");
          return;
        case pluss::codem::ScalarType::Int16:
          PLUSS_CHECK(sqlite3_bind_int64(stmt, index, reinterpret_cast<const std::int16_t*>(base)[row]) == SQLITE_OK,
                      "DataFrame::sql().execute: sqlite bind int16 failed");
          return;
        case pluss::codem::ScalarType::Int32:
          PLUSS_CHECK(sqlite3_bind_int64(stmt, index, reinterpret_cast<const std::int32_t*>(base)[row]) == SQLITE_OK,
                      "DataFrame::sql().execute: sqlite bind int32 failed");
          return;
        case pluss::codem::ScalarType::Int64:
          PLUSS_CHECK(sqlite3_bind_int64(stmt, index, reinterpret_cast<const std::int64_t*>(base)[row]) == SQLITE_OK,
                      "DataFrame::sql().execute: sqlite bind int64 failed");
          return;
        case pluss::codem::ScalarType::UInt8:
          PLUSS_CHECK(sqlite3_bind_int64(stmt, index, reinterpret_cast<const std::uint8_t*>(base)[row]) == SQLITE_OK,
                      "DataFrame::sql().execute: sqlite bind uint8 failed");
          return;
        case pluss::codem::ScalarType::UInt16:
          PLUSS_CHECK(sqlite3_bind_int64(stmt, index, reinterpret_cast<const std::uint16_t*>(base)[row]) == SQLITE_OK,
                      "DataFrame::sql().execute: sqlite bind uint16 failed");
          return;
        case pluss::codem::ScalarType::UInt32:
          PLUSS_CHECK(sqlite3_bind_int64(stmt, index, reinterpret_cast<const std::uint32_t*>(base)[row]) == SQLITE_OK,
                      "DataFrame::sql().execute: sqlite bind uint32 failed");
          return;
        case pluss::codem::ScalarType::UInt64:
          PLUSS_CHECK(sqlite3_bind_int64(stmt, index, static_cast<sqlite3_int64>(reinterpret_cast<const std::uint64_t*>(base)[row])) == SQLITE_OK,
                      "DataFrame::sql().execute: sqlite bind uint64 failed");
          return;
        case pluss::codem::ScalarType::Float16:
          PLUSS_CHECK(sqlite3_bind_double(stmt,
                                          index,
                                          pluss::codem::float16_to_float(reinterpret_cast<const pluss::codem::Float16*>(base)[row])) == SQLITE_OK,
                      "DataFrame::sql().execute: sqlite bind float16 failed");
          return;
        case pluss::codem::ScalarType::BFloat16:
          PLUSS_CHECK(sqlite3_bind_double(stmt,
                                          index,
                                          pluss::codem::bfloat16_to_float(reinterpret_cast<const pluss::codem::BFloat16*>(base)[row])) == SQLITE_OK,
                      "DataFrame::sql().execute: sqlite bind bfloat16 failed");
          return;
        case pluss::codem::ScalarType::Float32:
          PLUSS_CHECK(sqlite3_bind_double(stmt, index, reinterpret_cast<const float*>(base)[row]) == SQLITE_OK,
                      "DataFrame::sql().execute: sqlite bind float32 failed");
          return;
        case pluss::codem::ScalarType::Float64:
          PLUSS_CHECK(sqlite3_bind_double(stmt, index, reinterpret_cast<const double*>(base)[row]) == SQLITE_OK,
                      "DataFrame::sql().execute: sqlite bind float64 failed");
          return;
        default:
          bind_sqlite_text(stmt, index, series_scalar_text(series, row));
          return;
      }
    }
    case ThreadKind::Note:
      bind_sqlite_text(stmt, index, std::string(thread.as_note().at(row)));
      return;
    case ThreadKind::Timeline:
      PLUSS_CHECK(sqlite3_bind_int64(
                      stmt,
                      index,
                      static_cast<sqlite3_int64>(thread.as_timeline().ticks()[static_cast<std::size_t>(row)]))
                      == SQLITE_OK,
                  "DataFrame::sql().execute: sqlite bind timeline failed");
      return;
    case ThreadKind::Duration:
      PLUSS_CHECK(sqlite3_bind_int64(
                      stmt,
                      index,
                      static_cast<sqlite3_int64>(
                          thread.as_duration().host()[static_cast<std::size_t>(row)]))
                      == SQLITE_OK,
                  "DataFrame::sql().execute: sqlite bind duration failed");
      return;
  }
}

std::string sqlite_declared_type(const Thread& thread) {
  switch (thread.kind()) {
    case ThreadKind::Series: {
      const auto dt = thread.as_series().dtype();
      if (dt.scalar == pluss::codem::ScalarType::Bool || dt.is_integral()) {
        return "INTEGER";
      }
      if (dt.is_floating()) {
        return "REAL";
      }
      return "TEXT";
    }
    case ThreadKind::Timeline:
    case ThreadKind::Duration:
      return "INTEGER";
    case ThreadKind::Note:
      return "TEXT";
  }
}

void exec_sql(sqlite3* db, std::string_view sql, std::string_view where) {
  char* errmsg = nullptr;
  const auto rc = sqlite3_exec(db, std::string(sql).c_str(), nullptr, nullptr, &errmsg);
  if (rc != SQLITE_OK) {
    const std::string detail = (errmsg != nullptr) ? std::string(errmsg) : std::string(sqlite3_errmsg(db));
    if (errmsg != nullptr) {
      sqlite3_free(errmsg);
    }
    PLUSS_FAIL(std::string(where) + ": " + detail);
  }
}

std::optional<pluss::Series> maybe_validity(const std::vector<bool>& valid) {
  if (std::all_of(valid.begin(), valid.end(), [](const bool v) { return v; })) {
    return std::nullopt;
  }
  return pluss::make_series(valid);
}

std::string hex_blob(const void* data, const int nbytes) {
  const auto* bytes = reinterpret_cast<const unsigned char*>(data);
  static constexpr char kHex[] = "0123456789ABCDEF";
  std::string out;
  out.reserve(static_cast<std::size_t>(nbytes) * 2);
  for (int i = 0; i < nbytes; ++i) {
    out.push_back(kHex[(bytes[i] >> 4) & 0xF]);
    out.push_back(kHex[bytes[i] & 0xF]);
  }
  return out;
}

bool declared_type_is_integer(std::string_view decl) {
  const auto low = lower_ascii_copy(decl);
  return low.find("int") != std::string::npos;
}

bool declared_type_is_float(std::string_view decl) {
  const auto low = lower_ascii_copy(decl);
  return low.find("real") != std::string::npos || low.find("float") != std::string::npos
         || low.find("double") != std::string::npos || low.find("numeric") != std::string::npos
         || low.find("decimal") != std::string::npos;
}

Thread build_column_thread(const ColumnCollector& collector) {
  bool saw_text = false;
  bool saw_double = false;
  bool saw_int = false;
  for (const auto& cell : collector.values) {
    if (std::holds_alternative<std::string>(cell)) {
      saw_text = true;
      break;
    }
    if (std::holds_alternative<double>(cell)) {
      saw_double = true;
    } else if (std::holds_alternative<std::int64_t>(cell)) {
      saw_int = true;
    }
  }

  if (!saw_text && !saw_double && !saw_int && collector.declared_type.has_value()) {
    saw_int = declared_type_is_integer(*collector.declared_type);
    saw_double = !saw_int && declared_type_is_float(*collector.declared_type);
  }

  std::vector<bool> valid;
  valid.reserve(collector.values.size());
  for (const auto& cell : collector.values) {
    valid.push_back(!std::holds_alternative<std::monostate>(cell));
  }
  const auto valid_mask = maybe_validity(valid);

  if (saw_text || (!saw_double && !saw_int)) {
    std::vector<std::string> values;
    values.reserve(collector.values.size());
    for (const auto& cell : collector.values) {
      if (const auto* p = std::get_if<std::string>(&cell)) {
        values.push_back(*p);
      } else if (const auto* p = std::get_if<double>(&cell)) {
        values.push_back(fmt::format("{:.17g}", *p));
      } else if (const auto* p = std::get_if<std::int64_t>(&cell)) {
        values.push_back(std::to_string(*p));
      } else {
        values.emplace_back();
      }
    }
    return valid_mask.has_value() ? Thread(pluss::make_note(values), *valid_mask) : Thread(pluss::make_note(values));
  }

  if (saw_double) {
    std::vector<double> values;
    values.reserve(collector.values.size());
    for (const auto& cell : collector.values) {
      if (const auto* p = std::get_if<double>(&cell)) {
        values.push_back(*p);
      } else if (const auto* p = std::get_if<std::int64_t>(&cell)) {
        values.push_back(static_cast<double>(*p));
      } else {
        values.push_back(0.0);
      }
    }
    return valid_mask.has_value() ? Thread(pluss::make_series(values), *valid_mask) : Thread(pluss::make_series(values));
  }

  std::vector<std::int64_t> values;
  values.reserve(collector.values.size());
  for (const auto& cell : collector.values) {
    if (const auto* p = std::get_if<std::int64_t>(&cell)) {
      values.push_back(*p);
    } else if (const auto* p = std::get_if<double>(&cell)) {
      values.push_back(static_cast<std::int64_t>(*p));
    } else {
      values.push_back(0);
    }
  }
  return valid_mask.has_value() ? Thread(pluss::make_series(values), *valid_mask) : Thread(pluss::make_series(values));
}

std::int64_t estimate_frame_bytes(const DataFrame& frame) {
  constexpr std::int64_t kCellHint = 16;
  const auto cells = frame.ncases() * frame.nfeatures();
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

int sqlite_progress_callback(void* raw) {
  auto* ctx = static_cast<TimeoutContext*>(raw);
  if (ctx == nullptr || ctx->options == nullptr || !ctx->options->timeout_ms.has_value()) {
    return 0;
  }
  const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now() - ctx->start).count();
  return elapsed > *ctx->options->timeout_ms ? 1 : 0;
}

void attach_timeout_handler(sqlite3* db, TimeoutContext* ctx) {
  if (ctx == nullptr || ctx->options == nullptr || !ctx->options->timeout_ms.has_value()) {
    sqlite3_progress_handler(db, 0, nullptr, nullptr);
    return;
  }
  PLUSS_CHECK(*ctx->options->timeout_ms >= 0, "DataFrame::sql().execute: timeout_ms must be >= 0");
  sqlite3_progress_handler(db, 1000, sqlite_progress_callback, ctx);
}

void load_source(sqlite3* db,
                 const std::string& source_name,
                 const DataFrame& frame,
                 const std::vector<std::string>& case_aliases,
                 const std::size_t source_index) {
  const auto internal_name = source_internal_table_name(source_index);
  std::vector<std::string> create_parts;
  const auto feature_names = frame.features();
  create_parts.reserve(feature_names.size() + 1);
  for (const auto& feature_name : feature_names) {
    create_parts.push_back(sql_identifier(feature_name) + " " + sqlite_declared_type(frame.features(feature_name)));
  }
  create_parts.push_back(sql_identifier(kInternalCaseColumn) + " TEXT");

  std::string create_sql = "CREATE TEMP TABLE " + sql_identifier(internal_name) + " (";
  for (std::size_t i = 0; i < create_parts.size(); ++i) {
    create_sql += create_parts[i];
    if (i + 1 < create_parts.size()) {
      create_sql += ", ";
    }
  }
  create_sql += ")";
  exec_sql(db, create_sql, "DataFrame::sql().execute");

  std::string insert_sql = "INSERT INTO " + sql_identifier(internal_name) + " VALUES (";
  for (std::size_t i = 0; i < create_parts.size(); ++i) {
    insert_sql += "?";
    if (i + 1 < create_parts.size()) {
      insert_sql += ", ";
    }
  }
  insert_sql += ")";

  sqlite3_stmt* raw_stmt = nullptr;
  PLUSS_CHECK(sqlite3_prepare_v2(db, insert_sql.c_str(), -1, &raw_stmt, nullptr) == SQLITE_OK,
              "DataFrame::sql().execute: failed to prepare sqlite insert");
  SqliteStmt stmt(raw_stmt);

  const auto labels = frame.label().values();
  for (std::int64_t row = 0; row < frame.ncases(); ++row) {
    PLUSS_CHECK(sqlite3_reset(stmt.get()) == SQLITE_OK, "DataFrame::sql().execute: sqlite reset failed");
    PLUSS_CHECK(sqlite3_clear_bindings(stmt.get()) == SQLITE_OK,
                "DataFrame::sql().execute: sqlite clear bindings failed");
    int bind_index = 1;
    for (const auto& feature_name : feature_names) {
      bind_thread_value(stmt.get(), bind_index++, frame.features(feature_name), row);
    }

    std::string case_text;
    if (row_is_valid(labels, row)) {
      switch (labels.kind()) {
        case ThreadKind::Series:
          case_text = series_scalar_text(labels.as_series(), row);
          break;
        case ThreadKind::Note:
          case_text = std::string(labels.as_note().at(row));
          break;
        case ThreadKind::Timeline:
          case_text = fmt::format("{}", labels.as_timeline().ticks()[static_cast<std::size_t>(row)]);
          break;
        case ThreadKind::Duration:
          case_text = fmt::format(
              "{}", labels.as_duration().host()[static_cast<std::size_t>(row)]);
          break;
      }
    }
    bind_sqlite_text(stmt.get(), bind_index, case_text);

    const auto rc = sqlite3_step(stmt.get());
    PLUSS_CHECK(rc == SQLITE_DONE, "DataFrame::sql().execute: sqlite insert failed");
  }

  std::string view_sql = "CREATE TEMP VIEW " + sql_identifier(source_name) + " AS SELECT ";
  bool first = true;
  for (const auto& feature_name : feature_names) {
    if (!first) {
      view_sql += ", ";
    }
    first = false;
    view_sql += sql_identifier(feature_name);
  }
  for (const auto& alias : visible_case_aliases(frame, case_aliases)) {
    if (!first) {
      view_sql += ", ";
    }
    first = false;
    view_sql += sql_identifier(kInternalCaseColumn) + " AS " + sql_identifier(alias);
  }
  view_sql += " FROM " + sql_identifier(internal_name);
  exec_sql(db, view_sql, "DataFrame::sql().execute");
}

std::string rewrite_case_alias_references(std::string_view statement,
                                          const std::vector<std::string>& case_aliases) {
  if (case_aliases.empty()) {
    return std::string(statement);
  }

  std::unordered_set<std::string> alias_lookup;
  alias_lookup.reserve(case_aliases.size());
  for (const auto& alias : case_aliases) {
    alias_lookup.insert(lower_ascii_copy(alias));
  }

  const auto toks = tokenize_sql(statement);
  if (toks.empty()) {
    return std::string(statement);
  }

  std::string out;
  out.reserve(statement.size() + case_aliases.size() * 8);
  std::size_t cursor = 0;
  for (std::size_t i = 0; i < toks.size(); ++i) {
    const auto& tok = toks[i];
    out.append(statement.substr(cursor, tok.begin - cursor));
    cursor = tok.end;

    std::string replacement = tok.text;
    bool changed = false;

    if (tok.kind == SqlTokenKind::Word) {
      const auto dot = tok.text.rfind('.');
      if (dot != std::string::npos && dot + 1 < tok.text.size()) {
        const auto suffix = lower_ascii_copy(std::string_view(tok.text).substr(dot + 1));
        if (alias_lookup.contains(suffix)) {
          replacement = tok.text.substr(0, dot + 1) + "\"" + tok.text.substr(dot + 1) + "\"";
          changed = true;
        }
      } else if (alias_lookup.contains(lower_ascii_copy(tok.text))) {
        const bool is_case_when = token_is_word(tok, "case") && i + 1 < toks.size() && token_is_word(toks[i + 1], "when");
        if (!is_case_when) {
          replacement = "\"" + tok.text + "\"";
          changed = true;
        }
      }
    }

    out += changed ? replacement : std::string(statement.substr(tok.begin, tok.end - tok.begin));
  }
  out.append(statement.substr(cursor));
  return out;
}

std::vector<ColumnCollector> collect_sqlite_rows(sqlite3* db,
                                                 std::string_view statement,
                                                 const DataFrame::SqlOptions& options,
                                                 TimeoutContext* timeout) {
  sqlite3_stmt* raw = nullptr;
  const char* tail = nullptr;
  const auto rc = sqlite3_prepare_v2(db, std::string(statement).c_str(), -1, &raw, &tail);
  PLUSS_CHECK(rc == SQLITE_OK,
              "DataFrame::sql().execute: sqlite prepare failed: " + std::string(sqlite3_errmsg(db)));
  SqliteStmt stmt(raw);

  PLUSS_CHECK(trim_ascii(tail != nullptr ? std::string_view(tail) : std::string_view()).empty(),
              "DataFrame::sql().execute: only one SQL statement is allowed per execute() call");

  const auto ncols = sqlite3_column_count(stmt.get());
  PLUSS_CHECK(ncols > 0,
              "DataFrame::sql().execute: statement did not produce a result set");

  std::vector<ColumnCollector> collectors;
  collectors.reserve(static_cast<std::size_t>(ncols));
  for (int i = 0; i < ncols; ++i) {
    const auto* raw_name = sqlite3_column_name(stmt.get(), i);
    const auto* raw_decl = sqlite3_column_decltype(stmt.get(), i);
    collectors.push_back(ColumnCollector{
        .name = raw_name != nullptr ? std::string(raw_name) : fmt::format("column_{}", i),
        .declared_type = raw_decl != nullptr ? std::optional<std::string>(raw_decl) : std::nullopt,
        .values = {},
    });
  }

  while (true) {
    const auto step_rc = sqlite3_step(stmt.get());
    if (step_rc == SQLITE_DONE) {
      break;
    }
    if (step_rc == SQLITE_INTERRUPT && options.timeout_ms.has_value()) {
      const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::steady_clock::now() - timeout->start).count();
      PLUSS_CHECK(elapsed <= *options.timeout_ms,
                  "DataFrame::sql().execute: timeout exceeded during SQL execution");
    }
    PLUSS_CHECK(step_rc == SQLITE_ROW,
                "DataFrame::sql().execute: sqlite step failed: " + std::string(sqlite3_errmsg(db)));

    for (int i = 0; i < ncols; ++i) {
      auto& collector = collectors[static_cast<std::size_t>(i)];
      switch (sqlite3_column_type(stmt.get(), i)) {
        case SQLITE_NULL:
          collector.values.emplace_back(std::monostate{});
          break;
        case SQLITE_INTEGER:
          collector.values.emplace_back(static_cast<std::int64_t>(sqlite3_column_int64(stmt.get(), i)));
          break;
        case SQLITE_FLOAT:
          collector.values.emplace_back(sqlite3_column_double(stmt.get(), i));
          break;
        case SQLITE_TEXT: {
          const auto* raw_text = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), i));
          const auto nbytes = sqlite3_column_bytes(stmt.get(), i);
          collector.values.emplace_back(
              raw_text != nullptr ? std::string(raw_text, raw_text + nbytes) : std::string());
          break;
        }
        case SQLITE_BLOB:
        default:
          collector.values.emplace_back(hex_blob(sqlite3_column_blob(stmt.get(), i), sqlite3_column_bytes(stmt.get(), i)));
          break;
      }
    }
  }

  return collectors;
}

DataFrame build_result_frame(std::vector<ColumnCollector> collectors,
                             const DataFrame::SqlOptions& options,
                             const ProjectionHints& projection_hints) {
  if (projection_hints.parsed && !projection_hints.star_only
      && projection_hints.output_names.size() == collectors.size()) {
    for (std::size_t i = 0; i < collectors.size(); ++i) {
      collectors[i].name = projection_hints.output_names[i];
    }
  }

  std::vector<ColumnCollector> filtered;
  filtered.reserve(collectors.size());
  for (auto& collector : collectors) {
    if (projection_hints.star_only
        && std::find(options.case_axis_aliases.begin(), options.case_axis_aliases.end(), collector.name)
               != options.case_axis_aliases.end()) {
      continue;
    }
    filtered.push_back(std::move(collector));
  }

  std::unordered_map<std::string, std::size_t> name_to_pos;
  std::vector<std::pair<std::string, Thread>> cols;
  cols.reserve(filtered.size());
  for (auto& collector : filtered) {
    auto out_name = collector.name;
    const auto it = name_to_pos.find(out_name);
    if (it != name_to_pos.end()) {
      switch (options.collision_policy) {
        case DataFrame::SqlOptions::CollisionPolicy::Error:
          PLUSS_FAIL("DataFrame::sql().execute: duplicate projected column name '" + out_name + "'");
        case DataFrame::SqlOptions::CollisionPolicy::Overwrite:
          cols[it->second].second = build_column_thread(collector);
          continue;
        case DataFrame::SqlOptions::CollisionPolicy::Suffix: {
          std::size_t k = 2;
          std::string candidate = fmt::format("{}_{}", out_name, k);
          while (name_to_pos.find(candidate) != name_to_pos.end()) {
            ++k;
            candidate = fmt::format("{}_{}", out_name, k);
          }
          out_name = std::move(candidate);
          break;
        }
      }
    }

    name_to_pos.emplace(out_name, cols.size());
    cols.emplace_back(out_name, build_column_thread(collector));
  }

  auto out = DataFrame(std::move(cols));
  if (options.max_result_rows.has_value() && out.ncases() > *options.max_result_rows) {
    out = out.slice(0, *options.max_result_rows);
  }
  enforce_memory_budget(options, out, "result");
  return out;
}

SqliteDb build_sqlite_workspace(const DataFrame& primary_frame,
                                const DataFrame::SqlOptions& options) {
  sqlite3* raw_db = nullptr;
  PLUSS_CHECK(sqlite3_open(":memory:", &raw_db) == SQLITE_OK,
              "DataFrame::sql().execute: failed to open in-memory sqlite database");
  SqliteDb db(raw_db);

  exec_sql(db.get(), "PRAGMA temp_store = MEMORY", "DataFrame::sql().execute");
  exec_sql(db.get(), "PRAGMA synchronous = OFF", "DataFrame::sql().execute");
  exec_sql(db.get(), "BEGIN", "DataFrame::sql().execute");

  std::size_t source_index = 0;
  load_source(db.get(), std::string(kPrimarySourceName), primary_frame, options.case_axis_aliases, source_index++);
  for (const auto& alias : options.primary_source_names) {
    exec_sql(db.get(),
             "CREATE TEMP VIEW " + sql_identifier(alias) + " AS SELECT * FROM " + sql_identifier(kPrimarySourceName),
             "DataFrame::sql().execute");
  }
  for (const auto& binding : options.sources) {
    PLUSS_CHECK(binding.frame != nullptr,
                "DataFrame::sql().execute: source '" + binding.name + "' is bound to null DataFrame");
    load_source(db.get(), binding.name, *binding.frame, options.case_axis_aliases, source_index++);
  }

  exec_sql(db.get(), "COMMIT", "DataFrame::sql().execute");
  return db;
}

} // namespace

namespace detail {

std::string sql_primary_source_name(std::string_view statement, const DataFrame::SqlOptions& options) {
  const auto rendered = apply_sql_params(statement, options);
  const auto expanded = maybe_expand_table_name(rendered, options);
  const auto trimmed = trim_ascii(expanded);
  if (trimmed.empty()) {
    return {};
  }
  if (is_simple_identifier(trimmed)) {
    return trimmed;
  }

  const auto toks = tokenize_sql(trimmed);
  std::int64_t depth = 0;
  for (std::size_t i = 0; i + 1 < toks.size(); ++i) {
    const auto& tok = toks[i];
    if (tok.kind == SqlTokenKind::Punct && tok.text == "(") {
      ++depth;
      continue;
    }
    if (tok.kind == SqlTokenKind::Punct && tok.text == ")") {
      --depth;
      continue;
    }
    if (depth == 0 && token_is_word(tok, "from")) {
      return parse_identifier_token(toks[i + 1], "DataFrame::sql().execute FROM");
    }
  }
  return {};
}

} // namespace detail

DataFrameSqlAccessor::DataFrameSqlAccessor(DataFrame frame, DataFrame::SqlOptions options)
    : frame_(std::move(frame)),
      options_(std::move(options)) {}

DataFrame DataFrameSqlAccessor::execute(std::string_view statement) const {
  const auto t0 = std::chrono::steady_clock::now();
  const auto rendered = apply_sql_params(statement, options_);
  auto sql = maybe_expand_table_name(rendered, options_);
  sql = rewrite_case_alias_references(sql, options_.case_axis_aliases);
  sql = trim_ascii(sql);
  PLUSS_CHECK(!sql.empty(), "DataFrame::sql().execute: SQL statement cannot be empty");

  if (options_.diagnostics_trace_tokens) {
    const auto toks = tokenize_sql(sql);
    std::string token_dump;
    token_dump.reserve(toks.size() * 12);
    for (std::size_t i = 0; i < toks.size(); ++i) {
      token_dump += toks[i].text;
      if (i + 1 < toks.size()) {
        token_dump += " | ";
      }
    }
    ::pluss::codem::warn("sql.trace.tokens: " + token_dump);
  }

  auto db = build_sqlite_workspace(frame_, options_);
  TimeoutContext timeout{
      .start = t0,
      .options = &options_,
  };
  attach_timeout_handler(db.get(), &timeout);

  if (options_.diagnostics_explain || options_.diagnostics_trace_plan) {
    ::pluss::codem::warn(format_explain_query_plan(db.get(), sql));
  }

  const auto projection_hints = parse_projection_hints(sql);
  auto collectors = collect_sqlite_rows(db.get(), sql, options_, &timeout);
  auto out = build_result_frame(std::move(collectors), options_, projection_hints);

  if (options_.result_mode == DataFrame::SqlOptions::ResultMode::Chunked) {
    ::pluss::codem::warn(fmt::format("sql.result_mode=Chunked requested (chunk_rows={}) but execute() "
                                     "returns a materialized DataFrame",
                                     options_.chunk_rows));
  }

  if (options_.diagnostics_profile) {
    const auto total_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - t0).count();
    ::pluss::codem::warn(
        fmt::format("sql.profile: total={}ms rows={}", total_ms, out.ncases()));
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

DataFrameSqlPrepared DataFrame::prep_sql(std::string_view statement) const {
  return sql().prepare(statement);
}

DataFrameSqlPrepared DataFrame::prep_sql(std::string_view statement, SqlOptions options) const {
  return sql(std::move(options)).prepare(statement);
}

} // namespace pluss::table
