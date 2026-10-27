#include "sequence_heads.h"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace pluss::ai::nodeb::detail {

namespace {

[[nodiscard]] std::string hex_encode(std::string_view value) {
  static constexpr char digits[] = "0123456789abcdef";
  std::string out;
  out.reserve(value.size() * 2U);
  for (unsigned char ch : value) {
    out.push_back(digits[(ch >> 4) & 0x0f]);
    out.push_back(digits[ch & 0x0f]);
  }
  return out;
}

[[nodiscard]] std::string hex_decode(std::string_view value) {
  if ((value.size() & 1U) != 0U) {
    ::pluss::ai::nodeb::detail::fail(
        "nlp::detail::hex_decode: invalid hex payload length.");
  }
  auto from_hex = [](char ch) -> unsigned char {
    if (ch >= '0' && ch <= '9') return static_cast<unsigned char>(ch - '0');
    if (ch >= 'a' && ch <= 'f') return static_cast<unsigned char>(10 + ch - 'a');
    if (ch >= 'A' && ch <= 'F') return static_cast<unsigned char>(10 + ch - 'A');
    ::pluss::ai::nodeb::detail::fail(
        "nlp::detail::hex_decode: invalid hex payload.");
  };
  std::string out;
  out.reserve(value.size() / 2U);
  for (std::size_t i = 0; i < value.size(); i += 2U) {
    const auto hi = from_hex(value[i]);
    const auto lo = from_hex(value[i + 1U]);
    out.push_back(static_cast<char>((hi << 4) | lo));
  }
  return out;
}

[[nodiscard]] runtime::SequenceOptions sequence_options(bool use_lemmas,
                                                        bool lowercase,
                                                        bool skip_punct) {
  runtime::SequenceOptions options{};
  options.use_lemmas = use_lemmas;
  options.lowercase = lowercase;
  options.skip_punct = skip_punct;
  options.batch_first = true;
  return options;
}

[[nodiscard]] std::vector<double> row_mean(const std::vector<std::vector<double>>& rows) {
  if (rows.empty()) return {};
  std::vector<double> out(rows.front().size(), 0.0);
  for (const auto& row : rows) {
    for (std::size_t i = 0; i < row.size(); ++i) out[i] += row[i];
  }
  const double inv = 1.0 / static_cast<double>(rows.size());
  for (auto& value : out) value *= inv;
  return out;
}

[[nodiscard]] std::vector<double> row_last(const std::vector<std::vector<double>>& rows) {
  if (rows.empty()) return {};
  return rows.back();
}

[[nodiscard]] std::vector<std::vector<double>> contextualize(
    std::vector<std::vector<double>> rows,
    SequenceHead head) {
  if (rows.empty()) return rows;
  if (head == SequenceHead::Mean) return rows;
  if (head == SequenceHead::Transformer) {
    const auto global = row_mean(rows);
    for (auto& row : rows) {
      for (std::size_t i = 0; i < row.size(); ++i) {
        row[i] = 0.5 * row[i] + 0.5 * global[i];
      }
    }
    return rows;
  }

  std::vector<std::vector<double>> prefix(rows.size(), std::vector<double>(rows.front().size(), 0.0));
  std::vector<std::vector<double>> suffix(rows.size(), std::vector<double>(rows.front().size(), 0.0));
  std::vector<double> running(rows.front().size(), 0.0);
  for (std::size_t t = 0; t < rows.size(); ++t) {
    for (std::size_t i = 0; i < running.size(); ++i) running[i] += rows[t][i];
    const double inv = 1.0 / static_cast<double>(t + 1);
    for (std::size_t i = 0; i < running.size(); ++i) prefix[t][i] = running[i] * inv;
  }
  std::fill(running.begin(), running.end(), 0.0);
  for (std::size_t off = 0; off < rows.size(); ++off) {
    const auto t = rows.size() - 1U - off;
    for (std::size_t i = 0; i < running.size(); ++i) running[i] += rows[t][i];
    const double inv = 1.0 / static_cast<double>(off + 1U);
    for (std::size_t i = 0; i < running.size(); ++i) suffix[t][i] = running[i] * inv;
  }
  for (std::size_t t = 0; t < rows.size(); ++t) {
    for (std::size_t i = 0; i < rows[t].size(); ++i) {
      rows[t][i] = 0.5 * prefix[t][i] + 0.5 * suffix[t][i];
    }
  }
  return rows;
}

[[nodiscard]] std::vector<double> pull_series_row(const Series& row) {
  return row.to(pluss::pCPU, pluss::ScalarType::Float64)
      .reshape({row.numel()})
      .pull<double>();
}

} // namespace

std::string hex_encode_string(std::string_view value) {
  return hex_encode(value);
}

std::string hex_decode_string(std::string_view value) {
  return hex_decode(value);
}

std::vector<double> parse_doubles_line(std::string_view value) {
  std::vector<double> out;
  std::istringstream iss{std::string(value)};
  double parsed = 0.0;
  while (iss >> parsed) out.push_back(parsed);
  return out;
}

std::string join_doubles_line(const std::vector<double>& values) {
  std::ostringstream oss;
  oss.precision(17);
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i != 0) oss << ' ';
    oss << values[i];
  }
  return oss.str();
}

std::string_view to_string(TaskBackend value) noexcept {
  switch (value) {
    case TaskBackend::Classic: return "Classic";
    case TaskBackend::Sequence: return "Sequence";
    default: return "Classic";
  }
}

std::string_view to_string(SequenceHead value) noexcept {
  switch (value) {
    case SequenceHead::Mean: return "Mean";
    case SequenceHead::BiLstm: return "BiLstm";
    case SequenceHead::Transformer: return "Transformer";
    default: return "Mean";
  }
}

std::string_view to_string(SequencePooling value) noexcept {
  switch (value) {
    case SequencePooling::Mean: return "Mean";
    case SequencePooling::Last: return "Last";
    default: return "Mean";
  }
}

std::string_view to_string(QuantizationHint value) noexcept {
  switch (value) {
    case QuantizationHint::None: return "None";
    case QuantizationHint::Float16Ready: return "Float16Ready";
    case QuantizationHint::Int8Ready: return "Int8Ready";
    default: return "None";
  }
}

std::string_view to_string(DistillationHint value) noexcept {
  switch (value) {
    case DistillationHint::None: return "None";
    case DistillationHint::Teacher: return "Teacher";
    case DistillationHint::Student: return "Student";
    default: return "None";
  }
}

TaskBackend parse_task_backend(std::string_view value) {
  if (value == "Classic") return TaskBackend::Classic;
  if (value == "Sequence") return TaskBackend::Sequence;
  ::pluss::ai::nodeb::detail::fail(
      "nlp::detail::parse_task_backend: unknown backend '" + std::string(value) + "'.");
}

SequenceHead parse_sequence_head(std::string_view value) {
  if (value == "Mean") return SequenceHead::Mean;
  if (value == "BiLstm") return SequenceHead::BiLstm;
  if (value == "Transformer") return SequenceHead::Transformer;
  ::pluss::ai::nodeb::detail::fail(
      "nlp::detail::parse_sequence_head: unknown head '" + std::string(value) + "'.");
}

SequencePooling parse_sequence_pooling(std::string_view value) {
  if (value == "Mean") return SequencePooling::Mean;
  if (value == "Last") return SequencePooling::Last;
  ::pluss::ai::nodeb::detail::fail(
      "nlp::detail::parse_sequence_pooling: unknown pooling '" + std::string(value) + "'.");
}

QuantizationHint parse_quantization_hint(std::string_view value) {
  if (value == "None") return QuantizationHint::None;
  if (value == "Float16Ready") return QuantizationHint::Float16Ready;
  if (value == "Int8Ready") return QuantizationHint::Int8Ready;
  ::pluss::ai::nodeb::detail::fail(
      "nlp::detail::parse_quantization_hint: unknown hint '" + std::string(value) + "'.");
}

DistillationHint parse_distillation_hint(std::string_view value) {
  if (value == "None") return DistillationHint::None;
  if (value == "Teacher") return DistillationHint::Teacher;
  if (value == "Student") return DistillationHint::Student;
  ::pluss::ai::nodeb::detail::fail(
      "nlp::detail::parse_distillation_hint: unknown hint '" + std::string(value) + "'.");
}

void initialize_sequence_state(SequenceTaskState& state,
                               const std::vector<Doc>& docs,
                               bool use_lemmas,
                               bool lowercase,
                               bool skip_punct,
                               SequenceHead head,
                               SequencePooling pooling,
                               std::int64_t width,
                               std::uint64_t seed) {
  PLUSS_CALL_GUARD();
  state = SequenceTaskState{};
  state.indexer = runtime::Indexer(sequence_options(use_lemmas, lowercase, skip_punct));
  const auto learn = state.indexer.learn(docs);
  if (!learn.ok()) {
    ::pluss::ai::nodeb::detail::fail(
        "nlp::detail::initialize_sequence_state: " + learn.message);
  }
  state.table = runtime::EmbeddingTable(runtime::EmbeddingOptions{
      .width = width,
      .use_positions = true,
      .learned_positions = true,
      .max_positions = 4096,
      .cfg = Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU),
      .seed = seed,
  });
  state.table.reset(static_cast<std::int64_t>(state.indexer.vocabulary().size()),
                    state.indexer.pad_id());
  state.head = head;
  state.pooling = pooling;
  state.width = width;
  state.ready = true;
}

std::vector<std::vector<std::vector<double>>> encode_docs(
    const SequenceTaskState& state,
    const std::vector<Doc>& docs) {
  PLUSS_CALL_GUARD();
  if (!state.ready) {
    ::pluss::ai::nodeb::detail::fail(
        "nlp::detail::encode_docs: sequence state is not ready.");
  }
  const auto batch_or = state.indexer.batch(docs);
  if (!batch_or.ok()) {
    ::pluss::ai::nodeb::detail::fail(
        "nlp::detail::encode_docs: " + batch_or.status().message);
  }
  const auto batch = batch_or.value();
  const auto embedded = state.table.forward(batch).to(pluss::pCPU, pluss::ScalarType::Float64);
  const auto lengths = batch.lengths.to(pluss::pCPU, pluss::ScalarType::Float64)
                           .reshape({batch.batch_size})
                           .host<double>();

  std::vector<std::vector<std::vector<double>>> out;
  out.reserve(static_cast<std::size_t>(batch.batch_size));
  for (std::int64_t b = 0; b < batch.batch_size; ++b) {
    const auto len = static_cast<std::int64_t>(lengths[static_cast<std::size_t>(b)]);
    const auto matrix = batch.batch_first ? embedded.select(0, b).contiguous()
                                          : embedded.select(1, b).contiguous();
    std::vector<std::vector<double>> rows;
    rows.reserve(static_cast<std::size_t>(std::max<std::int64_t>(0, len)));
    for (std::int64_t t = 0; t < len; ++t) {
      rows.push_back(pull_series_row(matrix.select(0, t)));
    }
    out.push_back(contextualize(std::move(rows), state.head));
  }
  return out;
}

std::vector<double> pool_document_rows(const std::vector<std::vector<double>>& rows,
                                       SequencePooling pooling) {
  PLUSS_CALL_GUARD();
  return pooling == SequencePooling::Last ? row_last(rows) : row_mean(rows);
}

std::vector<double> pool_span_rows(const std::vector<std::vector<double>>& rows,
                                   std::int64_t token_start,
                                   std::int64_t token_end) {
  PLUSS_CALL_GUARD();
  if (token_end <= token_start) return {};
  if (token_start < 0 || token_end > static_cast<std::int64_t>(rows.size())) {
    ::pluss::ai::nodeb::detail::fail(
        "nlp::detail::pool_span_rows: span is out of range.");
  }
  std::vector<std::vector<double>> slice;
  slice.reserve(static_cast<std::size_t>(token_end - token_start));
  for (std::int64_t i = token_start; i < token_end; ++i) {
    slice.push_back(rows[static_cast<std::size_t>(i)]);
  }
  return row_mean(slice);
}

double cosine_similarity(const std::vector<double>& lhs,
                         const std::vector<double>& rhs) noexcept {
  if (lhs.size() != rhs.size() || lhs.empty()) return -1.0;
  double dot = 0.0;
  double l2_lhs = 0.0;
  double l2_rhs = 0.0;
  for (std::size_t i = 0; i < lhs.size(); ++i) {
    dot += lhs[i] * rhs[i];
    l2_lhs += lhs[i] * lhs[i];
    l2_rhs += rhs[i] * rhs[i];
  }
  if (l2_lhs <= 0.0 || l2_rhs <= 0.0) return -1.0;
  return dot / std::sqrt(l2_lhs * l2_rhs);
}

std::vector<std::string> dump_sequence_state_lines(const char* prefix,
                                                   const SequenceTaskState& state) {
  PLUSS_CALL_GUARD();
  std::vector<std::string> lines{
      std::string(prefix) + "_ready=" + (state.ready ? "1" : "0"),
      std::string(prefix) + "_head=" + std::string(to_string(state.head)),
      std::string(prefix) + "_pooling=" + std::string(to_string(state.pooling)),
      std::string(prefix) + "_width=" + std::to_string(state.width),
  };
  if (!state.ready) {
    lines.push_back(std::string(prefix) + "_indexer_count=0");
    lines.push_back(std::string(prefix) + "_table_count=0");
    return lines;
  }
  const auto indexer_lines = state.indexer.dump_state_lines();
  const auto table_lines = state.table.dump_state_lines();
  lines.push_back(std::string(prefix) + "_indexer_count=" + std::to_string(indexer_lines.size()));
  for (std::size_t i = 0; i < indexer_lines.size(); ++i) {
    lines.push_back(std::string(prefix) + "_indexer_" + std::to_string(i) + "=" +
                    hex_encode(indexer_lines[i]));
  }
  lines.push_back(std::string(prefix) + "_table_count=" + std::to_string(table_lines.size()));
  for (std::size_t i = 0; i < table_lines.size(); ++i) {
    lines.push_back(std::string(prefix) + "_table_" + std::to_string(i) + "=" +
                    hex_encode(table_lines[i]));
  }
  return lines;
}

void load_sequence_state_lines(const char* prefix,
                               pluss::msharp::RangeView::Of<std::string_view> lines,
                               SequenceTaskState& state) {
  PLUSS_CALL_GUARD();
  state = SequenceTaskState{};
  std::vector<std::string> indexer_lines{};
  std::vector<std::string> table_lines{};
  for (std::size_t i = 0; i < lines.size(); ++i) {
    const auto line = lines[i];
    const auto eq = line.find('=');
    if (eq == std::string_view::npos) continue;
    const std::string key(line.substr(0, eq));
    const std::string_view value = line.substr(eq + 1);
    if (key == std::string(prefix) + "_ready") {
      state.ready = value == "1";
      continue;
    }
    if (key == std::string(prefix) + "_head") {
      state.head = parse_sequence_head(value);
      continue;
    }
    if (key == std::string(prefix) + "_pooling") {
      state.pooling = parse_sequence_pooling(value);
      continue;
    }
    if (key == std::string(prefix) + "_width") {
      state.width = std::stoll(std::string(value));
      continue;
    }
    if (key.rfind(std::string(prefix) + "_indexer_", 0) == 0 &&
        key != std::string(prefix) + "_indexer_count") {
      indexer_lines.push_back(hex_decode(value));
      continue;
    }
    if (key.rfind(std::string(prefix) + "_table_", 0) == 0 &&
        key != std::string(prefix) + "_table_count") {
      table_lines.push_back(hex_decode(value));
      continue;
    }
  }
  if (!state.ready) return;
  std::vector<std::string_view> indexer_views;
  indexer_views.reserve(indexer_lines.size());
  for (const auto& line : indexer_lines) indexer_views.push_back(line);
  const auto indexer_status =
      state.indexer.load_state_lines(pluss::msharp::RangeView::Of<std::string_view>(indexer_views));
  if (!indexer_status.ok()) {
    ::pluss::ai::nodeb::detail::fail(
        "nlp::detail::load_sequence_state_lines[indexer]: " + indexer_status.message);
  }
  std::vector<std::string_view> table_views;
  table_views.reserve(table_lines.size());
  for (const auto& line : table_lines) table_views.push_back(line);
  const auto table_status =
      state.table.load_state_lines(pluss::msharp::RangeView::Of<std::string_view>(table_views));
  if (!table_status.ok()) {
    ::pluss::ai::nodeb::detail::fail(
        "nlp::detail::load_sequence_state_lines[table]: " + table_status.message);
  }
}

} // namespace pluss::ai::nodeb::detail
