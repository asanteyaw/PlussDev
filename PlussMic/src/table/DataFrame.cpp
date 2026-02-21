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
#include <random>
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
#include "signal/catalog.h"
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

bool is_number_series_type(const pluss::Series& s) {
  const auto dt = s.dtype();
  return dt.scalar == pluss::codem::ScalarType::Bool || dt.is_integral() || dt.is_floating() || dt.is_complex();
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

Thread make_f64_thread(const std::vector<double>& values, const std::vector<bool>& valid) {
  PLUSS_CHECK(values.size() == valid.size(), "make_f64_thread: values/valid length mismatch");
  const auto n = static_cast<std::int64_t>(values.size());
  auto s = pluss::Series::empty({n}, pluss::codem::ScalarType::Float64);
  if (!values.empty()) {
    std::memcpy(s.raw_data(), values.data(), values.size() * sizeof(double));
  }
  return Thread(s, bool_series_from_keep(valid));
}

Thread make_note_thread(std::initializer_list<std::string_view> labels) {
  std::vector<std::string_view> views(labels.begin(), labels.end());
  const std::array<std::int64_t, 1> shape{static_cast<std::int64_t>(views.size())};
  auto note = pluss::coden::ops_internal::note_from_utf8(
      std::span<const std::string_view>(views.data(), views.size()),
      std::span<const std::int64_t>(shape.data(), shape.size()));
  return Thread(note);
}

Thread make_note_thread(const std::vector<std::string>& labels) {
  std::vector<std::string_view> views;
  views.reserve(labels.size());
  for (const auto& v : labels) {
    views.push_back(v);
  }
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

std::string group_op_name(DataFrame::GroupOp op) {
  switch (op) {
    case DataFrame::GroupOp::Count:
      return "count";
    case DataFrame::GroupOp::Nunique:
      return "nunique";
    case DataFrame::GroupOp::Sum:
      return "sum";
    case DataFrame::GroupOp::Mean:
      return "mean";
    case DataFrame::GroupOp::Min:
      return "min";
    case DataFrame::GroupOp::Max:
      return "max";
    default:
      return "group";
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

  // Delegate numeric reductions to Thread to keep 1D math semantics in Thread.
  const auto clean = t.dropna();
  if (clean.size() == 0) {
    return std::numeric_limits<double>::quiet_NaN();
  }

  auto scalar_series_to_double = [](const pluss::Series& s, std::string_view where) {
    PLUSS_CHECK(s.defined(), std::string(where) + ": scalar reduction produced undefined Series");
    PLUSS_CHECK(s.numel() == 1, std::string(where) + ": scalar reduction must have numel=1");
    double out = 0.0;
    PLUSS_CHECK(extract_numeric_value(s, 0, out), std::string(where) + ": non-numeric scalar reduction result");
    return out;
  };

  switch (op) {
    case DataFrame::SummaryOp::Sum:
      return scalar_series_to_double(clean.sum(), "DataFrame::summarize(sum)");
    case DataFrame::SummaryOp::Mean:
      return scalar_series_to_double(clean.mean(), "DataFrame::summarize(mean)");
    case DataFrame::SummaryOp::Min:
      return scalar_series_to_double(clean.min(), "DataFrame::summarize(min)");
    case DataFrame::SummaryOp::Max:
      return scalar_series_to_double(clean.max(), "DataFrame::summarize(max)");
    case DataFrame::SummaryOp::Std:
      return scalar_series_to_double(clean.std(true), "DataFrame::summarize(std)");
    case DataFrame::SummaryOp::Var:
      return scalar_series_to_double(clean.var(true), "DataFrame::summarize(var)");
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
    keys.push_back(frame.features(spec.feature));
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
    return frame_.features(ident);
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

  if (expected_rows.has_value()) {
    nrows_ = *expected_rows;
  } else if (cases_.defined()) {
    nrows_ = cases_.size();
  } else {
    nrows_ = 0;
  }

  if (!cases_.defined()) {
    cases_ = default_cases(nrows_);
  }
  PLUSS_CHECK(cases_.defined(), "DataFrame: cases are undefined");
  PLUSS_CHECK(cases_.size() == nrows_,
              fmt::format("DataFrame: case length mismatch (cases={}, nrows={}, cols={})",
                          cases_.size(),
                          nrows_,
                          columns_.size()));
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
pluss::Series ColumnRef::mean() const { return column_.mean(); }
Thread ColumnRef::head(std::int64_t n) const { return column_.head(n); }
ThreadValue ColumnRef::value() const { return column_.value(); }
Thread ColumnRef::values() const { return column_; }

DataFrame::LetSlot::LetSlot(DataFrame& owner, std::string name) : owner_(&owner), name_(std::move(name)) {
  PLUSS_CHECK(owner_ != nullptr, "DataFrame::LetSlot: owner must be defined");
  PLUSS_CHECK(!name_.empty(), "DataFrame::set: feature name cannot be empty");
}

DataFrame::LetSlot& DataFrame::LetSlot::operator=(const Thread& value) {
  PLUSS_CHECK(value.defined(), "DataFrame::set(name)=Thread requires defined rhs thread");
  if (owner_->ncols() > 0) {
    PLUSS_CHECK(value.size() == owner_->nrows(),
                "DataFrame::set(name)=Thread requires rhs length to match DataFrame cases");
  }
  owner_->set_features_(name_, value);
  return *this;
}

DataFrame::LetSlot& DataFrame::LetSlot::operator=(const ThreadValue& value) {
  return (*this = value.thread());
}

DataFrame::LetSlot& DataFrame::LetSlot::operator=(const DataFrame& frame) {
  PLUSS_CHECK(frame.ncols() == 1, "DataFrame::set(name)=DataFrame requires rhs with exactly one feature");
  const auto cols = frame.features();
  PLUSS_CHECK(cols.size() == 1, "DataFrame::set(name)=DataFrame requires rhs with exactly one feature");
  return (*this = cols.front());
}

DataFrame::LetSlots::LetSlots(DataFrame& owner, std::vector<std::string> names)
    : owner_(&owner), names_(std::move(names)) {
  PLUSS_CHECK(owner_ != nullptr, "DataFrame::LetSlots: owner must be defined");
  PLUSS_CHECK(!names_.empty(), "DataFrame::set(names): at least one feature name is required");
  std::unordered_set<std::string> seen;
  seen.reserve(names_.size());
  for (const auto& n : names_) {
    PLUSS_CHECK(!n.empty(), "DataFrame::set(names): feature name cannot be empty");
    PLUSS_CHECK(seen.insert(n).second, "DataFrame::set(names): duplicate feature name '" + n + "'");
  }
}

void DataFrame::LetSlots::assign_from_threads(std::span<const Thread> values) {
  PLUSS_CHECK(values.size() == names_.size(),
              "DataFrame::set(names): rhs feature count must match lhs feature count");

  std::optional<std::int64_t> expected_rows;
  if (owner_->ncols() > 0) {
    expected_rows = owner_->nrows();
  }

  for (const auto& value : values) {
    PLUSS_CHECK(value.defined(), "DataFrame::set(names): rhs thread must be defined");
    if (!expected_rows.has_value()) {
      expected_rows = value.size();
    } else {
      PLUSS_CHECK(value.size() == *expected_rows,
                  "DataFrame::set(names): rhs thread lengths must match target DataFrame cases");
    }
  }

  for (std::size_t i = 0; i < names_.size(); ++i) {
    owner_->set_features_(names_[i], values[i]);
  }
}

DataFrame::LetSlots& DataFrame::LetSlots::operator=(const DataFrame& frame) {
  const auto cols = frame.features();
  assign_from_threads(std::span<const Thread>(cols.data(), cols.size()));
  return *this;
}

DataFrame::LetSlots& DataFrame::LetSlots::operator=(std::initializer_list<Thread> values) {
  assign_from_threads(std::span<const Thread>(values.begin(), values.size()));
  return *this;
}

DataFrame::LetSlots& DataFrame::LetSlots::operator=(const std::vector<Thread>& values) {
  assign_from_threads(std::span<const Thread>(values.data(), values.size()));
  return *this;
}

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

pluss::signal::Catalog DataFrame::signalize() const {
  return signalize(pluss::signal::SignalizePolicy{});
}

pluss::signal::Catalog DataFrame::signalize(const pluss::signal::SignalizePolicy& policy) const {
  PLUSS_CHECK(defined(), "DataFrame::signalize requires a defined DataFrame");
  return pluss::signal::Catalog::signalize(*this, policy);
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

DataFrame DataFrame::iloc(std::int64_t row) const {
  return cases(row);
}

DataFrame DataFrame::iloc(std::int64_t start, std::int64_t stop) const {
  return slice(start, stop);
}

std::string DataFrame::iat(std::int64_t row, std::size_t feature_idx) const {
  return cases(row, feature_idx);
}

std::string DataFrame::at(std::int64_t row, std::string_view feature_name) const {
  return cases(row, feature_name);
}

std::optional<Thread> DataFrame::get(std::string_view feature_name) const {
  if (!has_feature(feature_name)) {
    return std::nullopt;
  }
  return features(feature_name);
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

Thread DataFrame::features(std::string_view name) { return column(name); }
Thread DataFrame::features(std::string_view name) const { return column(name); }
pluss::Series DataFrame::series_value(std::string_view feature_name) const { return features(feature_name).as_series(); }
pluss::Note DataFrame::note_value(std::string_view feature_name) const { return features(feature_name).as_note(); }
pluss::Timeline DataFrame::timeline_value(std::string_view feature_name) const { return features(feature_name).as_timeline(); }
pluss::TimeIndex DataFrame::timeindex_value(std::string_view feature_name) const { return features(feature_name).as_timeindex(); }

ColumnRef DataFrame::column_ref(std::string_view name) const {
  return ColumnRef(owned_name(name), column(name));
}

ColumnRef DataFrame::feature_ref(std::string_view name) const { return column_ref(name); }

DataFrame DataFrame::features(std::initializer_list<std::string_view> names) {
  return select(names);
}

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

DataFrame DataFrame::series() const {
  const auto& src = impl_ref_();
  std::vector<std::string> out_names;
  std::vector<Thread> out_cols;
  out_names.reserve(src.names_.size());
  out_cols.reserve(src.columns_.size());
  for (std::size_t i = 0; i < src.columns_.size(); ++i) {
    if (src.columns_[i].kind() != ThreadKind::Series) {
      continue;
    }
    out_names.push_back(src.names_[i]);
    out_cols.push_back(src.columns_[i]);
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      std::move(out_names), std::move(out_cols), src.cases_));
}

DataFrame DataFrame::note() const {
  const auto& src = impl_ref_();
  std::vector<std::string> out_names;
  std::vector<Thread> out_cols;
  out_names.reserve(src.names_.size());
  out_cols.reserve(src.columns_.size());
  for (std::size_t i = 0; i < src.columns_.size(); ++i) {
    if (src.columns_[i].kind() != ThreadKind::Note) {
      continue;
    }
    out_names.push_back(src.names_[i]);
    out_cols.push_back(src.columns_[i]);
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      std::move(out_names), std::move(out_cols), src.cases_));
}

DataFrame DataFrame::timeline() const {
  const auto& src = impl_ref_();
  std::vector<std::string> out_names;
  std::vector<Thread> out_cols;
  out_names.reserve(src.names_.size());
  out_cols.reserve(src.columns_.size());
  for (std::size_t i = 0; i < src.columns_.size(); ++i) {
    const auto k = src.columns_[i].kind();
    if (k != ThreadKind::Timeline && k != ThreadKind::TimeIndex) {
      continue;
    }
    out_names.push_back(src.names_[i]);
    out_cols.push_back(src.columns_[i]);
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      std::move(out_names), std::move(out_cols), src.cases_));
}

DataFrame DataFrame::cast_series(std::initializer_list<std::string_view> features,
                                 std::optional<pluss::codem::ScalarType> dtype) const {
  if (features.size() == 0) {
    return *this;
  }
  const auto& src = impl_ref_();
  std::vector<Thread> out_cols = src.columns_;
  std::unordered_set<std::string> seen;
  seen.reserve(features.size());
  for (const auto name_sv : features) {
    const auto name = owned_name(name_sv);
    PLUSS_CHECK(!name.empty(), "DataFrame::cast_series: feature name cannot be empty");
    const auto it = src.name_to_idx_.find(name);
    PLUSS_CHECK(it != src.name_to_idx_.end(), "DataFrame::cast_series: unknown feature '" + name + "'");
    if (!seen.insert(name).second) {
      continue;
    }
    out_cols[it->second] = src.columns_[it->second].cast(ThreadCastTarget::Series, dtype);
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(src.names_, std::move(out_cols), src.cases_));
}

DataFrame DataFrame::cast_note(std::initializer_list<std::string_view> features) const {
  if (features.size() == 0) {
    return *this;
  }
  const auto& src = impl_ref_();
  std::vector<Thread> out_cols = src.columns_;
  std::unordered_set<std::string> seen;
  seen.reserve(features.size());
  for (const auto name_sv : features) {
    const auto name = owned_name(name_sv);
    PLUSS_CHECK(!name.empty(), "DataFrame::cast_note: feature name cannot be empty");
    const auto it = src.name_to_idx_.find(name);
    PLUSS_CHECK(it != src.name_to_idx_.end(), "DataFrame::cast_note: unknown feature '" + name + "'");
    if (!seen.insert(name).second) {
      continue;
    }
    out_cols[it->second] = src.columns_[it->second].cast(ThreadCastTarget::Note, std::nullopt);
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(src.names_, std::move(out_cols), src.cases_));
}

DataFrame DataFrame::cast_timeline(std::initializer_list<std::string_view> features) const {
  if (features.size() == 0) {
    return *this;
  }
  const auto& src = impl_ref_();
  std::vector<Thread> out_cols = src.columns_;
  std::unordered_set<std::string> seen;
  seen.reserve(features.size());
  for (const auto name_sv : features) {
    const auto name = owned_name(name_sv);
    PLUSS_CHECK(!name.empty(), "DataFrame::cast_timeline: feature name cannot be empty");
    const auto it = src.name_to_idx_.find(name);
    PLUSS_CHECK(it != src.name_to_idx_.end(), "DataFrame::cast_timeline: unknown feature '" + name + "'");
    if (!seen.insert(name).second) {
      continue;
    }
    out_cols[it->second] = src.columns_[it->second].cast(ThreadCastTarget::Timeline, std::nullopt);
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(src.names_, std::move(out_cols), src.cases_));
}

DataFrame DataFrame::cast_timeindex(std::initializer_list<std::string_view> features) const {
  if (features.size() == 0) {
    return *this;
  }
  const auto& src = impl_ref_();
  std::vector<Thread> out_cols = src.columns_;
  std::unordered_set<std::string> seen;
  seen.reserve(features.size());
  for (const auto name_sv : features) {
    const auto name = owned_name(name_sv);
    PLUSS_CHECK(!name.empty(), "DataFrame::cast_timeindex: feature name cannot be empty");
    const auto it = src.name_to_idx_.find(name);
    PLUSS_CHECK(it != src.name_to_idx_.end(), "DataFrame::cast_timeindex: unknown feature '" + name + "'");
    if (!seen.insert(name).second) {
      continue;
    }
    out_cols[it->second] = src.columns_[it->second].cast(ThreadCastTarget::TimeIndex, std::nullopt);
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(src.names_, std::move(out_cols), src.cases_));
}

DataFrame DataFrame::astype(std::initializer_list<std::string_view> features,
                            pluss::codem::ScalarType dtype) const {
  return cast_series(features, dtype);
}

DataFrame DataFrame::insert(std::int64_t feature_index, std::string name, const Thread& feature_value) const {
  const auto& src = impl_ref_();
  PLUSS_CHECK(!name.empty(), "DataFrame::insert: feature name cannot be empty");
  PLUSS_CHECK(feature_value.defined(), "DataFrame::insert: feature is undefined");
  if (!src.columns_.empty()) {
    PLUSS_CHECK(feature_value.size() == src.nrows_, "DataFrame::insert: case count mismatch");
  }
  PLUSS_CHECK(src.name_to_idx_.find(name) == src.name_to_idx_.end(),
              "DataFrame::insert: duplicate feature name '" + name + "'");

  const auto pos = std::clamp<std::int64_t>(feature_index, 0, static_cast<std::int64_t>(src.names_.size()));
  auto out_names = src.names_;
  auto out_cols = src.columns_;
  out_names.insert(out_names.begin() + pos, std::move(name));
  out_cols.insert(out_cols.begin() + pos, feature_value);
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

DataFrame DataFrame::add_prefix(std::string_view prefix) const {
  const auto& src = impl_ref_();
  std::vector<std::string> out_names;
  out_names.reserve(src.names_.size());
  for (const auto& name : src.names_) {
    out_names.push_back(fmt::format("{}{}", prefix, name));
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(std::move(out_names), src.columns_, src.cases_));
}

DataFrame DataFrame::add_suffix(std::string_view suffix) const {
  const auto& src = impl_ref_();
  std::vector<std::string> out_names;
  out_names.reserve(src.names_.size());
  for (const auto& name : src.names_) {
    out_names.push_back(fmt::format("{}{}", name, suffix));
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(std::move(out_names), src.columns_, src.cases_));
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

DataFrame DataFrame::sort_values(std::initializer_list<std::string_view> features,
                                 bool ascending,
                                 bool nulls_last) const {
  return arrange(features, ascending, nulls_last);
}

DataFrame DataFrame::sort_values(std::initializer_list<ArrangeSpec> specs) const {
  return arrange(specs);
}

DataFrame DataFrame::sort_cases(bool ascending, bool nulls_last) const {
  std::vector<std::int64_t> idx(static_cast<std::size_t>(nrows()));
  std::iota(idx.begin(), idx.end(), 0);
  const auto& case_axis = cases();
  std::stable_sort(idx.begin(), idx.end(), [&](std::int64_t a, std::int64_t b) {
    const auto cmp = case_axis.compare_rows(a, b, nulls_last);
    return ascending ? (cmp < 0) : (cmp > 0);
  });
  return take(idx);
}

DataFrame DataFrame::top(std::string_view feature_name, std::int64_t n) const {
  PLUSS_CHECK(n >= 0, "DataFrame::top: n must be >= 0");
  if (n == 0) {
    return slice(0, 0);
  }
  return sort_values({ArrangeSpec{owned_name(feature_name), false, true}})
      .head(n, false);
}

DataFrame DataFrame::bottom(std::string_view feature_name, std::int64_t n) const {
  PLUSS_CHECK(n >= 0, "DataFrame::bottom: n must be >= 0");
  if (n == 0) {
    return slice(0, 0);
  }
  return sort_values({ArrangeSpec{owned_name(feature_name), true, true}})
      .head(n, false);
}

pluss::Series DataFrame::repeated_cases(std::initializer_list<std::string_view> features) const {
  const auto& src = impl_ref_();
  std::vector<std::size_t> feature_idx;
  if (features.size() == 0) {
    feature_idx.resize(src.columns_.size());
    std::iota(feature_idx.begin(), feature_idx.end(), static_cast<std::size_t>(0));
  } else {
    std::unordered_set<std::size_t> seen;
    seen.reserve(features.size());
    for (const auto f : features) {
      const auto it = src.name_to_idx_.find(owned_name(f));
      PLUSS_CHECK(it != src.name_to_idx_.end(), "DataFrame::repeated_cases: unknown feature '" + owned_name(f) + "'");
      if (seen.insert(it->second).second) {
        feature_idx.push_back(it->second);
      }
    }
  }

  auto out = pluss::Series::zeros({nrows()}, pluss::codem::ScalarType::Bool);
  auto* mask = reinterpret_cast<bool*>(out.raw_data());

  std::unordered_set<std::string> seen_keys;
  seen_keys.reserve(static_cast<std::size_t>(nrows()));
  for (std::int64_t r = 0; r < src.nrows_; ++r) {
    std::string key;
    key.reserve(feature_idx.size() * 16);
    for (const auto idx : feature_idx) {
      const auto& col = src.columns_[idx];
      if (!col.is_valid(r)) {
        key.append("\x1D", 1);
      } else {
        key += thread_scalar_string_at(col, r);
      }
      key.append("\x1E", 1);
    }
    const auto inserted = seen_keys.insert(std::move(key)).second;
    mask[r] = !inserted;
  }
  return out;
}

DataFrame DataFrame::unique_cases(std::initializer_list<std::string_view> features) const {
  const auto dup = repeated_cases(features).to_vec<bool>();
  auto keep = pluss::Series::zeros({nrows()}, pluss::codem::ScalarType::Bool);
  auto* p = reinterpret_cast<bool*>(keep.raw_data());
  for (std::int64_t i = 0; i < nrows(); ++i) {
    p[i] = !dup[static_cast<std::size_t>(i)];
  }
  return filter(keep);
}

DataFrame DataFrame::set_cases(std::string_view feature_name, bool drop_feature) const {
  auto out = with_cases(features(feature_name));
  if (drop_feature) {
    out = out.drop({feature_name});
  }
  return out;
}

DataFrame DataFrame::reset_cases(std::string case_feature) const {
  auto out = with_column(case_feature, cases());
  return out.with_cases(default_cases(out.nrows()));
}

DataFrame DataFrame::align_cases(const Thread& new_cases) const {
  PLUSS_CHECK(new_cases.defined(), "DataFrame::align_cases: new cases are undefined");
  std::unordered_map<std::string, std::int64_t> old_pos;
  old_pos.reserve(static_cast<std::size_t>(nrows()));
  for (std::int64_t i = 0; i < nrows(); ++i) {
    old_pos.emplace(thread_scalar_string_at(cases(), i), i);
  }

  std::vector<std::int64_t> indices;
  indices.reserve(static_cast<std::size_t>(new_cases.size()));
  for (std::int64_t i = 0; i < new_cases.size(); ++i) {
    const auto key = thread_scalar_string_at(new_cases, i);
    const auto it = old_pos.find(key);
    PLUSS_CHECK(it != old_pos.end(),
                "DataFrame::align_cases: case label not found '" + key + "'");
    indices.push_back(it->second);
  }
  return take(indices).with_cases(new_cases);
}

DataFrame DataFrame::join(const DataFrame& other, std::string collision_suffix) const {
  CombineSpec spec;
  spec.op = CombineOp::Features;
  spec.collision_suffix = std::move(collision_suffix);
  return combine(other, spec);
}

DataFrame DataFrame::update(const DataFrame& other, bool overwrite) const {
  CombineSpec spec;
  spec.op = CombineOp::Update;
  spec.overwrite = overwrite;
  return combine(other, spec);
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

DataFrame::LetSlot DataFrame::set(std::string_view name) {
  return LetSlot(*this, owned_name(name));
}

DataFrame::LetSlots DataFrame::set(std::initializer_list<std::string_view> names) {
  return LetSlots(*this, normalize_name_list(names));
}

DataFrame DataFrame::set_features(std::string_view name, const Thread& value) const {
  return with_column(owned_name(name), value);
}

DataFrame& DataFrame::set_features_(std::string_view name, const Thread& value) {
  return with_column_(owned_name(name), value);
}

DataFrame DataFrame::set_features(std::initializer_list<std::string_view> names,
                                  std::initializer_list<Thread> values) const {
  PLUSS_CHECK(names.size() == values.size(),
              "DataFrame::set_features: names/value size mismatch");
  DataFrame out = *this;
  auto value_it = values.begin();
  for (const auto name : names) {
    out = out.with_column(owned_name(name), *value_it);
    ++value_it;
  }
  return out;
}

DataFrame& DataFrame::set_features_(std::initializer_list<std::string_view> names,
                                    std::initializer_list<Thread> values) {
  PLUSS_CHECK(names.size() == values.size(),
              "DataFrame::set_features_: names/value size mismatch");
  auto value_it = values.begin();
  for (const auto name : names) {
    with_column_(owned_name(name), *value_it);
    ++value_it;
  }
  return *this;
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

DataFrame DataFrame::rename_features(
    std::initializer_list<std::pair<std::string_view, std::string_view>> renames) const {
  DataFrame out = *this;
  for (const auto& [old_name, new_name] : renames) {
    out = out.rename(old_name, std::string(new_name));
  }
  return out;
}

DataFrame DataFrame::relabel_features(std::initializer_list<std::string_view> feature_names) const {
  const auto& src = impl_ref_();
  PLUSS_CHECK(feature_names.size() == src.names_.size(),
              "DataFrame::relabel_features: number of names must match feature count");
  std::vector<std::string> out_names;
  out_names.reserve(feature_names.size());
  std::unordered_set<std::string> seen;
  seen.reserve(feature_names.size());
  for (const auto n : feature_names) {
    const auto key = owned_name(n);
    PLUSS_CHECK(!key.empty(), "DataFrame::relabel_features: feature name cannot be empty");
    PLUSS_CHECK(seen.insert(key).second,
                "DataFrame::relabel_features: duplicate feature name '" + key + "'");
    out_names.push_back(key);
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      std::move(out_names), src.columns_, src.cases_));
}

DataFrame DataFrame::set_feature_axis(std::initializer_list<std::string_view> feature_names) const {
  return relabel_features(feature_names);
}

DataFrame DataFrame::set_axis(std::initializer_list<std::string_view> feature_names) const {
  return set_feature_axis(feature_names);
}

DataFrame DataFrame::conform_to(const DataFrame& other) const {
  const auto& src = impl_ref_();
  const auto ordered = other.feature_names();
  PLUSS_CHECK(ordered.size() == src.names_.size(),
              "DataFrame::conform_to: both frames must have same feature count");

  std::vector<std::string> out_names;
  std::vector<Thread> out_cols;
  out_names.reserve(ordered.size());
  out_cols.reserve(ordered.size());
  std::unordered_set<std::string> seen;
  seen.reserve(ordered.size());
  for (const auto& n : ordered) {
    PLUSS_CHECK(seen.insert(n).second, "DataFrame::conform_to: duplicate feature in target '" + n + "'");
    const auto it = src.name_to_idx_.find(n);
    PLUSS_CHECK(it != src.name_to_idx_.end(), "DataFrame::conform_to: missing feature '" + n + "'");
    out_names.push_back(n);
    out_cols.push_back(src.columns_[it->second]);
  }
  auto out = DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      std::move(out_names), std::move(out_cols), src.cases_));
  return out.align_cases(other.cases());
}

DataFrame DataFrame::reindex_like(const DataFrame& other) const {
  return conform_to(other);
}

DataFrame DataFrame::reindex_cases(const Thread& new_cases) const {
  return align_cases(new_cases);
}

DataFrame DataFrame::stitch_on(std::string_view left_feature,
                               const DataFrame& right,
                               std::string_view right_feature,
                               std::string right_suffix) const {
  const auto left_key = owned_name(left_feature);
  const auto right_key = right_feature.empty() ? left_key : owned_name(right_feature);
  PLUSS_CHECK(!left_key.empty(), "DataFrame::stitch_on: left feature cannot be empty");
  PLUSS_CHECK(!right_key.empty(), "DataFrame::stitch_on: right feature cannot be empty");
  PLUSS_CHECK(has_feature(left_key), "DataFrame::stitch_on: left feature not found '" + left_key + "'");
  PLUSS_CHECK(right.has_feature(right_key), "DataFrame::stitch_on: right feature not found '" + right_key + "'");

  const auto& left_src = impl_ref_();
  const auto& right_src = right.impl_ref_();
  const auto left_col = features(left_key);
  const auto right_col = right.features(right_key);

  std::unordered_map<std::string, std::vector<std::int64_t>> right_lookup;
  right_lookup.reserve(static_cast<std::size_t>(right.nrows()));
  for (std::int64_t r = 0; r < right.nrows(); ++r) {
    right_lookup[thread_scalar_string_at(right_col, r)].push_back(r);
  }

  std::vector<std::int64_t> left_rows;
  std::vector<std::int64_t> right_rows;
  for (std::int64_t l = 0; l < nrows(); ++l) {
    const auto key = thread_scalar_string_at(left_col, l);
    const auto it = right_lookup.find(key);
    if (it == right_lookup.end()) {
      continue;
    }
    for (const auto rr : it->second) {
      left_rows.push_back(l);
      right_rows.push_back(rr);
    }
  }

  std::vector<std::pair<std::string, Thread>> out_cols;
  out_cols.reserve(left_src.names_.size() + right_src.names_.size());

  for (std::size_t i = 0; i < left_src.names_.size(); ++i) {
    out_cols.emplace_back(left_src.names_[i], left_src.columns_[i].take(left_rows));
  }

  std::unordered_set<std::string> used_names;
  used_names.reserve(out_cols.size() + right_src.names_.size());
  for (const auto& [name, _] : out_cols) {
    used_names.insert(name);
  }

  for (std::size_t i = 0; i < right_src.names_.size(); ++i) {
    const auto& base = right_src.names_[i];
    if (base == right_key && left_key == right_key) {
      continue;
    }
    std::string out_name = base;
    if (used_names.find(out_name) != used_names.end()) {
      out_name = base + right_suffix;
      std::size_t k = 2;
      while (used_names.find(out_name) != used_names.end()) {
        out_name = fmt::format("{}{}_{}", base, right_suffix, k++);
      }
    }
    used_names.insert(out_name);
    out_cols.emplace_back(std::move(out_name), right_src.columns_[i].take(right_rows));
  }

  return DataFrame(std::move(out_cols));
}

DataFrame DataFrame::stitch_on(std::initializer_list<std::string_view> left_features,
                               const DataFrame& right,
                               std::initializer_list<std::string_view> right_features,
                               std::string right_suffix) const {
  PLUSS_CHECK(left_features.size() > 0, "DataFrame::stitch_on: at least one left key feature is required");
  std::vector<std::string> left_keys = normalize_name_list(left_features);
  std::vector<std::string> right_keys;
  if (right_features.size() == 0) {
    right_keys = left_keys;
  } else {
    right_keys = normalize_name_list(right_features);
  }
  PLUSS_CHECK(left_keys.size() == right_keys.size(),
              "DataFrame::stitch_on: left/right key feature counts must match");

  const auto& left_src = impl_ref_();
  const auto& right_src = right.impl_ref_();

  std::vector<Thread> left_cols;
  std::vector<Thread> right_cols;
  left_cols.reserve(left_keys.size());
  right_cols.reserve(right_keys.size());
  for (std::size_t i = 0; i < left_keys.size(); ++i) {
    PLUSS_CHECK(has_feature(left_keys[i]),
                "DataFrame::stitch_on: left key feature not found '" + left_keys[i] + "'");
    PLUSS_CHECK(right.has_feature(right_keys[i]),
                "DataFrame::stitch_on: right key feature not found '" + right_keys[i] + "'");
    left_cols.push_back(features(left_keys[i]));
    right_cols.push_back(right.features(right_keys[i]));
  }

  auto make_key = [](const std::vector<Thread>& key_cols, std::int64_t row) {
    std::string k;
    k.reserve(key_cols.size() * 16);
    for (const auto& c : key_cols) {
      k += thread_scalar_string_at(c, row);
      k.push_back('\x1F');
    }
    return k;
  };

  std::unordered_map<std::string, std::vector<std::int64_t>> right_lookup;
  right_lookup.reserve(static_cast<std::size_t>(right.nrows()));
  for (std::int64_t r = 0; r < right.nrows(); ++r) {
    right_lookup[make_key(right_cols, r)].push_back(r);
  }

  std::vector<std::int64_t> left_rows;
  std::vector<std::int64_t> right_rows;
  for (std::int64_t l = 0; l < nrows(); ++l) {
    const auto key = make_key(left_cols, l);
    const auto it = right_lookup.find(key);
    if (it == right_lookup.end()) {
      continue;
    }
    for (const auto rr : it->second) {
      left_rows.push_back(l);
      right_rows.push_back(rr);
    }
  }

  std::vector<std::pair<std::string, Thread>> out_cols;
  out_cols.reserve(left_src.names_.size() + right_src.names_.size());
  for (std::size_t i = 0; i < left_src.names_.size(); ++i) {
    out_cols.emplace_back(left_src.names_[i], left_src.columns_[i].take(left_rows));
  }

  std::unordered_set<std::string> right_key_set(right_keys.begin(), right_keys.end());
  std::unordered_set<std::string> used_names;
  used_names.reserve(out_cols.size() + right_src.names_.size());
  for (const auto& [name, _] : out_cols) {
    used_names.insert(name);
  }

  for (std::size_t i = 0; i < right_src.names_.size(); ++i) {
    const auto& base = right_src.names_[i];
    if (right_key_set.find(base) != right_key_set.end()) {
      continue;
    }
    std::string out_name = base;
    if (used_names.find(out_name) != used_names.end()) {
      out_name = base + right_suffix;
      std::size_t k = 2;
      while (used_names.find(out_name) != used_names.end()) {
        out_name = fmt::format("{}{}_{}", base, right_suffix, k++);
      }
    }
    used_names.insert(out_name);
    out_cols.emplace_back(std::move(out_name), right_src.columns_[i].take(right_rows));
  }

  return DataFrame(std::move(out_cols));
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

DataFrame DataFrame::sample(std::int64_t n, std::optional<std::uint64_t> random_state) const {
  const auto total = nrows();
  PLUSS_CHECK(total >= 0, "DataFrame::sample: invalid case count");
  PLUSS_CHECK(n >= 0, "DataFrame::sample: n must be >= 0");
  const auto draw = std::min<std::int64_t>(n, total);

  std::vector<std::int64_t> idx(static_cast<std::size_t>(total));
  std::iota(idx.begin(), idx.end(), 0);

  std::mt19937_64 rng(random_state.value_or(std::random_device{}()));
  std::shuffle(idx.begin(), idx.end(), rng);
  idx.resize(static_cast<std::size_t>(draw));
  return take(idx);
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
    out = out.with_column(assignment.first, assignment.second);
  }
  return out;
}

DataFrame DataFrame::derive(std::initializer_list<DeriveSpec> specs) const {
  DataFrame out = *this;
  for (const auto& spec : specs) {
    PLUSS_CHECK(!spec.feature.empty(), "DataFrame::derive: feature name cannot be empty");
    PLUSS_CHECK(static_cast<bool>(spec.expr), "DataFrame::derive: derive expression is empty");
    out = out.with_column(spec.feature, spec.expr(out));
  }
  return out;
}

DataFrame DataFrame::assign(std::initializer_list<std::pair<std::string, Thread>> assignments) const {
  return derive(assignments);
}

DataFrame DataFrame::assign(std::initializer_list<DeriveSpec> specs) const {
  return derive(specs);
}

DataFrame DataFrame::summarize_specs(const std::vector<SummarySpec>& specs, bool render) const {
  PLUSS_CHECK(!specs.empty(), "DataFrame::summarize: no SummarySpec supplied");

  std::vector<std::pair<std::string, Thread>> out_cols;
  out_cols.reserve(specs.size());

  std::unordered_set<std::string> names_seen;
  names_seen.reserve(specs.size());

  for (const auto& spec : specs) {
    PLUSS_CHECK(!spec.feature.empty(), "DataFrame::summarize: SummarySpec.feature cannot be empty");
    const auto feature_value = features(spec.feature);
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

DataFrame DataFrame::summarize(std::initializer_list<SummarySpec> specs, bool render) const {
  return summarize_specs(std::vector<SummarySpec>(specs.begin(), specs.end()), render);
}

DataFrame DataFrame::aggregate(std::initializer_list<SummarySpec> specs, bool render) const {
  return summarize(specs, render);
}

DataFrame DataFrame::group(std::initializer_list<std::string_view> keys,
                           std::initializer_list<GroupSpec> specs,
                           bool dropna_keys,
                           bool render) const {
  PLUSS_CHECK(keys.size() != 0, "DataFrame::group: at least one key feature is required");
  PLUSS_CHECK(specs.size() != 0, "DataFrame::group: at least one GroupSpec is required");

  const auto key_names = normalize_name_list(keys);
  std::unordered_set<std::string> key_seen;
  key_seen.reserve(key_names.size());
  for (const auto& key : key_names) {
    PLUSS_CHECK(key_seen.insert(key).second, "DataFrame::group: duplicate key '" + key + "'");
  }

  struct KeyInfo {
    std::string name;
    Thread col;
    Thread value_col;
    std::optional<std::vector<bool>> valid;
  };
  std::vector<KeyInfo> key_infos;
  key_infos.reserve(key_names.size());
  for (const auto& key : key_names) {
    auto col = features(key);
    auto value_col = col.has_validity_mask() ? col.clear_validity_mask() : col;
    key_infos.push_back({
        key,
        col,
        value_col,
        col.has_validity_mask() ? std::optional<std::vector<bool>>(col.validity_mask()->to_vec<bool>()) : std::nullopt,
    });
  }

  struct SpecInfo {
    GroupOp op{GroupOp::Count};
    std::string out_name;
    bool has_feature{false};
    Thread col{};
    Thread value_col{};
    std::optional<std::vector<bool>> valid;
  };

  std::vector<SpecInfo> spec_infos;
  spec_infos.reserve(specs.size());
  std::unordered_set<std::string> out_seen(key_names.begin(), key_names.end());
  out_seen.reserve(key_names.size() + specs.size());

  for (const auto& spec : specs) {
    const auto has_feature = !spec.feature.empty();
    if (spec.op != GroupOp::Count) {
      PLUSS_CHECK(has_feature, "DataFrame::group: feature is required for non-count aggregations");
    }

    Thread col;
    Thread value_col;
    std::optional<std::vector<bool>> valid;
    if (has_feature) {
      col = features(spec.feature);
      value_col = col.has_validity_mask() ? col.clear_validity_mask() : col;
      valid = col.has_validity_mask() ? std::optional<std::vector<bool>>(col.validity_mask()->to_vec<bool>())
                                      : std::nullopt;

      if (spec.op == GroupOp::Sum || spec.op == GroupOp::Mean || spec.op == GroupOp::Min || spec.op == GroupOp::Max) {
        PLUSS_CHECK(col.kind() == ThreadKind::Series && is_number_series_type(col.as_series()),
                    "DataFrame::group: numeric aggregations require numeric Series features");
      }
    }

    auto out_name = spec.output;
    if (out_name.empty()) {
      if (spec.op == GroupOp::Count && !has_feature) {
        out_name = "count";
      } else if (has_feature) {
        out_name = fmt::format("{}_{}", spec.feature, group_op_name(spec.op));
      } else {
        out_name = group_op_name(spec.op);
      }
    }
    PLUSS_CHECK(out_seen.insert(out_name).second, "DataFrame::group: duplicate output feature '" + out_name + "'");

    spec_infos.push_back({
        spec.op,
        std::move(out_name),
        has_feature,
        std::move(col),
        std::move(value_col),
        std::move(valid),
    });
  }

  struct AggState {
    std::int64_t count{0};
    double sum{0.0};
    double min{0.0};
    double max{0.0};
    bool has_numeric{false};
    std::unordered_set<std::string> uniq{};
  };

  struct GroupState {
    std::vector<std::string> keys;
    std::vector<AggState> agg;
  };

  std::vector<GroupState> groups;
  groups.reserve(static_cast<std::size_t>(std::max<std::int64_t>(16, nrows() / 32)));
  std::unordered_map<std::string, std::size_t> index_by_key;
  index_by_key.reserve(groups.capacity());

  for (std::int64_t r = 0; r < nrows(); ++r) {
    std::vector<std::string> key_vals(key_infos.size());
    bool skip = false;
    for (std::size_t k = 0; k < key_infos.size(); ++k) {
      const auto& key_info = key_infos[k];
      const bool valid = !key_info.valid.has_value() || (*key_info.valid)[static_cast<std::size_t>(r)];
      if (!valid && dropna_keys) {
        skip = true;
        break;
      }
      key_vals[k] = valid ? thread_scalar_string_at(key_info.value_col, r) : "NA";
    }
    if (skip) {
      continue;
    }

    std::string composite;
    for (std::size_t k = 0; k < key_vals.size(); ++k) {
      if (k) {
        composite.push_back('\x1f');
      }
      composite += key_vals[k];
    }

    const auto [it, inserted] = index_by_key.emplace(composite, groups.size());
    if (inserted) {
      groups.push_back({std::move(key_vals), std::vector<AggState>(spec_infos.size())});
    }
    auto& g = groups[it->second];

    for (std::size_t s = 0; s < spec_infos.size(); ++s) {
      const auto& spec = spec_infos[s];
      auto& agg = g.agg[s];

      if (spec.op == GroupOp::Count && !spec.has_feature) {
        agg.count += 1;
        continue;
      }

      const bool valid = !spec.valid.has_value() || (*spec.valid)[static_cast<std::size_t>(r)];
      if (!valid) {
        continue;
      }

      if (spec.op == GroupOp::Count) {
        agg.count += 1;
        continue;
      }
      if (spec.op == GroupOp::Nunique) {
        agg.uniq.insert(thread_scalar_string_at(spec.value_col, r));
        continue;
      }

      double x = 0.0;
      if (!extract_numeric_value(spec.col.as_series(), r, x)) {
        continue;
      }
      agg.count += 1;
      agg.sum += x;
      if (!agg.has_numeric) {
        agg.min = x;
        agg.max = x;
        agg.has_numeric = true;
      } else {
        agg.min = std::min(agg.min, x);
        agg.max = std::max(agg.max, x);
      }
    }
  }

  std::vector<std::pair<std::string, Thread>> out_cols;
  out_cols.reserve(key_infos.size() + spec_infos.size());

  for (std::size_t k = 0; k < key_infos.size(); ++k) {
    std::vector<std::string> col;
    col.reserve(groups.size());
    for (const auto& g : groups) {
      col.push_back(g.keys[k]);
    }
    out_cols.emplace_back(key_infos[k].name, make_note_thread(col));
  }

  for (std::size_t s = 0; s < spec_infos.size(); ++s) {
    const auto& spec = spec_infos[s];
    if (spec.op == GroupOp::Count || spec.op == GroupOp::Nunique) {
      std::vector<std::int64_t> col;
      col.reserve(groups.size());
      for (const auto& g : groups) {
        const auto& agg = g.agg[s];
        if (spec.op == GroupOp::Count) {
          col.push_back(agg.count);
        } else {
          col.push_back(static_cast<std::int64_t>(agg.uniq.size()));
        }
      }
      out_cols.emplace_back(spec.out_name, make_i64_thread(col));
      continue;
    }

    std::vector<double> col;
    col.reserve(groups.size());
    for (const auto& g : groups) {
      const auto& agg = g.agg[s];
      if (spec.op == GroupOp::Sum) {
        col.push_back(agg.count > 0 ? agg.sum : std::numeric_limits<double>::quiet_NaN());
      } else if (spec.op == GroupOp::Mean) {
        col.push_back(agg.count > 0 ? (agg.sum / static_cast<double>(agg.count)) : std::numeric_limits<double>::quiet_NaN());
      } else if (spec.op == GroupOp::Min) {
        col.push_back(agg.has_numeric ? agg.min : std::numeric_limits<double>::quiet_NaN());
      } else {
        col.push_back(agg.has_numeric ? agg.max : std::numeric_limits<double>::quiet_NaN());
      }
    }
    out_cols.emplace_back(spec.out_name, make_f64_thread(col));
  }

  auto out = DataFrame(std::move(out_cols));
  if (render) {
    display::print(std::cout, out);
    out.suppress_next_stream_();
  }
  return out;
}

DataFrame DataFrame::group_by(std::initializer_list<std::string_view> keys,
                              std::initializer_list<GroupSpec> specs,
                              bool dropna_keys,
                              bool render) const {
  return group(keys, specs, dropna_keys, render);
}

DataFrame DataFrame::pivot(std::string_view case_feature,
                           std::string_view feature_feature,
                           std::string_view value_feature,
                           bool dropna_keys,
                           bool render) const {
  const auto dup = group(
      {case_feature, feature_feature},
      {GroupSpec{"__pivot_cell_count__", "", GroupOp::Count}},
      dropna_keys,
      false);
  const auto counts = dup.series_value("__pivot_cell_count__").to_vec<std::int64_t>();
  const bool has_duplicate = std::any_of(counts.begin(), counts.end(), [](std::int64_t v) { return v > 1; });
  PLUSS_CHECK(!has_duplicate,
              "DataFrame::pivot: duplicate case/feature pairs found; use pivot_table with an aggregation op");
  return pivot_table(case_feature, feature_feature, value_feature, GroupOp::Max, dropna_keys, render);
}

DataFrame DataFrame::pivot_table(std::string_view case_feature,
                                 std::string_view feature_feature,
                                 std::string_view value_feature,
                                 GroupOp op,
                                 bool dropna_keys,
                                 bool render) const {
  PLUSS_CHECK(op == GroupOp::Count || op == GroupOp::Nunique || op == GroupOp::Sum || op == GroupOp::Mean ||
                  op == GroupOp::Min || op == GroupOp::Max,
              "DataFrame::pivot_table: unsupported aggregation op");

  const auto case_col = features(case_feature);
  const auto feat_col = features(feature_feature);
  const auto value_col = features(value_feature);

  const bool needs_numeric = (op != GroupOp::Count && op != GroupOp::Nunique);
  if (needs_numeric) {
    PLUSS_CHECK(value_col.kind() == ThreadKind::Series && is_number_series_type(value_col.as_series()),
                "DataFrame::pivot_table: Sum/Mean/Min/Max require a numeric Series value feature");
  }

  const auto case_vals = case_col.has_validity_mask() ? case_col.clear_validity_mask() : case_col;
  const auto feat_vals = feat_col.has_validity_mask() ? feat_col.clear_validity_mask() : feat_col;
  const auto value_vals = value_col.has_validity_mask() ? value_col.clear_validity_mask() : value_col;
  const auto* value_series = (value_col.kind() == ThreadKind::Series) ? &value_col.as_series() : nullptr;

  struct PivotAgg {
    std::int64_t count{0};
    double sum{0.0};
    double min{0.0};
    double max{0.0};
    bool has_numeric{false};
    std::unordered_set<std::string> uniq{};
  };

  std::unordered_map<std::string, std::size_t> case_to_row;
  std::unordered_map<std::string, std::size_t> feat_to_col;
  std::vector<std::string> case_labels;
  std::vector<std::string> feat_labels;
  std::vector<std::vector<PivotAgg>> grid;

  case_to_row.reserve(static_cast<std::size_t>(std::max<std::int64_t>(16, nrows() / 64)));
  feat_to_col.reserve(static_cast<std::size_t>(std::max<std::int64_t>(16, nrows() / 64)));

  auto ensure_case = [&](const std::string& key) -> std::size_t {
    const auto [it, inserted] = case_to_row.emplace(key, case_labels.size());
    if (inserted) {
      case_labels.push_back(key);
      grid.emplace_back(feat_labels.size());
    }
    return it->second;
  };

  auto ensure_feature = [&](const std::string& key) -> std::size_t {
    const auto [it, inserted] = feat_to_col.emplace(key, feat_labels.size());
    if (inserted) {
      feat_labels.push_back(key);
      for (auto& row : grid) {
        row.emplace_back();
      }
    }
    return it->second;
  };

  for (std::int64_t r = 0; r < nrows(); ++r) {
    const bool case_valid = case_col.is_valid(r);
    const bool feat_valid = feat_col.is_valid(r);
    if (dropna_keys && (!case_valid || !feat_valid)) {
      continue;
    }

    const auto case_key = case_valid ? thread_scalar_string_at(case_vals, r) : std::string("NA");
    const auto feat_key = feat_valid ? thread_scalar_string_at(feat_vals, r) : std::string("NA");
    const auto row_idx = ensure_case(case_key);
    const auto col_idx = ensure_feature(feat_key);
    auto& agg = grid[row_idx][col_idx];

    if (!value_col.is_valid(r)) {
      continue;
    }

    if (op == GroupOp::Count) {
      agg.count += 1;
      continue;
    }
    if (op == GroupOp::Nunique) {
      agg.uniq.insert(thread_scalar_string_at(value_vals, r));
      continue;
    }

    double x = 0.0;
    if (!extract_numeric_value(*value_series, r, x)) {
      continue;
    }
    agg.count += 1;
    agg.sum += x;
    if (!agg.has_numeric) {
      agg.min = x;
      agg.max = x;
      agg.has_numeric = true;
    } else {
      agg.min = std::min(agg.min, x);
      agg.max = std::max(agg.max, x);
    }
  }

  std::vector<std::pair<std::string, Thread>> out_cols;
  out_cols.reserve(feat_labels.size());
  for (std::size_t c = 0; c < feat_labels.size(); ++c) {
    if (op == GroupOp::Count || op == GroupOp::Nunique) {
      std::vector<std::int64_t> vals;
      vals.reserve(case_labels.size());
      for (std::size_t r = 0; r < case_labels.size(); ++r) {
        const auto& agg = grid[r][c];
        vals.push_back(op == GroupOp::Count ? agg.count : static_cast<std::int64_t>(agg.uniq.size()));
      }
      out_cols.emplace_back(feat_labels[c], make_i64_thread(vals));
      continue;
    }

    std::vector<double> vals(case_labels.size(), std::numeric_limits<double>::quiet_NaN());
    std::vector<bool> valid(case_labels.size(), false);
    for (std::size_t r = 0; r < case_labels.size(); ++r) {
      const auto& agg = grid[r][c];
      if (op == GroupOp::Sum && agg.count > 0) {
        vals[r] = agg.sum;
        valid[r] = true;
      } else if (op == GroupOp::Mean && agg.count > 0) {
        vals[r] = agg.sum / static_cast<double>(agg.count);
        valid[r] = true;
      } else if (op == GroupOp::Min && agg.has_numeric) {
        vals[r] = agg.min;
        valid[r] = true;
      } else if (op == GroupOp::Max && agg.has_numeric) {
        vals[r] = agg.max;
        valid[r] = true;
      }
    }
    out_cols.emplace_back(feat_labels[c], make_f64_thread(vals, valid));
  }

  auto out = DataFrame(std::move(out_cols), make_note_thread(case_labels));
  if (render) {
    display::print(std::cout, out);
    out.suppress_next_stream_();
  }
  return out;
}

DataFrame DataFrame::combine(const DataFrame& other) const {
  return combine(other, CombineSpec{});
}

DataFrame DataFrame::combine(const DataFrame& other, const CombineSpec& spec) const {
  PLUSS_CHECK(nrows() == other.nrows(),
              "DataFrame::combine: case count mismatch between left and right frames");

  const auto& left = impl_ref_();
  const auto& right = other.impl_ref_();

  std::vector<std::string> out_names = left.names_;
  std::vector<Thread> out_cols = left.columns_;

  std::unordered_map<std::string, std::size_t> out_index;
  out_index.reserve(out_names.size());
  for (std::size_t i = 0; i < out_names.size(); ++i) {
    out_index.emplace(out_names[i], i);
  }

  auto next_unique_name = [&](std::string candidate) {
    if (out_index.find(candidate) == out_index.end()) {
      return candidate;
    }
    std::size_t suffix_id = 2;
    while (true) {
      const auto attempt = fmt::format("{}_{}", candidate, suffix_id);
      if (out_index.find(attempt) == out_index.end()) {
        return attempt;
      }
      ++suffix_id;
    }
  };

  for (std::size_t i = 0; i < right.names_.size(); ++i) {
    const auto& right_name = right.names_[i];
    const auto& right_col = right.columns_[i];

    const auto it = out_index.find(right_name);
    if (spec.op == CombineOp::Update) {
      if (it == out_index.end()) {
        continue;
      }
      if (!spec.overwrite) {
        continue;
      }
      out_cols[it->second] = right_col;
      continue;
    }

    if (it == out_index.end()) {
      out_index.emplace(right_name, out_names.size());
      out_names.push_back(right_name);
      out_cols.push_back(right_col);
      continue;
    }

    PLUSS_CHECK(!spec.collision_suffix.empty(),
                "DataFrame::combine(features): collision_suffix must be non-empty when duplicate features exist");
    auto candidate = next_unique_name(right_name + spec.collision_suffix);
    out_index.emplace(candidate, out_names.size());
    out_names.push_back(std::move(candidate));
    out_cols.push_back(right_col);
  }

  std::vector<std::pair<std::string, Thread>> pairs;
  pairs.reserve(out_names.size());
  for (std::size_t i = 0; i < out_names.size(); ++i) {
    pairs.emplace_back(out_names[i], out_cols[i]);
  }
  return DataFrame(std::move(pairs), left.cases_);
}

DataFrame DataFrame::count_by(std::string_view feature_name,
                              std::int64_t top_n,
                              bool dropna,
                              bool render) const {
  auto grouped = group({feature_name}, {GroupSpec{"count", "", GroupOp::Count}}, dropna, false);
  const auto keys_note = grouped.note_value(feature_name);
  const auto counts = grouped.series_value("count").to_vec<std::int64_t>();

  std::vector<std::pair<std::string, std::int64_t>> rows;
  rows.reserve(static_cast<std::size_t>(grouped.nrows()));
  for (std::int64_t i = 0; i < grouped.nrows(); ++i) {
    rows.emplace_back(
        std::string(keys_note.at(i)),
        counts[static_cast<std::size_t>(i)]);
  }
  std::sort(rows.begin(), rows.end(), [](const auto& a, const auto& b) {
    if (a.second != b.second) {
      return a.second > b.second;
    }
    return a.first < b.first;
  });
  if (top_n > 0 && static_cast<std::size_t>(top_n) < rows.size()) {
    rows.resize(static_cast<std::size_t>(top_n));
  }

  std::vector<std::string> keys;
  std::vector<std::int64_t> vals;
  keys.reserve(rows.size());
  vals.reserve(rows.size());
  for (const auto& [k, c] : rows) {
    keys.push_back(k);
    vals.push_back(c);
  }

  auto out = DataFrame({
      {owned_name(feature_name), make_note_thread(keys)},
      {"count", make_i64_thread(vals)},
  });
  if (render) {
    display::print(std::cout, out);
    out.suppress_next_stream_();
  }
  return out;
}

DataFrame DataFrame::mean_by(std::string_view group_feature_name,
                             std::string_view value_feature_name,
                             std::int64_t top_n,
                             std::int64_t min_count,
                             bool dropna_group,
                             bool render) const {
  PLUSS_CHECK(min_count >= 1, "DataFrame::mean_by: min_count must be >= 1");
  const auto mean_name = fmt::format("mean_{}", value_feature_name);
  auto grouped = group(
      {group_feature_name},
      {
          GroupSpec{mean_name, owned_name(value_feature_name), GroupOp::Mean},
          GroupSpec{"count", owned_name(value_feature_name), GroupOp::Count},
      },
      dropna_group,
      false);

  struct MeanRow {
    std::string key;
    double mean{0.0};
    std::int64_t count{0};
  };
  const auto groups_note = grouped.note_value(group_feature_name);
  const auto grouped_means = grouped.series_value(mean_name).to_vec<double>();
  const auto grouped_counts = grouped.series_value("count").to_vec<std::int64_t>();

  std::vector<MeanRow> rows;
  rows.reserve(static_cast<std::size_t>(grouped.nrows()));
  for (std::int64_t i = 0; i < grouped.nrows(); ++i) {
    const auto cnt = grouped_counts[static_cast<std::size_t>(i)];
    if (cnt < min_count) {
      continue;
    }
    rows.push_back({
        std::string(groups_note.at(i)),
        grouped_means[static_cast<std::size_t>(i)],
        cnt,
    });
  }

  std::sort(rows.begin(), rows.end(), [](const MeanRow& a, const MeanRow& b) {
    if (a.mean != b.mean) {
      return a.mean > b.mean;
    }
    return a.key < b.key;
  });
  if (top_n > 0 && static_cast<std::size_t>(top_n) < rows.size()) {
    rows.resize(static_cast<std::size_t>(top_n));
  }

  std::vector<std::string> out_groups;
  std::vector<double> out_means;
  std::vector<std::int64_t> out_counts;
  out_groups.reserve(rows.size());
  out_means.reserve(rows.size());
  out_counts.reserve(rows.size());
  for (const auto& row : rows) {
    out_groups.push_back(row.key);
    out_means.push_back(row.mean);
    out_counts.push_back(row.count);
  }

  auto out = DataFrame({
      {owned_name(group_feature_name), make_note_thread(out_groups)},
      {fmt::format("mean_{}", value_feature_name), make_f64_thread(out_means)},
      {"count", make_i64_thread(out_counts)},
  });
  if (render) {
    display::print(std::cout, out);
    out.suppress_next_stream_();
  }
  return out;
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

DataFrame DataFrame::sum(bool render) const {
  const auto& src = impl_ref_();
  std::vector<SummarySpec> specs;
  specs.reserve(src.names_.size());
  for (std::size_t i = 0; i < src.names_.size(); ++i) {
    const auto& col = src.columns_[i];
    if (col.kind() == ThreadKind::Series && is_numeric_series_type(col.as_series())) {
      specs.push_back(SummarySpec{src.names_[i], src.names_[i], SummaryOp::Sum});
    }
  }
  PLUSS_CHECK(!specs.empty(), "DataFrame::sum: no numeric features");
  return summarize_specs(specs, render);
}

DataFrame DataFrame::mean(bool render) const {
  const auto& src = impl_ref_();
  std::vector<SummarySpec> specs;
  specs.reserve(src.names_.size());
  for (std::size_t i = 0; i < src.names_.size(); ++i) {
    const auto& col = src.columns_[i];
    if (col.kind() == ThreadKind::Series && is_numeric_series_type(col.as_series())) {
      specs.push_back(SummarySpec{src.names_[i], src.names_[i], SummaryOp::Mean});
    }
  }
  PLUSS_CHECK(!specs.empty(), "DataFrame::mean: no numeric features");
  return summarize_specs(specs, render);
}

DataFrame DataFrame::min(bool render) const {
  const auto& src = impl_ref_();
  std::vector<SummarySpec> specs;
  specs.reserve(src.names_.size());
  for (std::size_t i = 0; i < src.names_.size(); ++i) {
    const auto& col = src.columns_[i];
    if (col.kind() == ThreadKind::Series && is_numeric_series_type(col.as_series())) {
      specs.push_back(SummarySpec{src.names_[i], src.names_[i], SummaryOp::Min});
    }
  }
  PLUSS_CHECK(!specs.empty(), "DataFrame::min: no numeric features");
  return summarize_specs(specs, render);
}

DataFrame DataFrame::max(bool render) const {
  const auto& src = impl_ref_();
  std::vector<SummarySpec> specs;
  specs.reserve(src.names_.size());
  for (std::size_t i = 0; i < src.names_.size(); ++i) {
    const auto& col = src.columns_[i];
    if (col.kind() == ThreadKind::Series && is_numeric_series_type(col.as_series())) {
      specs.push_back(SummarySpec{src.names_[i], src.names_[i], SummaryOp::Max});
    }
  }
  PLUSS_CHECK(!specs.empty(), "DataFrame::max: no numeric features");
  return summarize_specs(specs, render);
}

DataFrame DataFrame::std(bool render) const {
  const auto& src = impl_ref_();
  std::vector<SummarySpec> specs;
  specs.reserve(src.names_.size());
  for (std::size_t i = 0; i < src.names_.size(); ++i) {
    const auto& col = src.columns_[i];
    if (col.kind() == ThreadKind::Series && is_numeric_series_type(col.as_series())) {
      specs.push_back(SummarySpec{src.names_[i], src.names_[i], SummaryOp::Std});
    }
  }
  PLUSS_CHECK(!specs.empty(), "DataFrame::std: no numeric features");
  return summarize_specs(specs, render);
}

DataFrame DataFrame::var(bool render) const {
  const auto& src = impl_ref_();
  std::vector<SummarySpec> specs;
  specs.reserve(src.names_.size());
  for (std::size_t i = 0; i < src.names_.size(); ++i) {
    const auto& col = src.columns_[i];
    if (col.kind() == ThreadKind::Series && is_numeric_series_type(col.as_series())) {
      specs.push_back(SummarySpec{src.names_[i], src.names_[i], SummaryOp::Var});
    }
  }
  PLUSS_CHECK(!specs.empty(), "DataFrame::var: no numeric features");
  return summarize_specs(specs, render);
}

DataFrame DataFrame::count(bool render) const {
  const auto& src = impl_ref_();
  std::vector<SummarySpec> specs;
  specs.reserve(src.names_.size());
  for (const auto& name : src.names_) {
    specs.push_back(SummarySpec{name, name, SummaryOp::Count});
  }
  PLUSS_CHECK(!specs.empty(), "DataFrame::count: no features");
  return summarize_specs(specs, render);
}

DataFrame DataFrame::nunique(bool render) const {
  const auto& src = impl_ref_();
  std::vector<SummarySpec> specs;
  specs.reserve(src.names_.size());
  for (const auto& name : src.names_) {
    specs.push_back(SummarySpec{name, name, SummaryOp::Nunique});
  }
  PLUSS_CHECK(!specs.empty(), "DataFrame::nunique: no features");
  return summarize_specs(specs, render);
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

DataFrame DataFrame::isna(std::initializer_list<std::string_view> features) const {
  return missing(MissingOp::IsNull, features);
}

DataFrame DataFrame::isnull(std::initializer_list<std::string_view> features) const {
  return isna(features);
}

DataFrame DataFrame::notna(std::initializer_list<std::string_view> features) const {
  return missing(MissingOp::NotNull, features);
}

DataFrame DataFrame::notnull(std::initializer_list<std::string_view> features) const {
  return notna(features);
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

DataFrame DataFrame::fillna(double value, std::initializer_list<std::string_view> features) const {
  MissingSpec spec;
  spec.op = MissingOp::Fill;
  spec.value = value;
  spec.features = normalize_name_list(features);
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

DataFrame DataFrame::dropna_all(std::initializer_list<std::string_view> features) const {
  return missing(MissingOp::DropAll, features);
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

DataFrame DataFrame::tally(std::string_view feature_name,
                           std::int64_t top_n,
                           bool dropna,
                           bool render) const {
  return count_by(feature_name, top_n, dropna, render);
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

DataFrame DataFrame::copy() const {
  return *this;
}

bool DataFrame::equals(const DataFrame& other) const {
  if (nrows() != other.nrows() || ncols() != other.ncols()) {
    return false;
  }
  if (feature_names() != other.feature_names()) {
    return false;
  }
  if (cases().dtype_name() != other.cases().dtype_name()) {
    return false;
  }
  for (std::int64_t r = 0; r < nrows(); ++r) {
    if (cases().is_valid(r) != other.cases().is_valid(r)) {
      return false;
    }
    if (!cases().is_valid(r)) {
      continue;
    }
    if (thread_scalar_string_at(cases(), r) != thread_scalar_string_at(other.cases(), r)) {
      return false;
    }
  }
  for (const auto& name : feature_names()) {
    const auto lhs = features(name);
    const auto rhs = other.features(name);
    if (lhs.kind() != rhs.kind() || lhs.size() != rhs.size()) {
      return false;
    }
    for (std::int64_t r = 0; r < lhs.size(); ++r) {
      if (lhs.is_valid(r) != rhs.is_valid(r)) {
        return false;
      }
      if (!lhs.is_valid(r)) {
        continue;
      }
      if (thread_scalar_string_at(lhs, r) != thread_scalar_string_at(rhs, r)) {
        return false;
      }
    }
  }
  return true;
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
