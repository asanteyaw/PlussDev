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
#include "aux/transmit.h"
#include "loader/CsvLoader.h"
#include "loader/Loader.h"
#include "kindle/catalog.h"
#include "table/Atlas.h"
#include "table/display/Display.h"

namespace pluss::table {

namespace detail {

class DataFrameImpl final : public pluss::msharp::RefCounted {
public:
  DataFrameImpl(std::vector<std::string> names,
                std::vector<Thread> columns,
                std::optional<Thread> cases = std::nullopt,
                std::string title = {});
  void validate_and_reindex_();

  std::vector<std::string> names_;
  std::vector<Thread> columns_;
  std::unordered_map<std::string, std::size_t> name_to_idx_;
  std::int64_t nrows_{0};
  Thread cases_{};
  std::string title_{};
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

bool is_none_token(std::string_view s) {
  auto t = lower_copy(owned_name(s));
  std::size_t b = 0;
  std::size_t e = t.size();
  while (b < e && std::isspace(static_cast<unsigned char>(t[b])) != 0) ++b;
  while (e > b && std::isspace(static_cast<unsigned char>(t[e - 1])) != 0) --e;
  t = t.substr(b, e - b);
  return t.empty() || t == "none" || t == "null";
}

Thread infer_padded_thread_from_raw(const pluss::loader::CsvRawColumn& column,
                                    const pluss::loader::ReadCsvOptions& options,
                                    std::int64_t target_len) {
  std::vector<std::string> values = column.values;
  if (target_len >= 0 && static_cast<std::int64_t>(values.size()) < target_len) {
    values.resize(static_cast<std::size_t>(target_len));
  }
  return pluss::loader::infer_thread_from_raw_csv_column(column.name, column.source_index, values, options);
}

DataFrame dataframe_from_raw_batch_padded(const pluss::loader::CsvRawBatch& raw,
                                          const pluss::loader::ReadCsvOptions& options) {
  std::int64_t target_len = 0;
  for (const auto& col : raw.features) {
    target_len = std::max<std::int64_t>(target_len, static_cast<std::int64_t>(col.values.size()));
  }
  if (raw.cases.has_value()) {
    target_len = std::max<std::int64_t>(target_len, static_cast<std::int64_t>(raw.cases->values.size()));
  }

  std::vector<std::pair<std::string, Thread>> cols;
  cols.reserve(raw.features.size());
  for (const auto& col : raw.features) {
    cols.emplace_back(col.name, infer_padded_thread_from_raw(col, options, target_len));
  }

  std::optional<Thread> cases = std::nullopt;
  if (raw.cases.has_value()) {
    cases = infer_padded_thread_from_raw(*raw.cases, options, target_len);
  }

  return DataFrame(std::move(cols), std::move(cases));
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
  PLUSS_CHECK(lhs.ncases() == rhs.ncases(), fmt::format("{}: case count mismatch", where));
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
  if (frame.ncases() <= 1 || input_specs.empty()) {
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

  std::vector<std::int64_t> indices(static_cast<std::size_t>(frame.ncases()));
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
                                     std::optional<Thread> cases,
                                     std::string title)
    : names_(std::move(names)), columns_(std::move(columns)), title_(std::move(title)) {
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
  return pluss::msharp::make_intrusive<detail::DataFrameImpl>(src.names_, src.columns_, src.cases_, src.title_);
}

} // namespace


#include "table/ops/dataframe/DataFrameLifecycleAndIO.inc"
#include "table/ops/dataframe/DataFrameCoreAndFeatures.inc"
#include "table/ops/dataframe/DataFrameSelectionAndTransform.inc"
#include "table/ops/dataframe/DataFrameGroupAndCombine.inc"
#include "table/ops/dataframe/DataFrameMathMissingAndDescribe.inc"

} // namespace pluss::table
