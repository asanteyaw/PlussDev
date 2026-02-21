#include "table/DataFrame.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <numeric>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include <variant>

#include <fmt/format.h>

#include "CodeM/core/Errors.h"
#include "CodeM/core/Half.h"
#include "loader/CsvLoader.h"
#include "loader/Loader.h"
#include "table/display/Display.h"

namespace pluss::table {

namespace detail {

class DataFrameImpl final : public pluss::msharp::RefCounted {
public:
  DataFrameImpl(std::vector<std::string> names,
                std::vector<Thread> columns,
                std::optional<Thread> cases = std::nullopt);
  void validate_and_reindex_();

  std::vector<std::string> names_;
  std::vector<Thread> columns_;
  std::unordered_map<std::string, std::size_t> name_to_idx_;
  std::int64_t nrows_{0};
  Thread cases_{};
  mutable bool suppress_next_stream_{false};
};

} // namespace detail

namespace {

std::string owned_name(std::string_view sv) {
  return std::string(sv.data(), sv.size());
}

std::string lower_copy(std::string s) {
  for (auto& ch : s) {
    ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  }
  return s;
}

std::pair<std::int64_t, std::int64_t> normalize_slice(std::int64_t start, std::int64_t stop, std::int64_t size) {
  std::int64_t s = start < 0 ? start + size : start;
  std::int64_t e = stop < 0 ? stop + size : stop;
  s = std::clamp<std::int64_t>(s, 0, size);
  e = std::clamp<std::int64_t>(e, 0, size);
  if (e < s) {
    e = s;
  }
  return {s, e};
}

std::int64_t normalize_row_index(std::int64_t row, std::int64_t size) {
  auto out = row < 0 ? row + size : row;
  PLUSS_CHECK(out >= 0 && out < size, "DataFrame: row index out of bounds");
  return out;
}

std::vector<std::string> normalize_name_list(std::initializer_list<std::string_view> names) {
  std::vector<std::string> out;
  out.reserve(names.size());
  for (const auto n : names) {
    out.push_back(owned_name(n));
  }
  return out;
}

void validate_bool_mask(const pluss::Series& mask, std::int64_t expected_size, std::string_view where) {
  PLUSS_CHECK(mask.defined(), std::string(where) + ": mask is undefined");
  PLUSS_CHECK(mask.dim() == 1, std::string(where) + ": mask must be 1D");
  PLUSS_CHECK(mask.scalar_type() == pluss::codem::ScalarType::Bool, std::string(where) + ": mask dtype must be Bool");
  PLUSS_CHECK(mask.numel() == expected_size, std::string(where) + ": mask length mismatch");
}

pluss::Series bool_series_from_keep(const std::vector<bool>& keep) {
  const auto n = static_cast<std::int64_t>(keep.size());
  auto out = pluss::Series::zeros({n}, pluss::codem::ScalarType::Bool);
  auto* p = reinterpret_cast<bool*>(out.raw_data());
  for (std::int64_t i = 0; i < n; ++i) {
    p[i] = keep[static_cast<std::size_t>(i)];
  }
  return out;
}

Thread default_cases(std::int64_t nrows) {
  PLUSS_CHECK(nrows >= 0, "DataFrame: nrows must be non-negative");
  std::vector<std::int64_t> idx(static_cast<std::size_t>(nrows));
  std::iota(idx.begin(), idx.end(), static_cast<std::int64_t>(0));
  auto s = pluss::Series::empty({nrows}, pluss::codem::ScalarType::Int64);
  if (!idx.empty()) {
    std::memcpy(s.raw_data(), idx.data(), idx.size() * sizeof(std::int64_t));
  }
  return Thread(s);
}

bool is_numeric_series_type(const pluss::Series& s) {
  const auto dt = s.dtype();
  return dt.scalar == pluss::codem::ScalarType::Bool || dt.is_integral() || dt.is_floating();
}

bool extract_numeric_value(const pluss::Series& s, std::int64_t idx, double& out) {
  const auto* base = s.raw_data();
  switch (s.scalar_type()) {
    case pluss::codem::ScalarType::Bool:
      out = reinterpret_cast<const bool*>(base)[idx] ? 1.0 : 0.0;
      return true;
    case pluss::codem::ScalarType::Int8:
      out = static_cast<double>(reinterpret_cast<const std::int8_t*>(base)[idx]);
      return true;
    case pluss::codem::ScalarType::Int16:
      out = static_cast<double>(reinterpret_cast<const std::int16_t*>(base)[idx]);
      return true;
    case pluss::codem::ScalarType::Int32:
      out = static_cast<double>(reinterpret_cast<const std::int32_t*>(base)[idx]);
      return true;
    case pluss::codem::ScalarType::Int64:
      out = static_cast<double>(reinterpret_cast<const std::int64_t*>(base)[idx]);
      return true;
    case pluss::codem::ScalarType::UInt8:
      out = static_cast<double>(reinterpret_cast<const std::uint8_t*>(base)[idx]);
      return true;
    case pluss::codem::ScalarType::UInt16:
      out = static_cast<double>(reinterpret_cast<const std::uint16_t*>(base)[idx]);
      return true;
    case pluss::codem::ScalarType::UInt32:
      out = static_cast<double>(reinterpret_cast<const std::uint32_t*>(base)[idx]);
      return true;
    case pluss::codem::ScalarType::UInt64:
      out = static_cast<double>(reinterpret_cast<const std::uint64_t*>(base)[idx]);
      return true;
    case pluss::codem::ScalarType::Float16:
      out = pluss::codem::float16_to_float(reinterpret_cast<const pluss::codem::Float16*>(base)[idx]);
      return true;
    case pluss::codem::ScalarType::BFloat16:
      out = pluss::codem::bfloat16_to_float(reinterpret_cast<const pluss::codem::BFloat16*>(base)[idx]);
      return true;
    case pluss::codem::ScalarType::Float32:
      out = static_cast<double>(reinterpret_cast<const float*>(base)[idx]);
      return true;
    case pluss::codem::ScalarType::Float64:
      out = reinterpret_cast<const double*>(base)[idx];
      return true;
    default:
      return false;
  }
}

std::string series_scalar_string_at(const pluss::Series& s, std::int64_t row) {
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

std::string thread_scalar_string_at(const Thread& t, std::int64_t row) {
  if (t.has_validity_mask()) {
    const auto valid = t.validity_mask()->to_vec<bool>();
    if (!valid[static_cast<std::size_t>(row)]) {
      return "NA";
    }
  }

  switch (t.kind()) {
    case ThreadKind::Series:
      return series_scalar_string_at(t.as_series(), row);
    case ThreadKind::Note:
      return std::string(t.as_note().at(row));
    case ThreadKind::Timeline:
      return fmt::format("{}", t.as_timeline().to_epoch_nanos_vector()[static_cast<std::size_t>(row)]);
    case ThreadKind::TimeIndex:
      return fmt::format("{}", t.as_timeindex().timeline().to_epoch_nanos_vector()[static_cast<std::size_t>(row)]);
    default:
      return "";
  }
}

Thread make_f64_thread(const std::vector<double>& values) {
  const auto n = static_cast<std::int64_t>(values.size());
  auto s = pluss::Series::empty({n}, pluss::codem::ScalarType::Float64);
  if (!values.empty()) {
    std::memcpy(s.raw_data(), values.data(), values.size() * sizeof(double));
  }
  return Thread(s);
}

Thread make_note_thread(std::initializer_list<std::string_view> labels) {
  std::vector<std::string_view> views(labels.begin(), labels.end());
  const std::array<std::int64_t, 1> shape{static_cast<std::int64_t>(views.size())};
  auto note = pluss::coden::ops_internal::note_from_utf8(
      std::span<const std::string_view>(views.data(), views.size()),
      std::span<const std::int64_t>(shape.data(), shape.size()));
  return Thread(note);
}

Thread make_i64_thread(const std::vector<std::int64_t>& values) {
  const auto n = static_cast<std::int64_t>(values.size());
  auto s = pluss::Series::empty({n}, pluss::codem::ScalarType::Int64);
  if (!values.empty()) {
    std::memcpy(s.raw_data(), values.data(), values.size() * sizeof(std::int64_t));
  }
  return Thread(s);
}

std::string summary_op_name(DataFrame::SummaryOp op) {
  switch (op) {
    case DataFrame::SummaryOp::Count:
      return "count";
    case DataFrame::SummaryOp::Nunique:
      return "nunique";
    case DataFrame::SummaryOp::Sum:
      return "sum";
    case DataFrame::SummaryOp::Mean:
      return "mean";
    case DataFrame::SummaryOp::Min:
      return "min";
    case DataFrame::SummaryOp::Max:
      return "max";
    case DataFrame::SummaryOp::Std:
      return "std";
    case DataFrame::SummaryOp::Var:
      return "var";
    default:
      return "summary";
  }
}

std::string window_op_name(ThreadWindowOp op) {
  switch (op) {
    case ThreadWindowOp::RollingSum:
      return "rollsum";
    case ThreadWindowOp::RollingMean:
      return "rollmean";
    case ThreadWindowOp::RollingMin:
      return "rollmin";
    case ThreadWindowOp::RollingMax:
      return "rollmax";
    default:
      return "window";
  }
}

std::int64_t non_null_count(const Thread& t) {
  if (!t.has_validity_mask()) {
    return t.size();
  }
  const auto valid = t.validity_mask()->to_vec<bool>();
  return static_cast<std::int64_t>(std::count(valid.begin(), valid.end(), true));
}

std::int64_t nunique_non_null(const Thread& t) {
  std::unordered_set<std::string> seen;
  seen.reserve(static_cast<std::size_t>(t.size()));
  std::optional<std::vector<bool>> valid;
  if (t.has_validity_mask()) {
    valid = t.validity_mask()->to_vec<bool>();
  }
  for (std::int64_t i = 0; i < t.size(); ++i) {
    if (valid.has_value() && !(*valid)[static_cast<std::size_t>(i)]) {
      continue;
    }
    seen.insert(thread_scalar_string_at(t, i));
  }
  return static_cast<std::int64_t>(seen.size());
}

double numeric_reduce_value(const Thread& t, DataFrame::SummaryOp op) {
  PLUSS_CHECK(t.kind() == ThreadKind::Series, "DataFrame::summarize: numeric reductions require Series feature");
  PLUSS_CHECK(is_numeric_series_type(t.as_series()),
              "DataFrame::summarize: numeric reductions require numeric Series feature");

  const auto& s = t.as_series();
  std::vector<double> values;
  values.reserve(static_cast<std::size_t>(t.size()));
  if (t.has_validity_mask()) {
    const auto valid = t.validity_mask()->to_vec<bool>();
    for (std::int64_t i = 0; i < t.size(); ++i) {
      if (!valid[static_cast<std::size_t>(i)]) {
        continue;
      }
      double v = 0.0;
      if (extract_numeric_value(s, i, v)) {
        values.push_back(v);
      }
    }
  } else {
    for (std::int64_t i = 0; i < t.size(); ++i) {
      double v = 0.0;
      if (extract_numeric_value(s, i, v)) {
        values.push_back(v);
      }
    }
  }

  if (values.empty()) {
    return std::numeric_limits<double>::quiet_NaN();
  }

  switch (op) {
    case DataFrame::SummaryOp::Sum:
      return std::accumulate(values.begin(), values.end(), 0.0);
    case DataFrame::SummaryOp::Mean:
      return std::accumulate(values.begin(), values.end(), 0.0) / static_cast<double>(values.size());
    case DataFrame::SummaryOp::Min:
      return *std::min_element(values.begin(), values.end());
    case DataFrame::SummaryOp::Max:
      return *std::max_element(values.begin(), values.end());
    case DataFrame::SummaryOp::Std: {
      if (values.size() <= 1) {
        return std::numeric_limits<double>::quiet_NaN();
      }
      const auto mean = std::accumulate(values.begin(), values.end(), 0.0) / static_cast<double>(values.size());
      double sq = 0.0;
      for (const auto v : values) {
        const auto d = v - mean;
        sq += d * d;
      }
      return std::sqrt(sq / static_cast<double>(values.size() - 1));
    }
    case DataFrame::SummaryOp::Var: {
      if (values.size() <= 1) {
        return std::numeric_limits<double>::quiet_NaN();
      }
      const auto mean = std::accumulate(values.begin(), values.end(), 0.0) / static_cast<double>(values.size());
      double sq = 0.0;
      for (const auto v : values) {
        const auto d = v - mean;
        sq += d * d;
      }
      return sq / static_cast<double>(values.size() - 1);
    }
    default:
      PLUSS_FAIL("DataFrame::summarize: unsupported numeric SummaryOp");
  }
}

enum class BinaryMathOp : std::uint8_t {
  Add = 0,
  Sub = 1,
  Mul = 2,
  Div = 3,
  TrueDiv = 4,
  FloorDiv = 5,
  Mod = 6,
  Pow = 7,
};

enum class BinaryCmpOp : std::uint8_t {
  Eq = 0,
  Ne = 1,
  Lt = 2,
  Le = 3,
  Gt = 4,
  Ge = 5,
};

const char* math_op_name(BinaryMathOp op) {
  switch (op) {
    case BinaryMathOp::Add:
      return "add";
    case BinaryMathOp::Sub:
      return "sub";
    case BinaryMathOp::Mul:
      return "mul";
    case BinaryMathOp::Div:
      return "div";
    case BinaryMathOp::TrueDiv:
      return "truediv";
    case BinaryMathOp::FloorDiv:
      return "floordiv";
    case BinaryMathOp::Mod:
      return "mod";
    case BinaryMathOp::Pow:
      return "pow";
    default:
      return "binary_math";
  }
}

const char* cmp_op_name(BinaryCmpOp op) {
  switch (op) {
    case BinaryCmpOp::Eq:
      return "eq";
    case BinaryCmpOp::Ne:
      return "ne";
    case BinaryCmpOp::Lt:
      return "lt";
    case BinaryCmpOp::Le:
      return "le";
    case BinaryCmpOp::Gt:
      return "gt";
    case BinaryCmpOp::Ge:
      return "ge";
    default:
      return "compare";
  }
}

enum class QueryTokenKind : std::uint8_t {
  Identifier = 0,
  String = 1,
  Number = 2,
  Boolean = 3,
  LParen = 4,
  RParen = 5,
  Comma = 6,
  Eq = 7,
  Ne = 8,
  Lt = 9,
  Le = 10,
  Gt = 11,
  Ge = 12,
  And = 13,
  Or = 14,
  Not = 15,
  IsNull = 16,
  NotNull = 17,
  Contains = 18,
  In = 19,
  Between = 20,
  End = 21,
};

struct QueryToken {
  QueryTokenKind kind{QueryTokenKind::End};
  std::string text;
};

enum class QueryCompareOp : std::uint8_t {
  Eq = 0,
  Ne = 1,
  Lt = 2,
  Le = 3,
  Gt = 4,
  Ge = 5,
};

struct QueryLiteral {
  enum class Kind : std::uint8_t {
    Number = 0,
    String = 1,
    Bool = 2,
  };

  Kind kind{Kind::String};
  double number{0.0};
  std::string text{};
  bool boolean{false};
};

using QueryEvalFn = std::function<bool(std::int64_t)>;

struct QueryRhs {
  bool is_feature{false};
  Thread feature{};
  QueryLiteral literal{};
};

bool is_ident_start(char ch) {
  return std::isalpha(static_cast<unsigned char>(ch)) || ch == '_';
}

bool is_ident_continue(char ch) {
  return std::isalnum(static_cast<unsigned char>(ch)) || ch == '_' || ch == '.';
}

std::string parse_quoted_literal(std::string_view expr, std::size_t& i, char quote) {
  std::string out;
  ++i; // consume opening quote
  while (i < expr.size()) {
    const char ch = expr[i++];
    if (ch == '\\') {
      PLUSS_CHECK(i < expr.size(), "DataFrame::query: dangling escape in string literal");
      out.push_back(expr[i++]);
      continue;
    }
    if (ch == quote) {
      if (i < expr.size() && expr[i] == quote) {
        out.push_back(quote);
        ++i;
        continue;
      }
      return out;
    }
    out.push_back(ch);
  }
  PLUSS_FAIL("DataFrame::query: unterminated string literal");
}

std::string parse_backtick_identifier(std::string_view expr, std::size_t& i) {
  std::string out;
  ++i; // consume opening backtick
  while (i < expr.size()) {
    const char ch = expr[i++];
    if (ch == '`') {
      return out;
    }
    out.push_back(ch);
  }
  PLUSS_FAIL("DataFrame::query: unterminated backtick identifier");
}

bool is_number_start(std::string_view expr, std::size_t i) {
  if (i >= expr.size()) {
    return false;
  }
  const char ch = expr[i];
  if (std::isdigit(static_cast<unsigned char>(ch)) || ch == '.') {
    return true;
  }
  if ((ch == '-' || ch == '+') && i + 1 < expr.size()) {
    const char next = expr[i + 1];
    return std::isdigit(static_cast<unsigned char>(next)) || next == '.';
  }
  return false;
}

std::size_t consume_number(std::string_view expr, std::size_t i) {
  std::size_t p = i;
  if (p < expr.size() && (expr[p] == '+' || expr[p] == '-')) {
    ++p;
  }
  bool has_digit = false;
  while (p < expr.size() && std::isdigit(static_cast<unsigned char>(expr[p]))) {
    has_digit = true;
    ++p;
  }
  if (p < expr.size() && expr[p] == '.') {
    ++p;
    while (p < expr.size() && std::isdigit(static_cast<unsigned char>(expr[p]))) {
      has_digit = true;
      ++p;
    }
  }
  PLUSS_CHECK(has_digit, "DataFrame::query: invalid numeric literal");
  if (p < expr.size() && (expr[p] == 'e' || expr[p] == 'E')) {
    ++p;
    if (p < expr.size() && (expr[p] == '+' || expr[p] == '-')) {
      ++p;
    }
    bool exp_digit = false;
    while (p < expr.size() && std::isdigit(static_cast<unsigned char>(expr[p]))) {
      exp_digit = true;
      ++p;
    }
    PLUSS_CHECK(exp_digit, "DataFrame::query: invalid numeric exponent");
  }
  return p;
}

std::vector<QueryToken> tokenize_query(std::string_view expression) {
  std::vector<QueryToken> out;
  out.reserve(32);

  for (std::size_t i = 0; i < expression.size();) {
    const char ch = expression[i];
    if (std::isspace(static_cast<unsigned char>(ch))) {
      ++i;
      continue;
    }

    if (ch == '(') {
      out.push_back({QueryTokenKind::LParen, "("});
      ++i;
      continue;
    }
    if (ch == ')') {
      out.push_back({QueryTokenKind::RParen, ")"});
      ++i;
      continue;
    }
    if (ch == ',') {
      out.push_back({QueryTokenKind::Comma, ","});
      ++i;
      continue;
    }
    if (ch == '=' && i + 1 < expression.size() && expression[i + 1] == '=') {
      out.push_back({QueryTokenKind::Eq, "=="});
      i += 2;
      continue;
    }
    if (ch == '=') {
      out.push_back({QueryTokenKind::Eq, "="});
      ++i;
      continue;
    }
    if (ch == '!' && i + 1 < expression.size() && expression[i + 1] == '=') {
      out.push_back({QueryTokenKind::Ne, "!="});
      i += 2;
      continue;
    }
    if (ch == '!') {
      out.push_back({QueryTokenKind::Not, "!"});
      ++i;
      continue;
    }
    if (ch == '<' && i + 1 < expression.size() && expression[i + 1] == '=') {
      out.push_back({QueryTokenKind::Le, "<="});
      i += 2;
      continue;
    }
    if (ch == '>') {
      if (i + 1 < expression.size() && expression[i + 1] == '=') {
        out.push_back({QueryTokenKind::Ge, ">="});
        i += 2;
      } else {
        out.push_back({QueryTokenKind::Gt, ">"});
        ++i;
      }
      continue;
    }
    if (ch == '<') {
      out.push_back({QueryTokenKind::Lt, "<"});
      ++i;
      continue;
    }
    if (ch == '&' && i + 1 < expression.size() && expression[i + 1] == '&') {
      out.push_back({QueryTokenKind::And, "&&"});
      i += 2;
      continue;
    }
    if (ch == '|' && i + 1 < expression.size() && expression[i + 1] == '|') {
      out.push_back({QueryTokenKind::Or, "||"});
      i += 2;
      continue;
    }
    if (ch == '\'' || ch == '"') {
      const auto parsed = parse_quoted_literal(expression, i, ch);
      out.push_back({QueryTokenKind::String, parsed});
      continue;
    }
    if (ch == '`') {
      const auto ident = parse_backtick_identifier(expression, i);
      PLUSS_CHECK(!ident.empty(), "DataFrame::query: empty backtick identifier");
      out.push_back({QueryTokenKind::Identifier, ident});
      continue;
    }
    if (is_number_start(expression, i)) {
      const auto end = consume_number(expression, i);
      out.push_back({QueryTokenKind::Number, std::string(expression.substr(i, end - i))});
      i = end;
      continue;
    }
    if (is_ident_start(ch)) {
      std::size_t end = i + 1;
      while (end < expression.size() && is_ident_continue(expression[end])) {
        ++end;
      }
      const auto ident = std::string(expression.substr(i, end - i));
      const auto low = lower_copy(ident);
      if (low == "and") {
        out.push_back({QueryTokenKind::And, ident});
      } else if (low == "or") {
        out.push_back({QueryTokenKind::Or, ident});
      } else if (low == "not") {
        out.push_back({QueryTokenKind::Not, ident});
      } else if (low == "true" || low == "false") {
        out.push_back({QueryTokenKind::Boolean, low});
      } else if (low == "isnull") {
        out.push_back({QueryTokenKind::IsNull, ident});
      } else if (low == "notnull") {
        out.push_back({QueryTokenKind::NotNull, ident});
      } else if (low == "contains") {
        out.push_back({QueryTokenKind::Contains, ident});
      } else if (low == "in") {
        out.push_back({QueryTokenKind::In, ident});
      } else if (low == "between") {
        out.push_back({QueryTokenKind::Between, ident});
      } else {
        out.push_back({QueryTokenKind::Identifier, ident});
      }
      i = end;
      continue;
    }

    PLUSS_FAIL(fmt::format("DataFrame::query: unsupported character '{}'", ch));
  }

  out.push_back({QueryTokenKind::End, ""});
  return out;
}

bool parse_double_exact(std::string_view text, double& out) {
  std::string owned(text);
  char* end = nullptr;
  out = std::strtod(owned.c_str(), &end);
  return end != nullptr && *end == '\0' && std::isfinite(out);
}

bool parse_bool_like(std::string_view text, bool& out) {
  const auto low = lower_copy(owned_name(text));
  if (low == "true" || low == "1" || low == "yes") {
    out = true;
    return true;
  }
  if (low == "false" || low == "0" || low == "no") {
    out = false;
    return true;
  }
  return false;
}

bool row_valid_for_thread(const Thread& t, std::int64_t row) {
  if (!t.has_validity_mask()) {
    return true;
  }
  const auto valid = t.validity_mask()->to_vec<bool>();
  return valid[static_cast<std::size_t>(row)];
}

bool compare_numbers(double lhs, double rhs, QueryCompareOp op) {
  switch (op) {
    case QueryCompareOp::Eq:
      return lhs == rhs;
    case QueryCompareOp::Ne:
      return lhs != rhs;
    case QueryCompareOp::Lt:
      return lhs < rhs;
    case QueryCompareOp::Le:
      return lhs <= rhs;
    case QueryCompareOp::Gt:
      return lhs > rhs;
    case QueryCompareOp::Ge:
      return lhs >= rhs;
    default:
      return false;
  }
}

bool compare_strings(const std::string& lhs, const std::string& rhs, QueryCompareOp op) {
  switch (op) {
    case QueryCompareOp::Eq:
      return lhs == rhs;
    case QueryCompareOp::Ne:
      return lhs != rhs;
    case QueryCompareOp::Lt:
      return lhs < rhs;
    case QueryCompareOp::Le:
      return lhs <= rhs;
    case QueryCompareOp::Gt:
      return lhs > rhs;
    case QueryCompareOp::Ge:
      return lhs >= rhs;
    default:
      return false;
  }
}

bool evaluate_query_compare(
    const Thread& t,
    std::int64_t row,
    QueryCompareOp op,
    const QueryLiteral& rhs) {
  if (!row_valid_for_thread(t, row)) {
    return false;
  }

  if (t.kind() == ThreadKind::Series) {
    if (rhs.kind == QueryLiteral::Kind::String) {
      return compare_strings(thread_scalar_string_at(t, row), rhs.text, op);
    }

    if (rhs.kind == QueryLiteral::Kind::Bool) {
      if (t.as_series().scalar_type() == pluss::codem::ScalarType::Bool) {
        const auto lhs_bool = t.as_series().to_vec<bool>(row);
        return compare_numbers(lhs_bool ? 1.0 : 0.0, rhs.boolean ? 1.0 : 0.0, op);
      }
      double lhs_num = 0.0;
      if (!extract_numeric_value(t.as_series(), row, lhs_num)) {
        return false;
      }
      return compare_numbers(lhs_num, rhs.boolean ? 1.0 : 0.0, op);
    }

    double lhs_num = 0.0;
    if (!extract_numeric_value(t.as_series(), row, lhs_num)) {
      return false;
    }
    return compare_numbers(lhs_num, rhs.number, op);
  }

  const auto lhs = thread_scalar_string_at(t, row);
  if (rhs.kind == QueryLiteral::Kind::String) {
    return compare_strings(lhs, rhs.text, op);
  }
  if (rhs.kind == QueryLiteral::Kind::Bool) {
    bool lhs_bool = false;
    if (!parse_bool_like(lhs, lhs_bool)) {
      return false;
    }
    return compare_numbers(lhs_bool ? 1.0 : 0.0, rhs.boolean ? 1.0 : 0.0, op);
  }

  double lhs_num = 0.0;
  if (!parse_double_exact(lhs, lhs_num)) {
    return false;
  }
  return compare_numbers(lhs_num, rhs.number, op);
}

bool evaluate_query_compare(
    const Thread& lhs,
    const Thread& rhs,
    std::int64_t row,
    QueryCompareOp op) {
  if (!row_valid_for_thread(lhs, row) || !row_valid_for_thread(rhs, row)) {
    return false;
  }

  const bool lhs_numeric = lhs.kind() == ThreadKind::Series && is_numeric_series_type(lhs.as_series());
  const bool rhs_numeric = rhs.kind() == ThreadKind::Series && is_numeric_series_type(rhs.as_series());
  if (lhs_numeric && rhs_numeric) {
    double lv = 0.0;
    double rv = 0.0;
    if (!extract_numeric_value(lhs.as_series(), row, lv) || !extract_numeric_value(rhs.as_series(), row, rv)) {
      return false;
    }
    return compare_numbers(lv, rv, op);
  }
  return compare_strings(thread_scalar_string_at(lhs, row), thread_scalar_string_at(rhs, row), op);
}

bool evaluate_in_list(const Thread& t, std::int64_t row, const std::vector<QueryLiteral>& items) {
  for (const auto& lit : items) {
    if (evaluate_query_compare(t, row, QueryCompareOp::Eq, lit)) {
      return true;
    }
  }
  return false;
}

Thread apply_math_thread_scalar(const Thread& col, double other, BinaryMathOp op, bool reverse) {
  ThreadBinaryMathOp mapped = ThreadBinaryMathOp::Add;
  switch (op) {
    case BinaryMathOp::Add:
      mapped = ThreadBinaryMathOp::Add;
      break;
    case BinaryMathOp::Sub:
      mapped = ThreadBinaryMathOp::Sub;
      break;
    case BinaryMathOp::Mul:
      mapped = ThreadBinaryMathOp::Mul;
      break;
    case BinaryMathOp::Div:
      mapped = ThreadBinaryMathOp::Div;
      break;
    case BinaryMathOp::TrueDiv:
      mapped = ThreadBinaryMathOp::TrueDiv;
      break;
    case BinaryMathOp::FloorDiv:
      mapped = ThreadBinaryMathOp::FloorDiv;
      break;
    case BinaryMathOp::Mod:
      mapped = ThreadBinaryMathOp::Mod;
      break;
    case BinaryMathOp::Pow:
      mapped = ThreadBinaryMathOp::Pow;
      break;
  }
  return col.numeric_binary(other, mapped, reverse);
}

Thread apply_math_thread_pair(const Thread& lhs, const Thread& rhs, BinaryMathOp op) {
  ThreadBinaryMathOp mapped = ThreadBinaryMathOp::Add;
  switch (op) {
    case BinaryMathOp::Add:
      mapped = ThreadBinaryMathOp::Add;
      break;
    case BinaryMathOp::Sub:
      mapped = ThreadBinaryMathOp::Sub;
      break;
    case BinaryMathOp::Mul:
      mapped = ThreadBinaryMathOp::Mul;
      break;
    case BinaryMathOp::Div:
      mapped = ThreadBinaryMathOp::Div;
      break;
    case BinaryMathOp::TrueDiv:
      mapped = ThreadBinaryMathOp::TrueDiv;
      break;
    case BinaryMathOp::FloorDiv:
      mapped = ThreadBinaryMathOp::FloorDiv;
      break;
    case BinaryMathOp::Mod:
      mapped = ThreadBinaryMathOp::Mod;
      break;
    case BinaryMathOp::Pow:
      mapped = ThreadBinaryMathOp::Pow;
      break;
  }
  return lhs.numeric_binary(rhs, mapped);
}

Thread apply_cmp_thread_scalar(const Thread& col, double other, BinaryCmpOp op) {
  ThreadBinaryCompareOp mapped = ThreadBinaryCompareOp::Eq;
  switch (op) {
    case BinaryCmpOp::Eq:
      mapped = ThreadBinaryCompareOp::Eq;
      break;
    case BinaryCmpOp::Ne:
      mapped = ThreadBinaryCompareOp::Ne;
      break;
    case BinaryCmpOp::Lt:
      mapped = ThreadBinaryCompareOp::Lt;
      break;
    case BinaryCmpOp::Le:
      mapped = ThreadBinaryCompareOp::Le;
      break;
    case BinaryCmpOp::Gt:
      mapped = ThreadBinaryCompareOp::Gt;
      break;
    case BinaryCmpOp::Ge:
      mapped = ThreadBinaryCompareOp::Ge;
      break;
  }
  return col.compare(other, mapped);
}

Thread apply_cmp_thread_pair(const Thread& lhs, const Thread& rhs, BinaryCmpOp op) {
  ThreadBinaryCompareOp mapped = ThreadBinaryCompareOp::Eq;
  switch (op) {
    case BinaryCmpOp::Eq:
      mapped = ThreadBinaryCompareOp::Eq;
      break;
    case BinaryCmpOp::Ne:
      mapped = ThreadBinaryCompareOp::Ne;
      break;
    case BinaryCmpOp::Lt:
      mapped = ThreadBinaryCompareOp::Lt;
      break;
    case BinaryCmpOp::Le:
      mapped = ThreadBinaryCompareOp::Le;
      break;
    case BinaryCmpOp::Gt:
      mapped = ThreadBinaryCompareOp::Gt;
      break;
    case BinaryCmpOp::Ge:
      mapped = ThreadBinaryCompareOp::Ge;
      break;
  }
  return lhs.compare(rhs, mapped);
}

void validate_binary_frame_compat(const DataFrame& lhs, const DataFrame& rhs, std::string_view where) {
  PLUSS_CHECK(lhs.nrows() == rhs.nrows(), fmt::format("{}: case count mismatch", where));
  PLUSS_CHECK(lhs.column_names() == rhs.column_names(),
              fmt::format("{}: features must match by name and order", where));
}

DataFrame dataframe_math_scalar(const DataFrame& frame, double other, BinaryMathOp op, bool reverse) {
  const auto cols = frame.columns();
  const auto& names = frame.column_names();
  std::vector<std::pair<std::string, Thread>> out;
  out.reserve(cols.size());
  for (std::size_t i = 0; i < cols.size(); ++i) {
    out.emplace_back(names[i], apply_math_thread_scalar(cols[i], other, op, reverse));
  }
  return DataFrame(std::move(out), frame.cases());
}

DataFrame dataframe_math_frame(const DataFrame& lhs, const DataFrame& rhs, BinaryMathOp op) {
  validate_binary_frame_compat(lhs, rhs, fmt::format("DataFrame::{}", math_op_name(op)));
  const auto lhs_cols = lhs.columns();
  const auto rhs_cols = rhs.columns();
  const auto& names = lhs.column_names();

  std::vector<std::pair<std::string, Thread>> out;
  out.reserve(lhs_cols.size());
  for (std::size_t i = 0; i < lhs_cols.size(); ++i) {
    out.emplace_back(names[i], apply_math_thread_pair(lhs_cols[i], rhs_cols[i], op));
  }
  return DataFrame(std::move(out), lhs.cases());
}

DataFrame dataframe_cmp_scalar(const DataFrame& frame, double other, BinaryCmpOp op) {
  const auto cols = frame.columns();
  const auto& names = frame.column_names();
  std::vector<std::pair<std::string, Thread>> out;
  out.reserve(cols.size());
  for (std::size_t i = 0; i < cols.size(); ++i) {
    out.emplace_back(names[i], apply_cmp_thread_scalar(cols[i], other, op));
  }
  return DataFrame(std::move(out), frame.cases());
}

DataFrame dataframe_cmp_frame(const DataFrame& lhs, const DataFrame& rhs, BinaryCmpOp op) {
  validate_binary_frame_compat(lhs, rhs, fmt::format("DataFrame::{}", cmp_op_name(op)));
  const auto lhs_cols = lhs.columns();
  const auto rhs_cols = rhs.columns();
  const auto& names = lhs.column_names();

  std::vector<std::pair<std::string, Thread>> out;
  out.reserve(lhs_cols.size());
  for (std::size_t i = 0; i < lhs_cols.size(); ++i) {
    out.emplace_back(names[i], apply_cmp_thread_pair(lhs_cols[i], rhs_cols[i], op));
  }
  return DataFrame(std::move(out), lhs.cases());
}

DataFrame arrange_impl_frame(
    const DataFrame& frame,
    const std::vector<DataFrame::ArrangeSpec>& input_specs) {
  if (frame.nrows() <= 1 || input_specs.empty()) {
    return frame;
  }

  std::vector<DataFrame::ArrangeSpec> normalized;
  normalized.reserve(input_specs.size());
  std::unordered_set<std::string> seen;
  seen.reserve(input_specs.size());
  for (const auto& spec : input_specs) {
    PLUSS_CHECK(!spec.feature.empty(), "DataFrame::arrange: feature name cannot be empty");
    PLUSS_CHECK(frame.has_feature(spec.feature),
                "DataFrame::arrange: unknown feature '" + spec.feature + "'");
    if (seen.insert(spec.feature).second) {
      normalized.push_back(spec);
    }
  }
  if (normalized.empty()) {
    return frame;
  }

  std::vector<Thread> keys;
  keys.reserve(normalized.size());
  for (const auto& spec : normalized) {
    keys.push_back(frame.feature(spec.feature));
  }

  std::vector<std::int64_t> indices(static_cast<std::size_t>(frame.nrows()));
  std::iota(indices.begin(), indices.end(), static_cast<std::int64_t>(0));

  std::stable_sort(indices.begin(), indices.end(), [&](std::int64_t lhs, std::int64_t rhs) {
    for (std::size_t i = 0; i < keys.size(); ++i) {
      const auto lhs_valid = keys[i].is_valid(lhs);
      const auto rhs_valid = keys[i].is_valid(rhs);
      if (lhs_valid != rhs_valid) {
        if (normalized[i].nulls_last) {
          return lhs_valid; // valid values first
        }
        return !lhs_valid; // null values first
      }

      const auto cmp = keys[i].compare_rows(lhs, rhs, true);
      if (cmp < 0) {
        return normalized[i].ascending;
      }
      if (cmp > 0) {
        return !normalized[i].ascending;
      }
    }
    return false;
  });

  return frame.take(indices);
}

class QueryParser final {
public:
  QueryParser(const DataFrame& frame, std::vector<QueryToken> tokens)
      : frame_(frame), tokens_(std::move(tokens)) {}

  QueryEvalFn parse() {
    auto eval = parse_or_expr();
    expect(QueryTokenKind::End, "DataFrame::query: unexpected trailing tokens");
    return eval;
  }

private:
  const QueryToken& peek() const {
    return tokens_[pos_];
  }

  bool match(QueryTokenKind kind) {
    if (peek().kind == kind) {
      ++pos_;
      return true;
    }
    return false;
  }

  const QueryToken& advance() {
    const auto& tok = peek();
    if (tok.kind != QueryTokenKind::End) {
      ++pos_;
    }
    return tok;
  }

  const QueryToken& expect(QueryTokenKind kind, std::string_view message) {
    PLUSS_CHECK(peek().kind == kind, std::string(message));
    return advance();
  }

  Thread resolve_feature(std::string_view ident) const {
    const auto low = lower_copy(owned_name(ident));
    if (low == "case" || low == "cases") {
      return frame_.cases();
    }
    return frame_.feature(ident);
  }

  QueryLiteral parse_literal() {
    QueryLiteral out;
    if (match(QueryTokenKind::String)) {
      out.kind = QueryLiteral::Kind::String;
      out.text = tokens_[pos_ - 1].text;
      return out;
    }
    if (match(QueryTokenKind::Boolean)) {
      out.kind = QueryLiteral::Kind::Bool;
      out.boolean = lower_copy(tokens_[pos_ - 1].text) == "true";
      return out;
    }
    if (match(QueryTokenKind::Number)) {
      out.kind = QueryLiteral::Kind::Number;
      double parsed = 0.0;
      PLUSS_CHECK(parse_double_exact(tokens_[pos_ - 1].text, parsed),
                  "DataFrame::query: invalid numeric literal");
      out.number = parsed;
      return out;
    }
    PLUSS_FAIL("DataFrame::query: expected a string, bool, or numeric literal");
  }

  QueryRhs parse_rhs_feature_or_literal() {
    QueryRhs rhs;
    if (peek().kind == QueryTokenKind::Identifier) {
      rhs.is_feature = true;
      rhs.feature = resolve_feature(advance().text);
      return rhs;
    }
    rhs.is_feature = false;
    rhs.literal = parse_literal();
    return rhs;
  }

  std::vector<QueryLiteral> parse_literal_list() {
    expect(QueryTokenKind::LParen, "DataFrame::query: expected '(' after in/not in");
    std::vector<QueryLiteral> items;
    if (match(QueryTokenKind::RParen)) {
      return items;
    }
    while (true) {
      items.push_back(parse_literal());
      if (match(QueryTokenKind::Comma)) {
        continue;
      }
      expect(QueryTokenKind::RParen, "DataFrame::query: expected ')' to close in-list");
      break;
    }
    return items;
  }

  QueryCompareOp parse_compare_op() {
    if (match(QueryTokenKind::Eq)) {
      return QueryCompareOp::Eq;
    }
    if (match(QueryTokenKind::Ne)) {
      return QueryCompareOp::Ne;
    }
    if (match(QueryTokenKind::Lt)) {
      return QueryCompareOp::Lt;
    }
    if (match(QueryTokenKind::Le)) {
      return QueryCompareOp::Le;
    }
    if (match(QueryTokenKind::Gt)) {
      return QueryCompareOp::Gt;
    }
    if (match(QueryTokenKind::Ge)) {
      return QueryCompareOp::Ge;
    }
    PLUSS_FAIL("DataFrame::query: expected a comparison operator");
  }

  QueryEvalFn parse_primary() {
    if (match(QueryTokenKind::LParen)) {
      auto inner = parse_or_expr();
      expect(QueryTokenKind::RParen, "DataFrame::query: expected ')'");
      return inner;
    }

    if (match(QueryTokenKind::IsNull)) {
      expect(QueryTokenKind::LParen, "DataFrame::query: expected '(' after isnull");
      const auto ident = expect(QueryTokenKind::Identifier, "DataFrame::query: expected feature name in isnull");
      auto target = resolve_feature(ident.text);
      expect(QueryTokenKind::RParen, "DataFrame::query: expected ')' after isnull(feature)");
      return [target](std::int64_t row) { return !row_valid_for_thread(target, row); };
    }

    if (match(QueryTokenKind::NotNull)) {
      expect(QueryTokenKind::LParen, "DataFrame::query: expected '(' after notnull");
      const auto ident = expect(QueryTokenKind::Identifier, "DataFrame::query: expected feature name in notnull");
      auto target = resolve_feature(ident.text);
      expect(QueryTokenKind::RParen, "DataFrame::query: expected ')' after notnull(feature)");
      return [target](std::int64_t row) { return row_valid_for_thread(target, row); };
    }

    if (match(QueryTokenKind::Contains)) {
      expect(QueryTokenKind::LParen, "DataFrame::query: expected '(' after contains");
      const auto ident = expect(QueryTokenKind::Identifier, "DataFrame::query: expected feature name in contains");
      auto target = resolve_feature(ident.text);
      expect(QueryTokenKind::Comma, "DataFrame::query: expected ',' in contains(feature, \"text\")");
      const auto literal = parse_literal();
      PLUSS_CHECK(literal.kind == QueryLiteral::Kind::String,
                  "DataFrame::query: contains(feature, literal) requires string literal");
      expect(QueryTokenKind::RParen, "DataFrame::query: expected ')' after contains(feature, literal)");
      const auto needle = literal.text;
      return [target, needle](std::int64_t row) {
        if (!row_valid_for_thread(target, row)) {
          return false;
        }
        return thread_scalar_string_at(target, row).find(needle) != std::string::npos;
      };
    }

    const auto ident = expect(QueryTokenKind::Identifier, "DataFrame::query: expected feature name");
    auto target = resolve_feature(ident.text);

    if (match(QueryTokenKind::IsNull)) {
      return [target](std::int64_t row) { return !row_valid_for_thread(target, row); };
    }
    if (match(QueryTokenKind::NotNull)) {
      return [target](std::int64_t row) { return row_valid_for_thread(target, row); };
    }

    bool negate = false;
    if (match(QueryTokenKind::Not)) {
      negate = true;
    }
    if (match(QueryTokenKind::In)) {
      const auto items = parse_literal_list();
      PLUSS_CHECK(!items.empty(), "DataFrame::query: in-list cannot be empty");
      return [target, items, negate](std::int64_t row) {
        const auto present = evaluate_in_list(target, row, items);
        return negate ? !present : present;
      };
    }
    PLUSS_CHECK(!negate, "DataFrame::query: expected 'in' after 'not' in predicate");

    if (match(QueryTokenKind::Between)) {
      const auto lower = parse_literal();
      expect(QueryTokenKind::And, "DataFrame::query: expected 'and' in between expression");
      const auto upper = parse_literal();
      return [target, lower, upper](std::int64_t row) {
        return evaluate_query_compare(target, row, QueryCompareOp::Ge, lower)
            && evaluate_query_compare(target, row, QueryCompareOp::Le, upper);
      };
    }

    const auto op = parse_compare_op();
    const auto rhs = parse_rhs_feature_or_literal();
    return [target, op, rhs](std::int64_t row) {
      if (rhs.is_feature) {
        return evaluate_query_compare(target, rhs.feature, row, op);
      }
      return evaluate_query_compare(target, row, op, rhs.literal);
    };
  }

  QueryEvalFn parse_unary_expr() {
    if (match(QueryTokenKind::Not)) {
      auto inner = parse_unary_expr();
      return [inner](std::int64_t row) { return !inner(row); };
    }
    return parse_primary();
  }

  QueryEvalFn parse_and_expr() {
    auto left = parse_unary_expr();
    while (match(QueryTokenKind::And)) {
      auto right = parse_unary_expr();
      auto prev = std::move(left);
      left = [prev, right](std::int64_t row) { return prev(row) && right(row); };
    }
    return left;
  }

  QueryEvalFn parse_or_expr() {
    auto left = parse_and_expr();
    while (match(QueryTokenKind::Or)) {
      auto right = parse_and_expr();
      auto prev = std::move(left);
      left = [prev, right](std::int64_t row) { return prev(row) || right(row); };
    }
    return left;
  }

  const DataFrame& frame_;
  std::vector<QueryToken> tokens_;
  std::size_t pos_{0};
};

double quantile_sorted(const std::vector<double>& sorted, double q) {
  PLUSS_CHECK(!sorted.empty(), "DataFrame::describe: quantile on empty data");
  const double pos = (static_cast<double>(sorted.size()) - 1.0) * q;
  const auto lo = static_cast<std::size_t>(std::floor(pos));
  const auto hi = static_cast<std::size_t>(std::ceil(pos));
  if (lo == hi) {
    return sorted[lo];
  }
  const double w = pos - static_cast<double>(lo);
  return sorted[lo] * (1.0 - w) + sorted[hi] * w;
}

std::int64_t normalize_view_start(std::int64_t start, std::int64_t nrows) {
  const std::int64_t s = start < 0 ? nrows + start : start;
  return std::clamp<std::int64_t>(s, 0, nrows);
}

std::int64_t normalize_view_stop(std::int64_t stop, std::int64_t nrows) {
  // Positive stop is exclusive. Negative stop is interpreted as inclusive-from-end
  // so `view(-5, -1)` includes the last row.
  const std::int64_t e = stop < 0 ? (nrows + stop + 1) : stop;
  return std::clamp<std::int64_t>(e, 0, nrows);
}

} // namespace

detail::DataFrameImpl::DataFrameImpl(std::vector<std::string> names,
                                     std::vector<Thread> columns,
                                     std::optional<Thread> cases)
    : names_(std::move(names)), columns_(std::move(columns)) {
  if (cases.has_value()) {
    cases_ = *cases;
  }
  validate_and_reindex_();
}

void detail::DataFrameImpl::validate_and_reindex_() {
  PLUSS_CHECK(names_.size() == columns_.size(), "DataFrame: names/columns size mismatch");
  name_to_idx_.clear();
  name_to_idx_.reserve(names_.size());

  std::optional<std::int64_t> expected_rows;
  for (std::size_t i = 0; i < names_.size(); ++i) {
    const auto& name = names_[i];
    const auto& col = columns_[i];
    PLUSS_CHECK(!name.empty(), "DataFrame: column names must be non-empty");
    PLUSS_CHECK(col.defined(), "DataFrame: column handle is undefined");
    PLUSS_CHECK(name_to_idx_.find(name) == name_to_idx_.end(), "DataFrame: duplicate column name '" + name + "'");
    name_to_idx_.emplace(name, i);

    const auto col_rows = col.size();
    if (!expected_rows.has_value()) {
      expected_rows = col_rows;
    } else {
      PLUSS_CHECK(*expected_rows == col_rows, "DataFrame: all columns must have identical length");
    }
  }

  nrows_ = expected_rows.value_or(0);

  if (!cases_.defined()) {
    cases_ = default_cases(nrows_);
  }
  PLUSS_CHECK(cases_.defined(), "DataFrame: cases are undefined");
  PLUSS_CHECK(cases_.size() == nrows_, "DataFrame: case length mismatch");
}

namespace {

pluss::msharp::intrusive_ptr<detail::DataFrameImpl> clone_impl(const detail::DataFrameImpl& src) {
  return pluss::msharp::make_intrusive<detail::DataFrameImpl>(src.names_, src.columns_, src.cases_);
}

} // namespace

ColumnRef::ColumnRef(std::string name, Thread column)
    : name_(std::move(name)), column_(std::move(column)) {}

const std::string& ColumnRef::name() const noexcept { return name_; }
const Thread& ColumnRef::thread() const noexcept { return column_; }
double ColumnRef::mean() const { return column_.mean(); }
Thread ColumnRef::head(std::int64_t n) const { return column_.head(n); }
ThreadValue ColumnRef::value() const { return column_.value(); }
Thread ColumnRef::values() const { return column_; }

DataFrame::DataFrame(std::vector<std::pair<std::string, Thread>> columns,
                     std::optional<Thread> cases) {
  std::vector<std::string> names;
  std::vector<Thread> values;
  names.reserve(columns.size());
  values.reserve(columns.size());
  for (auto& kv : columns) {
    names.push_back(std::move(kv.first));
    values.push_back(std::move(kv.second));
  }
  impl_ = pluss::msharp::make_intrusive<detail::DataFrameImpl>(std::move(names), std::move(values), std::move(cases));
}

DataFrame::DataFrame() = default;

DataFrame::DataFrame(pluss::msharp::intrusive_ptr<detail::DataFrameImpl> impl) : impl_(std::move(impl)) {}

DataFrame::DataFrame(const DataFrame& other) = default;
DataFrame::DataFrame(DataFrame&& other) noexcept = default;
DataFrame& DataFrame::operator=(const DataFrame& other) = default;
DataFrame& DataFrame::operator=(DataFrame&& other) noexcept = default;
DataFrame::~DataFrame() = default;

DataFrame DataFrame::load(std::string_view source_type, std::string_view filepath) {
  return load(source_type, filepath, pluss::loader::Config{});
}

DataFrame DataFrame::load(std::string_view source_type,
                          std::string_view filepath,
                          const pluss::loader::Config& config) {
  auto loaded = pluss::loader::load(source_type, filepath, config);
  if (loaded.is_frame()) {
    return loaded.dataframe();
  }
  PLUSS_FAIL("DataFrame::load: requested source returned a workbook. Use pluss::loader::load(...) for multi-sheet loads");
}

DataFrame DataFrame::load(std::string_view source_type,
                          std::string_view filepath,
                          std::initializer_list<pluss::loader::ConfigEntry> config_entries) {
  return load(source_type, filepath, pluss::loader::config_from(config_entries));
}

DataFrame DataFrame::load(std::string_view source_type,
                          std::string_view filepath,
                          const pluss::loader::ReadCsvOptions& options) {
  const auto source = lower_copy(owned_name(source_type));
  if (source == "csv") {
    return pluss::loader::load_csv(filepath, options);
  }
  PLUSS_FAIL("DataFrame::load: unsupported source type '" + std::string(source_type) + "'");
}

DataFrame DataFrame::load(std::string_view source_type,
                          std::string_view filepath,
                          char sep,
                          std::optional<std::size_t> nrows,
                          std::vector<std::string> usecols,
                          std::optional<std::string> case_col,
                          pluss::loader::BadLinePolicy on_bad_lines) {
  pluss::loader::ReadCsvOptions options;
  options.sep = sep;
  options.nrows = nrows;
  options.usecols = std::move(usecols);
  options.case_col = std::move(case_col);
  options.on_bad_lines = on_bad_lines;
  return load(source_type, filepath, options);
}

DataFrame DataFrame::load(std::string_view source_type,
                          std::string_view filepath,
                          char sep,
                          std::optional<std::size_t> nrows,
                          std::initializer_list<std::string_view> usecols,
                          std::optional<std::string> case_col,
                          pluss::loader::BadLinePolicy on_bad_lines) {
  return load(source_type, filepath, sep, nrows, normalize_name_list(usecols), std::move(case_col), on_bad_lines);
}

DataFrame DataFrame::load_csv(std::string_view filepath) {
  pluss::loader::ReadCsvOptions options;
  return load_csv(filepath, options);
}

DataFrame DataFrame::load_csv(std::string_view filepath, const pluss::loader::ReadCsvOptions& options) {
  return pluss::loader::load_csv(filepath, options);
}

DataFrame DataFrame::load_csv(std::string_view filepath, const pluss::loader::Config& config) {
  return pluss::loader::load_csv(filepath, config);
}

DataFrame DataFrame::load_csv(std::string_view filepath,
                              std::initializer_list<pluss::loader::ConfigEntry> config_entries) {
  return pluss::loader::load_csv(filepath, config_entries);
}

DataFrame DataFrame::load_csv(std::string_view filepath,
                              std::string_view header,
                              std::string_view case_feature,
                              std::string_view engine,
                              std::optional<std::size_t> ncases) {
  return pluss::loader::load_csv(filepath, header, case_feature, engine, ncases);
}

DataFrame DataFrame::load_csv(std::string_view filepath,
                              std::string_view header,
                              std::size_t ncases) {
  return pluss::loader::load_csv(filepath, header, ncases);
}

DataFrame DataFrame::load_csv(std::string_view filepath,
                              char sep,
                              std::optional<std::size_t> nrows,
                              std::vector<std::string> usecols,
                              std::optional<std::string> case_col,
                              pluss::loader::BadLinePolicy on_bad_lines) {
  pluss::loader::ReadCsvOptions options;
  options.sep = sep;
  options.nrows = nrows;
  options.usecols = std::move(usecols);
  options.case_col = std::move(case_col);
  options.on_bad_lines = on_bad_lines;
  return load_csv(filepath, options);
}

DataFrame DataFrame::load_csv(std::string_view filepath,
                              char sep,
                              std::optional<std::size_t> nrows,
                              std::initializer_list<std::string_view> usecols,
                              std::optional<std::string> case_col,
                              pluss::loader::BadLinePolicy on_bad_lines) {
  return load_csv(filepath, sep, nrows, normalize_name_list(usecols), std::move(case_col), on_bad_lines);
}

DataFrame DataFrame::load_excel(std::string_view filepath) {
  pluss::loader::ReadExcelOptions options;
  return load_excel(filepath, options);
}

DataFrame DataFrame::load_excel(std::string_view filepath, const pluss::loader::ReadExcelOptions& options) {
  return pluss::loader::load_excel(filepath, options);
}

DataFrame DataFrame::load_excel(std::string_view filepath, const pluss::loader::Config& config) {
  return pluss::loader::load_excel(filepath, config);
}

DataFrame DataFrame::load_excel(std::string_view filepath,
                                std::initializer_list<pluss::loader::ConfigEntry> config_entries) {
  return pluss::loader::load_excel(filepath, config_entries);
}

DataFrame DataFrame::load_excel(std::string_view filepath,
                                std::string_view sheet_name,
                                bool header) {
  const auto token = lower_copy(owned_name(sheet_name));
  PLUSS_CHECK(token != "none",
              "DataFrame::load_excel(sheet_name=\"none\") is workbook mode; use DataFrame::load_excel(filepath, nullptr, header)");
  return pluss::loader::load_excel(filepath, sheet_name, header);
}

DataFrame DataFrame::load_excel(std::string_view filepath,
                                std::int64_t sheet_index,
                                bool header) {
  return pluss::loader::load_excel(filepath, sheet_index, header);
}

pluss::loader::LoadResult DataFrame::load_excel(std::string_view filepath, std::nullptr_t all_sheets, bool header) {
  return pluss::loader::load_excel(filepath, all_sheets, header);
}

pluss::loader::LoadResult DataFrame::load_excel(std::string_view filepath,
                                                const std::vector<std::string>& sheet_names,
                                                bool header) {
  return pluss::loader::load_excel(filepath, sheet_names, header);
}

pluss::loader::LoadResult DataFrame::load_excel(std::string_view filepath,
                                                std::initializer_list<std::string_view> sheet_names,
                                                bool header) {
  return pluss::loader::load_excel(filepath, sheet_names, header);
}

pluss::loader::LoadResult DataFrame::load_excel(std::string_view filepath,
                                                const std::vector<std::int64_t>& sheet_indices,
                                                bool header) {
  return pluss::loader::load_excel(filepath, sheet_indices, header);
}

pluss::loader::LoadResult DataFrame::load_excel(std::string_view filepath,
                                                std::initializer_list<std::int64_t> sheet_indices,
                                                bool header) {
  return pluss::loader::load_excel(filepath, sheet_indices, header);
}

DataFrame DataFrame::load_parquet(std::string_view filepath) {
  return load_parquet(filepath, pluss::loader::Config{});
}

DataFrame DataFrame::load_parquet(std::string_view filepath, const pluss::loader::Config& config) {
  return pluss::loader::load_parquet(filepath, config);
}

DataFrame DataFrame::load_parquet(std::string_view filepath,
                                  std::initializer_list<pluss::loader::ConfigEntry> config_entries) {
  return pluss::loader::load_parquet(filepath, config_entries);
}

bool DataFrame::defined() const noexcept { return static_cast<bool>(impl_); }
bool DataFrame::unique() const noexcept { return impl_.unique(); }

const detail::DataFrameImpl& DataFrame::impl_ref_() const {
  PLUSS_CHECK(defined(), "DataFrame: undefined handle");
  return *impl_;
}

detail::DataFrameImpl& DataFrame::mut_impl_() {
  PLUSS_CHECK(defined(), "DataFrame: undefined handle");
  if (!impl_.unique()) {
    impl_ = clone_impl(*impl_);
  }
  return *impl_;
}

void DataFrame::suppress_next_stream_() const {
  if (!defined()) {
    return;
  }
  impl_ref_().suppress_next_stream_ = true;
}

bool DataFrame::consume_stream_suppression_() const {
  if (!defined()) {
    return false;
  }
  auto& src = impl_ref_();
  if (!src.suppress_next_stream_) {
    return false;
  }
  src.suppress_next_stream_ = false;
  return true;
}

std::int64_t DataFrame::nrows() const { return impl_ref_().nrows_; }
std::int64_t DataFrame::ncols() const { return static_cast<std::int64_t>(impl_ref_().columns_.size()); }
std::int64_t DataFrame::nfeatures() const { return ncols(); }

const std::vector<std::string>& DataFrame::column_names() const { return impl_ref_().names_; }

std::vector<Thread> DataFrame::columns() const { return impl_ref_().columns_; }
std::vector<Thread> DataFrame::values() const { return columns(); }
const std::vector<std::string>& DataFrame::feature_names() const { return column_names(); }
std::vector<Thread> DataFrame::features() const { return columns(); }

const Thread& DataFrame::cases() const { return impl_ref_().cases_; }

DataFrame DataFrame::cases(std::int64_t row) const {
  const auto idx = normalize_row_index(row, nrows());
  return slice(idx, idx + 1);
}

DataFrame DataFrame::cases(std::pair<std::int64_t, std::int64_t> row_span) const {
  return slice(row_span.first, row_span.second);
}

std::string DataFrame::cases(std::int64_t row, std::size_t feature_idx) const {
  const auto r = normalize_row_index(row, nrows());
  PLUSS_CHECK(feature_idx < static_cast<std::size_t>(ncols()), "DataFrame::cases(row, feature): feature index out of bounds");
  const auto& src = impl_ref_();
  return thread_scalar_string_at(src.columns_[feature_idx], r);
}

std::string DataFrame::cases(std::int64_t row, std::string_view feature_name) const {
  const auto r = normalize_row_index(row, nrows());
  return thread_scalar_string_at(column(feature_name), r);
}

pluss::Series DataFrame::cases_series() const {
  PLUSS_CHECK(cases().kind() == ThreadKind::Series, "DataFrame::cases_series requires a Series-backed case axis");
  return cases().as_series();
}

DataFrame DataFrame::with_cases(const Thread& case_idx) const {
  const auto& src = impl_ref_();
  PLUSS_CHECK(case_idx.defined(), "DataFrame::with_cases: cases are undefined");
  PLUSS_CHECK(case_idx.size() == src.nrows_, "DataFrame::with_cases: case length mismatch");
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(src.names_, src.columns_, case_idx));
}

DataFrame& DataFrame::with_cases_(const Thread& case_idx) {
  auto& w = mut_impl_();
  PLUSS_CHECK(case_idx.defined(), "DataFrame::with_cases_: cases are undefined");
  PLUSS_CHECK(case_idx.size() == w.nrows_, "DataFrame::with_cases_: case length mismatch");
  w.cases_ = case_idx;
  w.validate_and_reindex_();
  return *this;
}

bool DataFrame::has_column(std::string_view name) const {
  const auto& src = impl_ref_();
  return src.name_to_idx_.find(owned_name(name)) != src.name_to_idx_.end();
}

bool DataFrame::has_feature(std::string_view name) const { return has_column(name); }

Thread DataFrame::column(std::string_view name) const {
  const auto& src = impl_ref_();
  const auto it = src.name_to_idx_.find(owned_name(name));
  PLUSS_CHECK(it != src.name_to_idx_.end(), "DataFrame: unknown column '" + owned_name(name) + "'");
  return src.columns_[it->second];
}

Thread DataFrame::feature(std::string_view name) const { return column(name); }
Thread DataFrame::features(std::string_view name) const { return feature(name); }

ColumnRef DataFrame::column_ref(std::string_view name) const {
  return ColumnRef(owned_name(name), column(name));
}

ColumnRef DataFrame::feature_ref(std::string_view name) const { return column_ref(name); }

DataFrame DataFrame::features(std::initializer_list<std::string_view> names) const {
  return select(names);
}

DataFrame DataFrame::select(std::initializer_list<std::string_view> names) const {
  const auto picked = normalize_name_list(names);
  const auto& src = impl_ref_();

  std::vector<std::string> out_names;
  std::vector<Thread> out_cols;
  out_names.reserve(picked.size());
  out_cols.reserve(picked.size());

  for (const auto& n : picked) {
    const auto it = src.name_to_idx_.find(n);
    PLUSS_CHECK(it != src.name_to_idx_.end(), "DataFrame::select: unknown column '" + n + "'");
    out_names.push_back(n);
    out_cols.push_back(src.columns_[it->second]);
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(std::move(out_names), std::move(out_cols), src.cases_));
}

DataFrame DataFrame::drop(std::initializer_list<std::string_view> names) const {
  const auto drop_names = normalize_name_list(names);
  const auto& src = impl_ref_();
  std::unordered_set<std::string> drop_set;
  drop_set.reserve(drop_names.size());
  for (const auto& n : drop_names) {
    PLUSS_CHECK(src.name_to_idx_.find(n) != src.name_to_idx_.end(), "DataFrame::drop: unknown column '" + n + "'");
    drop_set.insert(n);
  }

  std::vector<std::string> out_names;
  std::vector<Thread> out_cols;
  out_names.reserve(src.names_.size());
  out_cols.reserve(src.columns_.size());

  for (std::size_t i = 0; i < src.names_.size(); ++i) {
    if (drop_set.find(src.names_[i]) == drop_set.end()) {
      out_names.push_back(src.names_[i]);
      out_cols.push_back(src.columns_[i]);
    }
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(std::move(out_names), std::move(out_cols), src.cases_));
}

DataFrame DataFrame::reorder(std::initializer_list<std::string_view> names) const {
  const auto ordered = normalize_name_list(names);
  const auto& src = impl_ref_();

  PLUSS_CHECK(ordered.size() == src.names_.size(), "DataFrame::reorder requires the full column set");
  std::unordered_set<std::string> seen;
  seen.reserve(ordered.size());
  for (const auto& n : ordered) {
    PLUSS_CHECK(seen.insert(n).second, "DataFrame::reorder contains duplicate column name '" + n + "'");
    PLUSS_CHECK(src.name_to_idx_.find(n) != src.name_to_idx_.end(), "DataFrame::reorder unknown column '" + n + "'");
  }
  return select(names);
}

DataFrame DataFrame::arrange(std::initializer_list<std::string_view> features,
                             bool ascending,
                             bool nulls_last) const {
  std::vector<ArrangeSpec> specs;
  specs.reserve(features.size());
  for (const auto name : features) {
    specs.push_back({owned_name(name), ascending, nulls_last});
  }
  return arrange_impl_frame(*this, specs);
}

DataFrame DataFrame::arrange(std::initializer_list<ArrangeSpec> specs) const {
  return arrange_impl_frame(*this, std::vector<ArrangeSpec>(specs.begin(), specs.end()));
}

DataFrame DataFrame::with_column(std::string name, const Thread& column_value) const {
  const auto& src = impl_ref_();
  PLUSS_CHECK(column_value.defined(), "DataFrame::with_column: input column is undefined");
  if (!src.columns_.empty()) {
    PLUSS_CHECK(column_value.size() == src.nrows_, "DataFrame::with_column: row count mismatch");
  }

  auto out_names = src.names_;
  auto out_cols = src.columns_;
  const auto it = src.name_to_idx_.find(name);
  if (it == src.name_to_idx_.end()) {
    out_names.push_back(std::move(name));
    out_cols.push_back(column_value);
  } else {
    out_cols[it->second] = column_value;
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(std::move(out_names), std::move(out_cols), src.cases_));
}

DataFrame& DataFrame::with_column_(std::string name, const Thread& column_value) {
  auto& w = mut_impl_();
  PLUSS_CHECK(column_value.defined(), "DataFrame::with_column_: input column is undefined");
  if (!w.columns_.empty()) {
    PLUSS_CHECK(column_value.size() == w.nrows_, "DataFrame::with_column_: row count mismatch");
  }

  const auto it = w.name_to_idx_.find(name);
  if (it == w.name_to_idx_.end()) {
    w.names_.push_back(std::move(name));
    w.columns_.push_back(column_value);
  } else {
    w.columns_[it->second] = column_value;
  }
  w.validate_and_reindex_();
  return *this;
}

DataFrame DataFrame::with_feature(std::string name, const Thread& feature_value) const {
  return with_column(std::move(name), feature_value);
}

DataFrame& DataFrame::with_feature_(std::string name, const Thread& feature_value) {
  return with_column_(std::move(name), feature_value);
}

DataFrame DataFrame::rename(std::string_view old_name, std::string new_name) const {
  const auto& src = impl_ref_();
  const auto old_key = owned_name(old_name);
  const auto it = src.name_to_idx_.find(old_key);
  PLUSS_CHECK(it != src.name_to_idx_.end(), "DataFrame::rename: unknown column '" + old_key + "'");
  if (old_key != new_name) {
    PLUSS_CHECK(src.name_to_idx_.find(new_name) == src.name_to_idx_.end(), "DataFrame::rename: target name already exists");
  }

  auto out_names = src.names_;
  out_names[it->second] = std::move(new_name);
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(std::move(out_names), src.columns_, src.cases_));
}

DataFrame& DataFrame::rename_(std::string_view old_name, std::string new_name) {
  auto& w = mut_impl_();
  const auto old_key = owned_name(old_name);
  const auto it = w.name_to_idx_.find(old_key);
  PLUSS_CHECK(it != w.name_to_idx_.end(), "DataFrame::rename_: unknown column '" + old_key + "'");
  if (old_key != new_name) {
    PLUSS_CHECK(w.name_to_idx_.find(new_name) == w.name_to_idx_.end(), "DataFrame::rename_: target name already exists");
  }

  w.names_[it->second] = std::move(new_name);
  w.validate_and_reindex_();
  return *this;
}

DataFrame DataFrame::head(std::int64_t n, bool render) const {
  const auto clipped = std::clamp<std::int64_t>(n, 0, nrows());
  auto out = slice(0, clipped);
  if (render) {
    display::print(std::cout, out);
    out.suppress_next_stream_();
  }
  return out;
}

DataFrame DataFrame::tail(std::int64_t n, bool render) const {
  const auto clipped = std::clamp<std::int64_t>(n, 0, nrows());
  auto out = slice(nrows() - clipped, nrows());
  if (render) {
    display::print(std::cout, out);
    out.suppress_next_stream_();
  }
  return out;
}

DataFrame DataFrame::slice(std::int64_t start, std::int64_t stop) const {
  const auto [s, e] = normalize_slice(start, stop, nrows());
  std::vector<std::int64_t> idx;
  idx.reserve(static_cast<std::size_t>(e - s));
  for (std::int64_t i = s; i < e; ++i) {
    idx.push_back(i);
  }
  return take(idx);
}

DataFrame DataFrame::take(std::span<const std::int64_t> indices) const {
  const auto& src = impl_ref_();
  std::vector<Thread> out_cols;
  out_cols.reserve(src.columns_.size());
  for (const auto& c : src.columns_) {
    out_cols.push_back(c.take(indices));
  }
  auto out_case = src.cases_.take(indices);
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(src.names_, std::move(out_cols), std::move(out_case)));
}

DataFrame DataFrame::where(const pluss::Series& bool_mask) const {
  return filter(bool_mask);
}

DataFrame DataFrame::filter(const pluss::Series& bool_mask) const {
  validate_bool_mask(bool_mask, nrows(), "DataFrame::filter");
  const auto& src = impl_ref_();
  std::vector<Thread> out_cols;
  out_cols.reserve(src.columns_.size());
  for (const auto& c : src.columns_) {
    out_cols.push_back(c.filter(bool_mask));
  }
  auto out_case = src.cases_.filter(bool_mask);
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(src.names_, std::move(out_cols), std::move(out_case)));
}

DataFrame DataFrame::query(const pluss::Series& bool_mask) const {
  return where(bool_mask);
}

DataFrame DataFrame::query(const QueryFn& predicate) const {
  PLUSS_CHECK(static_cast<bool>(predicate), "DataFrame::query: predicate is empty");
  return where(predicate(*this));
}

DataFrame DataFrame::query(std::string_view expression) const {
  const auto text = owned_name(expression);
  PLUSS_CHECK(!text.empty(), "DataFrame::query: expression cannot be empty");
  auto parser = QueryParser(*this, tokenize_query(text));
  auto eval = parser.parse();

  auto mask = pluss::Series::zeros({nrows()}, pluss::codem::ScalarType::Bool);
  auto* keep = reinterpret_cast<bool*>(mask.raw_data());
  for (std::int64_t i = 0; i < nrows(); ++i) {
    keep[i] = eval(i);
  }
  return where(mask);
}

DataFrame DataFrame::derive(std::initializer_list<std::pair<std::string, Thread>> assignments) const {
  DataFrame out = *this;
  for (const auto& assignment : assignments) {
    PLUSS_CHECK(!assignment.first.empty(), "DataFrame::derive: feature name cannot be empty");
    out = out.with_feature(assignment.first, assignment.second);
  }
  return out;
}

DataFrame DataFrame::derive(std::initializer_list<DeriveSpec> specs) const {
  DataFrame out = *this;
  for (const auto& spec : specs) {
    PLUSS_CHECK(!spec.feature.empty(), "DataFrame::derive: feature name cannot be empty");
    PLUSS_CHECK(static_cast<bool>(spec.expr), "DataFrame::derive: derive expression is empty");
    out = out.with_feature(spec.feature, spec.expr(out));
  }
  return out;
}

DataFrame DataFrame::assign(std::initializer_list<std::pair<std::string, Thread>> assignments) const {
  return derive(assignments);
}

DataFrame DataFrame::assign(std::initializer_list<DeriveSpec> specs) const {
  return derive(specs);
}

DataFrame DataFrame::summarize(std::initializer_list<SummarySpec> specs, bool render) const {
  PLUSS_CHECK(specs.size() != 0, "DataFrame::summarize: no SummarySpec supplied");

  std::vector<std::pair<std::string, Thread>> out_cols;
  out_cols.reserve(specs.size());

  std::unordered_set<std::string> names_seen;
  names_seen.reserve(specs.size());

  for (const auto& spec : specs) {
    PLUSS_CHECK(!spec.feature.empty(), "DataFrame::summarize: SummarySpec.feature cannot be empty");
    const auto feature_value = feature(spec.feature);
    const auto out_name =
        spec.output.empty() ? fmt::format("{}_{}", spec.feature, summary_op_name(spec.op)) : spec.output;
    PLUSS_CHECK(names_seen.insert(out_name).second, "DataFrame::summarize: duplicate output name '" + out_name + "'");

    if (spec.op == SummaryOp::Count) {
      out_cols.emplace_back(out_name, make_i64_thread({non_null_count(feature_value)}));
      continue;
    }
    if (spec.op == SummaryOp::Nunique) {
      out_cols.emplace_back(out_name, make_i64_thread({nunique_non_null(feature_value)}));
      continue;
    }
    out_cols.emplace_back(out_name, make_f64_thread({numeric_reduce_value(feature_value, spec.op)}));
  }

  auto out = DataFrame(std::move(out_cols), make_note_thread({"summary"}));
  if (render) {
    display::print(std::cout, out);
    out.suppress_next_stream_();
  }
  return out;
}

DataFrame DataFrame::agg(std::initializer_list<SummarySpec> specs, bool render) const {
  return summarize(specs, render);
}

DataFrame DataFrame::add(double other) const {
  return dataframe_math_scalar(*this, other, BinaryMathOp::Add, false);
}

DataFrame DataFrame::add(const DataFrame& other) const {
  return dataframe_math_frame(*this, other, BinaryMathOp::Add);
}

DataFrame DataFrame::sub(double other) const {
  return dataframe_math_scalar(*this, other, BinaryMathOp::Sub, false);
}

DataFrame DataFrame::sub(const DataFrame& other) const {
  return dataframe_math_frame(*this, other, BinaryMathOp::Sub);
}

DataFrame DataFrame::subtract(double other) const {
  return sub(other);
}

DataFrame DataFrame::subtract(const DataFrame& other) const {
  return sub(other);
}

DataFrame DataFrame::mul(double other) const {
  return dataframe_math_scalar(*this, other, BinaryMathOp::Mul, false);
}

DataFrame DataFrame::mul(const DataFrame& other) const {
  return dataframe_math_frame(*this, other, BinaryMathOp::Mul);
}

DataFrame DataFrame::multiply(double other) const {
  return mul(other);
}

DataFrame DataFrame::multiply(const DataFrame& other) const {
  return mul(other);
}

DataFrame DataFrame::div(double other) const {
  return dataframe_math_scalar(*this, other, BinaryMathOp::Div, false);
}

DataFrame DataFrame::div(const DataFrame& other) const {
  return dataframe_math_frame(*this, other, BinaryMathOp::Div);
}

DataFrame DataFrame::divide(double other) const {
  return div(other);
}

DataFrame DataFrame::divide(const DataFrame& other) const {
  return div(other);
}

DataFrame DataFrame::truediv(double other) const {
  return dataframe_math_scalar(*this, other, BinaryMathOp::TrueDiv, false);
}

DataFrame DataFrame::truediv(const DataFrame& other) const {
  return dataframe_math_frame(*this, other, BinaryMathOp::TrueDiv);
}

DataFrame DataFrame::floordiv(double other) const {
  return dataframe_math_scalar(*this, other, BinaryMathOp::FloorDiv, false);
}

DataFrame DataFrame::floordiv(const DataFrame& other) const {
  return dataframe_math_frame(*this, other, BinaryMathOp::FloorDiv);
}

DataFrame DataFrame::mod(double other) const {
  return dataframe_math_scalar(*this, other, BinaryMathOp::Mod, false);
}

DataFrame DataFrame::mod(const DataFrame& other) const {
  return dataframe_math_frame(*this, other, BinaryMathOp::Mod);
}

DataFrame DataFrame::pow(double other) const {
  return dataframe_math_scalar(*this, other, BinaryMathOp::Pow, false);
}

DataFrame DataFrame::pow(const DataFrame& other) const {
  return dataframe_math_frame(*this, other, BinaryMathOp::Pow);
}

DataFrame DataFrame::radd(double other) const {
  return dataframe_math_scalar(*this, other, BinaryMathOp::Add, true);
}

DataFrame DataFrame::rsub(double other) const {
  return dataframe_math_scalar(*this, other, BinaryMathOp::Sub, true);
}

DataFrame DataFrame::rmul(double other) const {
  return dataframe_math_scalar(*this, other, BinaryMathOp::Mul, true);
}

DataFrame DataFrame::rdiv(double other) const {
  return dataframe_math_scalar(*this, other, BinaryMathOp::Div, true);
}

DataFrame DataFrame::rtruediv(double other) const {
  return dataframe_math_scalar(*this, other, BinaryMathOp::TrueDiv, true);
}

DataFrame DataFrame::rfloordiv(double other) const {
  return dataframe_math_scalar(*this, other, BinaryMathOp::FloorDiv, true);
}

DataFrame DataFrame::rmod(double other) const {
  return dataframe_math_scalar(*this, other, BinaryMathOp::Mod, true);
}

DataFrame DataFrame::rpow(double other) const {
  return dataframe_math_scalar(*this, other, BinaryMathOp::Pow, true);
}

DataFrame DataFrame::eq(double other) const {
  return dataframe_cmp_scalar(*this, other, BinaryCmpOp::Eq);
}

DataFrame DataFrame::eq(const DataFrame& other) const {
  return dataframe_cmp_frame(*this, other, BinaryCmpOp::Eq);
}

DataFrame DataFrame::ne(double other) const {
  return dataframe_cmp_scalar(*this, other, BinaryCmpOp::Ne);
}

DataFrame DataFrame::ne(const DataFrame& other) const {
  return dataframe_cmp_frame(*this, other, BinaryCmpOp::Ne);
}

DataFrame DataFrame::lt(double other) const {
  return dataframe_cmp_scalar(*this, other, BinaryCmpOp::Lt);
}

DataFrame DataFrame::lt(const DataFrame& other) const {
  return dataframe_cmp_frame(*this, other, BinaryCmpOp::Lt);
}

DataFrame DataFrame::le(double other) const {
  return dataframe_cmp_scalar(*this, other, BinaryCmpOp::Le);
}

DataFrame DataFrame::le(const DataFrame& other) const {
  return dataframe_cmp_frame(*this, other, BinaryCmpOp::Le);
}

DataFrame DataFrame::gt(double other) const {
  return dataframe_cmp_scalar(*this, other, BinaryCmpOp::Gt);
}

DataFrame DataFrame::gt(const DataFrame& other) const {
  return dataframe_cmp_frame(*this, other, BinaryCmpOp::Gt);
}

DataFrame DataFrame::ge(double other) const {
  return dataframe_cmp_scalar(*this, other, BinaryCmpOp::Ge);
}

DataFrame DataFrame::ge(const DataFrame& other) const {
  return dataframe_cmp_frame(*this, other, BinaryCmpOp::Ge);
}

DataFrame DataFrame::compare(double other) const {
  return ne(other);
}

DataFrame DataFrame::compare(const DataFrame& other) const {
  return ne(other);
}

DataFrame DataFrame::missing(const MissingSpec& spec) const {
  const auto& src = impl_ref_();
  std::vector<std::size_t> target_indices;
  target_indices.reserve(spec.features.empty() ? src.names_.size() : spec.features.size());

  if (spec.features.empty()) {
    for (std::size_t i = 0; i < src.names_.size(); ++i) {
      target_indices.push_back(i);
    }
  } else {
    std::unordered_set<std::size_t> seen;
    seen.reserve(spec.features.size());
    for (const auto& name : spec.features) {
      const auto it = src.name_to_idx_.find(name);
      PLUSS_CHECK(it != src.name_to_idx_.end(), "DataFrame::missing: unknown feature '" + name + "'");
      if (seen.insert(it->second).second) {
        target_indices.push_back(it->second);
      }
    }
  }

  std::vector<bool> is_target(src.names_.size(), false);
  for (const auto idx : target_indices) {
    is_target[idx] = true;
  }

  if (spec.op == MissingOp::IsNull || spec.op == MissingOp::NotNull) {
    std::vector<std::pair<std::string, Thread>> out_cols;
    out_cols.reserve(target_indices.size());
    for (const auto idx : target_indices) {
      const auto mask = (spec.op == MissingOp::IsNull) ? src.columns_[idx].isnull() : src.columns_[idx].notnull();
      out_cols.emplace_back(src.names_[idx], Thread(mask));
    }
    return DataFrame(std::move(out_cols), src.cases_);
  }

  if (spec.op == MissingOp::DropAny || spec.op == MissingOp::DropAll) {
    if (src.nrows_ == 0 || target_indices.empty()) {
      return *this;
    }

    bool has_any_mask = false;
    std::vector<bool> keep(
        static_cast<std::size_t>(src.nrows_),
        spec.op == MissingOp::DropAny);

    for (const auto idx : target_indices) {
      const auto& col = src.columns_[idx];
      if (!col.has_validity_mask()) {
        if (spec.op == MissingOp::DropAll) {
          std::fill(keep.begin(), keep.end(), true);
        }
        continue;
      }

      has_any_mask = true;
      const auto valid = col.validity_mask()->to_vec<bool>();
      for (std::int64_t r = 0; r < src.nrows_; ++r) {
        const auto k = static_cast<std::size_t>(r);
        if (spec.op == MissingOp::DropAny) {
          keep[k] = keep[k] && valid[k];
        } else {
          keep[k] = keep[k] || valid[k];
        }
      }
    }

    if (!has_any_mask) {
      return *this;
    }
    return filter(bool_series_from_keep(keep));
  }

  if (spec.op == MissingOp::Fill) {
    PLUSS_CHECK(spec.value.has_value(), "DataFrame::missing(fill): value must be provided");
  }

  std::vector<Thread> out_cols;
  out_cols.reserve(src.columns_.size());
  for (std::size_t i = 0; i < src.columns_.size(); ++i) {
    const auto& col = src.columns_[i];
    if (!is_target[i]) {
      out_cols.push_back(col);
      continue;
    }

    switch (spec.op) {
      case MissingOp::Fill:
        if (col.kind() == ThreadKind::Series && col.has_validity_mask()) {
          out_cols.push_back(col.fillna(*spec.value));
        } else {
          out_cols.push_back(col);
        }
        break;
      case MissingOp::ForwardFill:
        out_cols.push_back(col.ffill());
        break;
      case MissingOp::BackwardFill:
        out_cols.push_back(col.bfill());
        break;
      default:
        out_cols.push_back(col);
        break;
    }
  }

  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(src.names_, std::move(out_cols), src.cases_));
}

DataFrame DataFrame::missing(
    MissingOp op,
    std::initializer_list<std::string_view> features,
    std::optional<double> value) const {
  MissingSpec spec;
  spec.op = op;
  spec.features = normalize_name_list(features);
  spec.value = value;
  return missing(spec);
}

DataFrame DataFrame::cast(std::initializer_list<CastSpec> specs) const {
  if (specs.size() == 0) {
    return *this;
  }

  const auto& src = impl_ref_();
  std::vector<Thread> out_cols = src.columns_;
  std::unordered_set<std::string> seen;
  seen.reserve(specs.size());

  for (const auto& spec : specs) {
    PLUSS_CHECK(!spec.feature.empty(), "DataFrame::cast: feature name cannot be empty");
    const auto it = src.name_to_idx_.find(spec.feature);
    PLUSS_CHECK(it != src.name_to_idx_.end(), "DataFrame::cast: unknown feature '" + spec.feature + "'");
    if (!seen.insert(spec.feature).second) {
      continue;
    }
    out_cols[it->second] = src.columns_[it->second].cast(spec.target, spec.series_dtype);
  }

  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(src.names_, std::move(out_cols), src.cases_));
}

DataFrame DataFrame::cast(
    std::initializer_list<std::pair<std::string_view, pluss::codem::ScalarType>> feature_dtypes) const {
  const auto& src = impl_ref_();
  std::vector<Thread> out_cols = src.columns_;
  std::unordered_set<std::string> seen;
  seen.reserve(feature_dtypes.size());
  for (const auto& [name, dtype] : feature_dtypes) {
    const auto key = owned_name(name);
    PLUSS_CHECK(!key.empty(), "DataFrame::cast: feature name cannot be empty");
    const auto it = src.name_to_idx_.find(key);
    PLUSS_CHECK(it != src.name_to_idx_.end(), "DataFrame::cast: unknown feature '" + key + "'");
    if (!seen.insert(key).second) {
      continue;
    }
    out_cols[it->second] = src.columns_[it->second].astype(dtype);
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(src.names_, std::move(out_cols), src.cases_));
}

DataFrame DataFrame::astype(
    std::initializer_list<std::pair<std::string_view, pluss::codem::ScalarType>> feature_dtypes) const {
  return cast(feature_dtypes);
}

DataFrame DataFrame::window(std::initializer_list<WindowSpec> specs, bool keep_input) const {
  PLUSS_CHECK(specs.size() != 0, "DataFrame::window: at least one WindowSpec is required");
  const auto& src = impl_ref_();

  std::vector<std::pair<std::string, Thread>> computed;
  computed.reserve(specs.size());

  std::unordered_set<std::string> names_seen;
  if (keep_input) {
    names_seen.insert(src.names_.begin(), src.names_.end());
  }

  for (const auto& spec : specs) {
    PLUSS_CHECK(!spec.feature.empty(), "DataFrame::window: WindowSpec.feature cannot be empty");
    const auto it = src.name_to_idx_.find(spec.feature);
    PLUSS_CHECK(it != src.name_to_idx_.end(), "DataFrame::window: unknown feature '" + spec.feature + "'");
    PLUSS_CHECK(spec.window > 0, "DataFrame::window: WindowSpec.window must be > 0");
    PLUSS_CHECK(spec.min_periods > 0, "DataFrame::window: WindowSpec.min_periods must be > 0");

    auto out_name = spec.output;
    if (out_name.empty()) {
      out_name = fmt::format("{}_{}_{}", spec.feature, window_op_name(spec.op), spec.window);
    }

    if (!keep_input) {
      PLUSS_CHECK(names_seen.insert(out_name).second,
                  "DataFrame::window: duplicate output feature '" + out_name + "'");
    }

    computed.emplace_back(
        out_name,
        src.columns_[it->second].window(spec.window, spec.op, spec.min_periods));
  }

  if (!keep_input) {
    return DataFrame(std::move(computed), src.cases_);
  }

  auto out_names = src.names_;
  auto out_cols = src.columns_;
  auto out_name_to_idx = src.name_to_idx_;

  for (auto& [name, thread] : computed) {
    const auto it = out_name_to_idx.find(name);
    if (it != out_name_to_idx.end()) {
      out_cols[it->second] = std::move(thread);
      continue;
    }
    const auto next = out_cols.size();
    out_name_to_idx.emplace(name, next);
    out_names.push_back(name);
    out_cols.push_back(std::move(thread));
  }

  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      std::move(out_names), std::move(out_cols), src.cases_));
}

DataFrame DataFrame::rolling(std::initializer_list<WindowSpec> specs, bool keep_input) const {
  return window(specs, keep_input);
}

DataFrame DataFrame::fillna(double value) const {
  MissingSpec spec;
  spec.op = MissingOp::Fill;
  spec.value = value;
  return missing(spec);
}

DataFrame& DataFrame::fillna_(double value) {
  auto& w = mut_impl_();
  for (auto& c : w.columns_) {
    if (c.kind() == ThreadKind::Series && c.has_validity_mask()) {
      c.fillna_(value);
    }
  }
  return *this;
}

std::int64_t DataFrame::count_null_rows() const {
  const auto& src = impl_ref_();
  if (src.nrows_ == 0) {
    return 0;
  }

  std::vector<bool> has_null(static_cast<std::size_t>(src.nrows_), false);
  for (const auto& c : src.columns_) {
    if (!c.has_validity_mask()) {
      continue;
    }
    const auto valid = c.validity_mask()->to_vec<bool>();
    for (std::int64_t i = 0; i < src.nrows_; ++i) {
      if (!valid[static_cast<std::size_t>(i)]) {
        has_null[static_cast<std::size_t>(i)] = true;
      }
    }
  }

  std::int64_t n = 0;
  for (const bool v : has_null) {
    if (v) {
      ++n;
    }
  }
  return n;
}

DataFrame DataFrame::dropna() const {
  return missing(MissingOp::DropAny);
}

DataFrame DataFrame::ffill() const {
  return missing(MissingOp::ForwardFill);
}

DataFrame DataFrame::bfill() const {
  return missing(MissingOp::BackwardFill);
}

DataFrame DataFrame::describe(bool render) const {
  const auto& src = impl_ref_();
  std::vector<std::pair<std::string, Thread>> out_cols;
  out_cols.reserve(src.columns_.size());

  for (std::size_t i = 0; i < src.columns_.size(); ++i) {
    const auto& col = src.columns_[i];
    if (col.kind() != ThreadKind::Series) {
      continue;
    }
    const auto& s = col.as_series();
    if (!is_numeric_series_type(s)) {
      continue;
    }

    std::vector<double> values;
    values.reserve(static_cast<std::size_t>(col.size()));
    if (col.has_validity_mask()) {
      const auto valid = col.validity_mask()->to_vec<bool>();
      for (std::int64_t r = 0; r < col.size(); ++r) {
        if (!valid[static_cast<std::size_t>(r)]) {
          continue;
        }
        double v = 0.0;
        if (extract_numeric_value(s, r, v)) {
          values.push_back(v);
        }
      }
    } else {
      for (std::int64_t r = 0; r < col.size(); ++r) {
        double v = 0.0;
        if (extract_numeric_value(s, r, v)) {
          values.push_back(v);
        }
      }
    }

    const double count = static_cast<double>(values.size());
    double mean = std::numeric_limits<double>::quiet_NaN();
    double stdv = std::numeric_limits<double>::quiet_NaN();
    double minv = std::numeric_limits<double>::quiet_NaN();
    double q25 = std::numeric_limits<double>::quiet_NaN();
    double q50 = std::numeric_limits<double>::quiet_NaN();
    double q75 = std::numeric_limits<double>::quiet_NaN();
    double maxv = std::numeric_limits<double>::quiet_NaN();

    if (!values.empty()) {
      std::sort(values.begin(), values.end());
      minv = values.front();
      maxv = values.back();
      const double sum = std::accumulate(values.begin(), values.end(), 0.0);
      mean = sum / count;
      q25 = quantile_sorted(values, 0.25);
      q50 = quantile_sorted(values, 0.50);
      q75 = quantile_sorted(values, 0.75);

      if (values.size() > 1) {
        double sq = 0.0;
        for (const double v : values) {
          const double d = v - mean;
          sq += d * d;
        }
        stdv = std::sqrt(sq / static_cast<double>(values.size() - 1));
      }
    }

    std::vector<double> stats{count, mean, stdv, minv, q25, q50, q75, maxv};
    out_cols.emplace_back(src.names_[i], make_f64_thread(stats));
  }

  PLUSS_CHECK(!out_cols.empty(), "DataFrame::describe: no numeric columns to describe");

  auto out = DataFrame(
      std::move(out_cols),
      make_note_thread({"count", "mean", "std", "min", "25%", "50%", "75%", "max"}));
  if (render) {
    display::print(std::cout, out);
    out.suppress_next_stream_();
  }
  return out;
}

DataFrame DataFrame::view(std::int64_t start, std::int64_t stop, bool render) const {
  const auto n = nrows();
  const auto s = normalize_view_start(start, n);
  const auto e = normalize_view_stop(stop, n);

  DataFrame out;
  if (e <= s) {
    out = slice(s, s);
  } else if (s == 0) {
    out = head(e, false);
  } else if (e == n) {
    out = tail(n - s, false);
  } else {
    auto after_start = tail(n - s, false);
    out = after_start.head(e - s, false);
  }

  if (render) {
    display::print(std::cout, out);
    out.suppress_next_stream_();
  }
  return out;
}

std::string DataFrame::to_string(const display::DisplayOptions& options) const {
  return display::render(*this, options);
}

std::string DataFrame::info_string(const display::DisplayOptions& options) const {
  return display::render_info(*this, options);
}

void DataFrame::info(const display::DisplayOptions& options) const {
  display::print_info(std::cout, *this, options);
}

void DataFrame::info(std::ostream& os, const display::DisplayOptions& options) const {
  display::print_info(os, *this, options);
}

void DataFrame::save_csv(std::string_view filepath) const {
  pluss::loader::save_csv(*this, filepath, {});
}

void DataFrame::save_csv(std::string_view filepath,
                         bool add_cases,
                         bool header,
                         char sep,
                         std::string na_rep,
                         std::optional<std::string> case_label,
                         std::optional<std::string> float_format,
                         char quotechar,
                         bool doublequote,
                         std::string line_terminator,
                         char decimal,
                         std::string encoding) const {
  pluss::loader::WriteCsvOptions options;
  options.sep = sep;
  options.header = header;
  options.add_cases = add_cases;
  options.cases = add_cases;
  options.na_rep = std::move(na_rep);
  options.case_label = std::move(case_label);
  options.float_format = std::move(float_format);
  options.quotechar = quotechar;
  options.doublequote = doublequote;
  options.line_terminator = std::move(line_terminator);
  options.decimal = decimal;
  options.encoding = std::move(encoding);
  pluss::loader::save_csv(*this, filepath, options);
}

void DataFrame::save_csv(std::string_view filepath, const pluss::loader::WriteCsvOptions& options) const {
  pluss::loader::save_csv(*this, filepath, options);
}

void DataFrame::save_csv(std::string_view filepath, const pluss::loader::Config& config) const {
  save_as("csv", filepath, config);
}

void DataFrame::save_csv(std::string_view filepath,
                         std::initializer_list<pluss::loader::ConfigEntry> config_entries) const {
  save_as("csv", filepath, config_entries);
}

void DataFrame::save_excel(std::string_view filepath) const {
  pluss::loader::save_excel(*this, filepath, {});
}

void DataFrame::save_excel(std::string_view filepath,
                           std::string sheet_name,
                           bool add_cases,
                           bool header,
                           std::string na_rep,
                           std::optional<std::string> case_label) const {
  pluss::loader::WriteExcelOptions options;
  options.sheet_name = std::move(sheet_name);
  options.add_cases = add_cases;
  options.cases = add_cases;
  options.header = header;
  options.na_rep = std::move(na_rep);
  options.case_label = std::move(case_label);
  pluss::loader::save_excel(*this, filepath, options);
}

void DataFrame::save_excel(std::string_view filepath, const pluss::loader::WriteExcelOptions& options) const {
  pluss::loader::save_excel(*this, filepath, options);
}

void DataFrame::save_excel(std::string_view filepath, const pluss::loader::Config& config) const {
  save_as("excel", filepath, config);
}

void DataFrame::save_excel(std::string_view filepath,
                           std::initializer_list<pluss::loader::ConfigEntry> config_entries) const {
  save_as("excel", filepath, config_entries);
}

void DataFrame::save_as(std::string_view target_type,
                        std::string_view filepath,
                        const pluss::loader::Config& config) const {
  pluss::loader::save_as(*this, target_type, filepath, config);
}

void DataFrame::save_as(std::string_view target_type,
                        std::string_view filepath,
                        std::initializer_list<pluss::loader::ConfigEntry> config_entries) const {
  save_as(target_type, filepath, pluss::loader::config_from(config_entries));
}

std::ostream& operator<<(std::ostream& os, const DataFrame& frame) {
  if (frame.consume_stream_suppression_()) {
    return os;
  }
  os << frame.to_string();
  return os;
}

} // namespace pluss::table
