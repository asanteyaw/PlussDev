#include "pluss/ai/nodeb/runtime.h"

#include "CodeN/native/cpu/TextKernels.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace pluss::ai::nodeb::runtime {

namespace {

using ::pluss::ai::nodej::Status;
using ::pluss::ai::nodej::StatusOr;

[[nodiscard]] Config i64_cpu() {
  return Config{}.with_dtype(ScalarType::Int64).with_platform(pluss::pCPU);
}

[[nodiscard]] Config bool_cpu() {
  return Config{}.with_dtype(ScalarType::Bool).with_platform(pluss::pCPU);
}

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
        "nodeb::runtime::hex_decode: invalid hex payload length.");
  }
  auto from_hex = [](char ch) -> unsigned char {
    if (ch >= '0' && ch <= '9') return static_cast<unsigned char>(ch - '0');
    if (ch >= 'a' && ch <= 'f') return static_cast<unsigned char>(10 + ch - 'a');
    if (ch >= 'A' && ch <= 'F') return static_cast<unsigned char>(10 + ch - 'A');
    ::pluss::ai::nodeb::detail::fail(
        "nodeb::runtime::hex_decode: invalid hex payload.");
  };
  std::string out;
  out.reserve(value.size() / 2U);
  for (std::size_t i = 0; i < value.size(); i += 2U) {
    const unsigned char hi = from_hex(value[i]);
    const unsigned char lo = from_hex(value[i + 1U]);
    out.push_back(static_cast<char>((hi << 4) | lo));
  }
  return out;
}

template <class T>
[[nodiscard]] std::string join_numbers(const std::vector<T>& values) {
  std::ostringstream oss;
  oss.precision(17);
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i != 0U) oss << ' ';
    oss << values[i];
  }
  return oss.str();
}

[[nodiscard]] std::vector<double> parse_doubles(std::string_view value) {
  std::vector<double> out;
  std::istringstream iss{std::string(value)};
  double parsed = 0.0;
  while (iss >> parsed) out.push_back(parsed);
  return out;
}

[[nodiscard]] std::vector<std::int64_t> parse_i64s(std::string_view value) {
  std::vector<std::int64_t> out;
  std::istringstream iss{std::string(value)};
  std::int64_t parsed = 0;
  while (iss >> parsed) out.push_back(parsed);
  return out;
}

[[nodiscard]] std::vector<std::string> doc_terms(const Doc& doc,
                                                 const SequenceOptions& options) {
  std::vector<std::string> out;
  out.reserve(doc.tokens.size() + 2U);
  if (options.add_bos) out.push_back(options.bos_token);
  for (const auto& token : doc.tokens) {
    if (options.skip_punct && token.is_punct) continue;
    std::string value =
        (options.use_lemmas && !token.lemma.empty()) ? token.lemma : token.orth;
    if (value.empty()) continue;
    if (options.lowercase) {
      value = ::pluss::coden::native::cpu::lower_ascii(value);
    }
    out.push_back(std::move(value));
  }
  if (options.add_eos) out.push_back(options.eos_token);
  return out;
}

[[nodiscard]] Status validate_encoded_shape(const Series& encoded,
                                            const SequenceBatch& batch,
                                            std::string_view where) {
  if (!encoded.defined()) {
    return Status::precondition(std::string(where) + ": encoded input must be defined.");
  }
  if (encoded.dim() != 3) {
    return Status::shape_mismatch(std::string(where) + ": encoded input must be rank-3.");
  }
  const auto sizes = encoded.sizes();
  if (batch.batch_first) {
    if (sizes[0] != batch.batch_size || sizes[1] != batch.max_length) {
      return Status::shape_mismatch(
          std::string(where) + ": expected [batch, seq, width] to match SequenceBatch.");
    }
  } else {
    if (sizes[0] != batch.max_length || sizes[1] != batch.batch_size) {
      return Status::shape_mismatch(
          std::string(where) + ": expected [seq, batch, width] to match SequenceBatch.");
    }
  }
  return Status::success();
}

[[nodiscard]] Series sequence_valid_mask(const SequenceBatch& batch,
                                         const Series& like) {
  const Series base = batch.batch_first ? batch.valid_mask
                                        : batch.valid_mask.transpose(0, 1);
  return base.to(like.platform(), like.scalar_type()).unsqueeze(-1);
}

[[nodiscard]] Series last_step_index(const SequenceBatch& batch) {
  const Config cfg = Config{}.with_dtype(ScalarType::Int64).with_platform(pluss::pCPU);
  const Series offsets =
      pluss::series::arange(0, batch.batch_size, cfg).mul(static_cast<double>(batch.max_length));
  const Series last =
      batch.lengths.to(pluss::pCPU, ScalarType::Int64).clamp_min(1).sub(1);
  return offsets.add(last).to(ScalarType::Int64);
}

[[nodiscard]] Series make_sinusoid_positions(std::int64_t rows,
                                             std::int64_t cols,
                                             const Config& cfg) {
  std::vector<double> values(static_cast<std::size_t>(rows * cols), 0.0);
  for (std::int64_t pos = 0; pos < rows; ++pos) {
    for (std::int64_t i = 0; i < cols; ++i) {
      const double denom =
          std::pow(10000.0, static_cast<double>(2 * (i / 2)) / static_cast<double>(std::max<std::int64_t>(1, cols)));
      const double angle = static_cast<double>(pos) / denom;
      values[static_cast<std::size_t>(pos * cols + i)] =
          (i % 2 == 0) ? std::sin(angle) : std::cos(angle);
    }
  }
  // `make_series(vector<T>, ...)` infers its dtype from `T` (here always
  // `double`), ignoring `cfg.dtype` — cast explicitly so the caller's
  // configured dtype (typically Float32) is actually honored.
  return pluss::make_series(std::move(values), {rows, cols}, cfg).to(cfg.platform, cfg.dtype);
}

} // namespace

bool SequenceBatch::empty() const noexcept {
  return batch_size == 0 || max_length == 0 || !token_ids.defined();
}

pluss::nn::PackedSequence SequenceBatch::pack(const Series& values) const {
  PLUSS_CALL_GUARD();
  const Status shape = validate_encoded_shape(values, *this, "nodeb::runtime::SequenceBatch::pack");
  if (!shape.ok()) {
    ::pluss::ai::nodeb::detail::fail(shape.message);
  }
  return pluss::nn::pack_padded_sequence(
      values,
      lengths.to(values.platform(), ScalarType::Int64),
      batch_first);
}

Series SequenceBatch::causal_mask() const {
  PLUSS_CALL_GUARD();
  if (max_length <= 0) {
    ::pluss::ai::nodeb::detail::fail(
        "nodeb::runtime::SequenceBatch::causal_mask: max_length must be > 0.");
  }
  return pluss::nn::generate_square_subsequent_mask(max_length, token_ids);
}

Indexer::Indexer(SequenceOptions options)
    : options_(std::move(options)) {}

Status Indexer::learn(const std::vector<Doc>& docs) {
  PLUSS_CALL_GUARD();
  if (docs.empty()) {
    return Status::invalid_argument(
        "nodeb::runtime::Indexer::learn: expected at least one document.");
  }

  vocabulary_.clear();
  token_to_id_.clear();
  pad_id_ = unk_id_ = bos_id_ = eos_id_ = -1;

  const auto add_token = [this](std::string token) -> std::int64_t {
    const auto found = token_to_id_.find(token);
    if (found != token_to_id_.end()) return found->second;
    const auto id = static_cast<std::int64_t>(vocabulary_.size());
    vocabulary_.push_back(token);
    token_to_id_.emplace(vocabulary_.back(), id);
    return id;
  };

  pad_id_ = add_token(options_.pad_token);
  unk_id_ = add_token(options_.unk_token);
  if (options_.add_bos) bos_id_ = add_token(options_.bos_token);
  if (options_.add_eos) eos_id_ = add_token(options_.eos_token);

  for (const auto& doc : docs) {
    for (auto&& term : doc_terms(doc, options_)) {
      add_token(std::move(term));
    }
  }

  if (vocabulary_.empty()) {
    return Status::internal(
        "nodeb::runtime::Indexer::learn: failed to materialize vocabulary.");
  }
  return Status::success();
}

StatusOr<SequenceBatch> Indexer::batch(const std::vector<Doc>& docs) const {
  PLUSS_CALL_GUARD();
  if (vocabulary_.empty() || token_to_id_.empty()) {
    return StatusOr<SequenceBatch>(Status::precondition(
        "nodeb::runtime::Indexer::batch: call learn(...) before batching."));
  }
  if (docs.empty()) {
    return StatusOr<SequenceBatch>(Status::invalid_argument(
        "nodeb::runtime::Indexer::batch: expected at least one document."));
  }

  std::vector<std::vector<std::int64_t>> sequences;
  sequences.reserve(docs.size());
  std::int64_t max_length = 0;
  for (const auto& doc : docs) {
    std::vector<std::int64_t> ids;
    for (const auto& term : doc_terms(doc, options_)) {
      const auto found = token_to_id_.find(term);
      ids.push_back(found == token_to_id_.end() ? unk_id_ : found->second);
    }
    max_length = std::max(max_length, static_cast<std::int64_t>(ids.size()));
    sequences.push_back(std::move(ids));
  }

  if (max_length <= 0) {
    return StatusOr<SequenceBatch>(Status::precondition(
        "nodeb::runtime::Indexer::batch: all documents became empty after token filtering."));
  }

  const auto batch_size = static_cast<std::int64_t>(docs.size());
  std::vector<std::int64_t> token_ids(
      static_cast<std::size_t>(batch_size * max_length),
      pad_id_);
  std::vector<std::int64_t> position_ids(
      static_cast<std::size_t>(batch_size * max_length),
      0);
  std::vector<bool> valid_mask(
      static_cast<std::size_t>(batch_size * max_length),
      false);
  std::vector<bool> key_padding_mask(
      static_cast<std::size_t>(batch_size * max_length),
      true);
  std::vector<std::int64_t> lengths;
  lengths.reserve(static_cast<std::size_t>(batch_size));

  for (std::int64_t row = 0; row < batch_size; ++row) {
    const auto& seq = sequences[static_cast<std::size_t>(row)];
    lengths.push_back(static_cast<std::int64_t>(seq.size()));
    for (std::int64_t col = 0; col < static_cast<std::int64_t>(seq.size()); ++col) {
      const std::size_t token_offset = static_cast<std::size_t>(
          options_.batch_first ? row * max_length + col
                               : col * batch_size + row);
      token_ids[token_offset] = seq[static_cast<std::size_t>(col)];
      position_ids[token_offset] = col;

      const std::size_t mask_offset = static_cast<std::size_t>(row * max_length + col);
      valid_mask[mask_offset] = true;
      key_padding_mask[mask_offset] = false;
    }
  }

  const std::vector<std::int64_t> token_shape =
      options_.batch_first
          ? std::vector<std::int64_t>{batch_size, max_length}
          : std::vector<std::int64_t>{max_length, batch_size};
  const std::vector<std::int64_t> mask_shape{batch_size, max_length};

  SequenceBatch out{};
  out.token_ids = pluss::make_series(std::move(token_ids), token_shape, i64_cpu());
  out.position_ids = pluss::make_series(std::move(position_ids), token_shape, i64_cpu());
  out.valid_mask = pluss::make_series(std::move(valid_mask), mask_shape, bool_cpu());
  out.key_padding_mask = pluss::make_series(std::move(key_padding_mask), mask_shape, bool_cpu());
  out.lengths = pluss::make_series(std::move(lengths), {batch_size}, i64_cpu());
  out.batch_size = batch_size;
  out.max_length = max_length;
  out.batch_first = options_.batch_first;
  return StatusOr<SequenceBatch>(std::move(out));
}

const SequenceOptions& Indexer::options() const noexcept {
  return options_;
}

const std::vector<std::string>& Indexer::vocabulary() const noexcept {
  return vocabulary_;
}

std::int64_t Indexer::pad_id() const noexcept {
  return pad_id_;
}

std::int64_t Indexer::unk_id() const noexcept {
  return unk_id_;
}

std::int64_t Indexer::bos_id() const noexcept {
  return bos_id_;
}

std::int64_t Indexer::eos_id() const noexcept {
  return eos_id_;
}

std::vector<std::string> Indexer::dump_state_lines() const {
  PLUSS_CALL_GUARD();
  std::vector<std::string> lines;
  lines.reserve(vocabulary_.size() + 16U);
  lines.push_back(std::string("use_lemmas=") + (options_.use_lemmas ? "1" : "0"));
  lines.push_back(std::string("skip_punct=") + (options_.skip_punct ? "1" : "0"));
  lines.push_back(std::string("lowercase=") + (options_.lowercase ? "1" : "0"));
  lines.push_back(std::string("add_bos=") + (options_.add_bos ? "1" : "0"));
  lines.push_back(std::string("add_eos=") + (options_.add_eos ? "1" : "0"));
  lines.push_back(std::string("batch_first=") + (options_.batch_first ? "1" : "0"));
  lines.push_back("pad_token=" + hex_encode(options_.pad_token));
  lines.push_back("unk_token=" + hex_encode(options_.unk_token));
  lines.push_back("bos_token=" + hex_encode(options_.bos_token));
  lines.push_back("eos_token=" + hex_encode(options_.eos_token));
  lines.push_back("pad_id=" + std::to_string(pad_id_));
  lines.push_back("unk_id=" + std::to_string(unk_id_));
  lines.push_back("bos_id=" + std::to_string(bos_id_));
  lines.push_back("eos_id=" + std::to_string(eos_id_));
  lines.push_back("vocab_count=" + std::to_string(vocabulary_.size()));
  for (std::size_t i = 0; i < vocabulary_.size(); ++i) {
    lines.push_back("vocab_" + std::to_string(i) + "=" + hex_encode(vocabulary_[i]));
  }
  return lines;
}

Status Indexer::load_state_lines(pluss::msharp::RangeView::Of<std::string_view> lines) {
  PLUSS_CALL_GUARD();
  if (lines.empty()) {
    return Status::invalid_argument(
        "nodeb::runtime::Indexer::load_state_lines: expected non-empty state.");
  }

  std::int64_t vocab_count = -1;
  std::vector<std::string> vocab{};
  SequenceOptions options{};
  std::int64_t pad_id = -1;
  std::int64_t unk_id = -1;
  std::int64_t bos_id = -1;
  std::int64_t eos_id = -1;

  for (const auto& raw_line : lines) {
    const auto eq = raw_line.find('=');
    if (eq == std::string_view::npos) continue;
    const std::string key(raw_line.substr(0, eq));
    const std::string_view value = raw_line.substr(eq + 1);

    if (key == "use_lemmas") options.use_lemmas = (value == "1");
    else if (key == "skip_punct") options.skip_punct = (value == "1");
    else if (key == "lowercase") options.lowercase = (value == "1");
    else if (key == "add_bos") options.add_bos = (value == "1");
    else if (key == "add_eos") options.add_eos = (value == "1");
    else if (key == "batch_first") options.batch_first = (value == "1");
    else if (key == "pad_token") options.pad_token = hex_decode(value);
    else if (key == "unk_token") options.unk_token = hex_decode(value);
    else if (key == "bos_token") options.bos_token = hex_decode(value);
    else if (key == "eos_token") options.eos_token = hex_decode(value);
    else if (key == "pad_id") pad_id = std::stoll(std::string(value));
    else if (key == "unk_id") unk_id = std::stoll(std::string(value));
    else if (key == "bos_id") bos_id = std::stoll(std::string(value));
    else if (key == "eos_id") eos_id = std::stoll(std::string(value));
    else if (key == "vocab_count") {
      vocab_count = std::stoll(std::string(value));
      vocab.assign(static_cast<std::size_t>(std::max<std::int64_t>(0, vocab_count)), {});
    } else if (key.rfind("vocab_", 0) == 0) {
      if (vocab_count < 0) {
        return Status::precondition(
            "nodeb::runtime::Indexer::load_state_lines: vocab_count must appear before vocab entries.");
      }
      const auto idx = static_cast<std::size_t>(std::stoll(key.substr(6)));
      if (idx >= vocab.size()) {
        return Status::shape_mismatch(
            "nodeb::runtime::Indexer::load_state_lines: vocab entry out of range.");
      }
      vocab[idx] = hex_decode(value);
    }
  }

  if (vocab_count < 0 || static_cast<std::size_t>(vocab_count) != vocab.size()) {
    return Status::shape_mismatch(
        "nodeb::runtime::Indexer::load_state_lines: missing or malformed vocabulary.");
  }
  if (vocab.empty()) {
    return Status::shape_mismatch(
        "nodeb::runtime::Indexer::load_state_lines: vocabulary must not be empty.");
  }

  options_ = std::move(options);
  vocabulary_ = std::move(vocab);
  token_to_id_.clear();
  for (std::size_t i = 0; i < vocabulary_.size(); ++i) {
    token_to_id_.emplace(vocabulary_[i], static_cast<std::int64_t>(i));
  }
  pad_id_ = pad_id;
  unk_id_ = unk_id;
  bos_id_ = bos_id;
  eos_id_ = eos_id;
  return Status::success();
}

EmbeddingTable::EmbeddingTable(EmbeddingOptions options)
    : options_(std::move(options)) {}

void EmbeddingTable::reset(std::int64_t vocab_size,
                           std::int64_t pad_id) {
  PLUSS_CALL_GUARD();
  if (vocab_size <= 0) {
    ::pluss::ai::nodeb::detail::fail(
        "nodeb::runtime::EmbeddingTable::reset: vocab_size must be > 0.");
  }
  if (options_.width <= 0) {
    ::pluss::ai::nodeb::detail::fail(
        "nodeb::runtime::EmbeddingTable::reset: width must be > 0.");
  }
  if (options_.use_positions && options_.max_positions <= 0) {
    ::pluss::ai::nodeb::detail::fail(
        "nodeb::runtime::EmbeddingTable::reset: max_positions must be > 0 when positions are enabled.");
  }

  const double scale = 1.0 / std::sqrt(static_cast<double>(options_.width));
  weights_ = pluss::series::rand(
      {vocab_size, options_.width},
      options_.cfg,
      options_.seed).sub(0.5).mul(2.0 * scale);

  if (pad_id >= 0 && pad_id < vocab_size) {
    std::vector<double> row_scale(static_cast<std::size_t>(vocab_size), 1.0);
    row_scale[static_cast<std::size_t>(pad_id)] = 0.0;
    // See make_sinusoid_positions: make_series ignores cfg.dtype, so cast
    // explicitly to avoid silently promoting weights_ to Float64.
    weights_ = weights_.mul(
        pluss::make_series(std::move(row_scale), {vocab_size, 1}, options_.cfg)
            .to(options_.cfg.platform, options_.cfg.dtype));
  }

  if (!options_.use_positions) {
    position_weights_ = Series();
  } else if (options_.learned_positions) {
    position_weights_ = pluss::series::rand(
        {options_.max_positions, options_.width},
        options_.cfg,
        options_.seed + 97ULL).sub(0.5).mul(2.0 * scale);
  } else {
    position_weights_ = make_sinusoid_positions(
        options_.max_positions,
        options_.width,
        options_.cfg);
  }

  vocab_size_ = vocab_size;
  pad_id_ = pad_id;
}

Series EmbeddingTable::forward(const SequenceBatch& batch) const {
  PLUSS_CALL_GUARD();
  if (!weights_.defined() || vocab_size_ <= 0) {
    ::pluss::ai::nodeb::detail::fail(
        "nodeb::runtime::EmbeddingTable::forward: call reset(...) or load_state_lines(...) first.");
  }
  if (batch.empty()) {
    ::pluss::ai::nodeb::detail::fail(
        "nodeb::runtime::EmbeddingTable::forward: sequence batch must not be empty.");
  }
  if (options_.use_positions && batch.max_length > options_.max_positions) {
    ::pluss::ai::nodeb::detail::fail(
        "nodeb::runtime::EmbeddingTable::forward: batch sequence length exceeds configured max_positions.");
  }

  const Series flat_ids =
      batch.token_ids.to(weights_.platform(), ScalarType::Int64).reshape({batch.token_ids.numel()});
  const std::vector<std::int64_t> shape =
      batch.batch_first
          ? std::vector<std::int64_t>{batch.batch_size, batch.max_length, options_.width}
          : std::vector<std::int64_t>{batch.max_length, batch.batch_size, options_.width};
  Series out = weights_.index_select(0, flat_ids).reshape(shape);

  if (options_.use_positions && position_weights_.defined()) {
    const Series flat_pos =
        batch.position_ids.to(position_weights_.platform(), ScalarType::Int64).reshape({batch.position_ids.numel()});
    Series pos = position_weights_.index_select(0, flat_pos).reshape(shape);
    out = out.add(pos.to(out.platform(), out.scalar_type()));
  }

  out = out.mul(sequence_valid_mask(batch, out));
  return out;
}

const EmbeddingOptions& EmbeddingTable::options() const noexcept {
  return options_;
}

const Series& EmbeddingTable::weights() const noexcept {
  return weights_;
}

const Series& EmbeddingTable::position_weights() const noexcept {
  return position_weights_;
}

std::int64_t EmbeddingTable::vocab_size() const noexcept {
  return vocab_size_;
}

std::int64_t EmbeddingTable::pad_id() const noexcept {
  return pad_id_;
}

std::vector<std::string> EmbeddingTable::dump_state_lines() const {
  PLUSS_CALL_GUARD();
  std::vector<std::string> lines;
  lines.reserve(16U);
  lines.push_back("width=" + std::to_string(options_.width));
  lines.push_back(std::string("use_positions=") + (options_.use_positions ? "1" : "0"));
  lines.push_back(std::string("learned_positions=") + (options_.learned_positions ? "1" : "0"));
  lines.push_back("max_positions=" + std::to_string(options_.max_positions));
  lines.push_back("seed=" + std::to_string(options_.seed));
  lines.push_back("vocab_size=" + std::to_string(vocab_size_));
  lines.push_back("pad_id=" + std::to_string(pad_id_));

  const Series weights_cpu = weights_.defined()
      ? weights_.to(pluss::pCPU, ScalarType::Float64).reshape({weights_.numel()})
      : Series();
  const auto weight_shape = weights_.defined() ? weights_.sizes() : std::vector<std::int64_t>{0, 0};
  lines.push_back("weight_rows=" + std::to_string(weight_shape.empty() ? 0 : weight_shape[0]));
  lines.push_back("weight_cols=" + std::to_string(weight_shape.size() < 2 ? 0 : weight_shape[1]));
  lines.push_back("weights=" + (weights_cpu.defined() ? join_numbers(weights_cpu.to_vec<double>()) : std::string{}));

  const Series pos_cpu = position_weights_.defined()
      ? position_weights_.to(pluss::pCPU, ScalarType::Float64).reshape({position_weights_.numel()})
      : Series();
  const auto pos_shape = position_weights_.defined() ? position_weights_.sizes() : std::vector<std::int64_t>{0, 0};
  lines.push_back("pos_rows=" + std::to_string(pos_shape.empty() ? 0 : pos_shape[0]));
  lines.push_back("pos_cols=" + std::to_string(pos_shape.size() < 2 ? 0 : pos_shape[1]));
  lines.push_back("positions=" + (pos_cpu.defined() ? join_numbers(pos_cpu.to_vec<double>()) : std::string{}));
  return lines;
}

Status EmbeddingTable::load_state_lines(pluss::msharp::RangeView::Of<std::string_view> lines) {
  PLUSS_CALL_GUARD();
  if (lines.empty()) {
    return Status::invalid_argument(
        "nodeb::runtime::EmbeddingTable::load_state_lines: expected non-empty state.");
  }

  std::int64_t weight_rows = 0;
  std::int64_t weight_cols = 0;
  std::int64_t pos_rows = 0;
  std::int64_t pos_cols = 0;
  std::vector<double> weight_values{};
  std::vector<double> pos_values{};
  EmbeddingOptions options = options_;
  std::int64_t vocab_size = 0;
  std::int64_t pad_id = -1;

  for (const auto& raw_line : lines) {
    const auto eq = raw_line.find('=');
    if (eq == std::string_view::npos) continue;
    const std::string key(raw_line.substr(0, eq));
    const std::string_view value = raw_line.substr(eq + 1);

    if (key == "width") options.width = std::stoll(std::string(value));
    else if (key == "use_positions") options.use_positions = (value == "1");
    else if (key == "learned_positions") options.learned_positions = (value == "1");
    else if (key == "max_positions") options.max_positions = std::stoll(std::string(value));
    else if (key == "seed") options.seed = static_cast<std::uint64_t>(std::stoull(std::string(value)));
    else if (key == "vocab_size") vocab_size = std::stoll(std::string(value));
    else if (key == "pad_id") pad_id = std::stoll(std::string(value));
    else if (key == "weight_rows") weight_rows = std::stoll(std::string(value));
    else if (key == "weight_cols") weight_cols = std::stoll(std::string(value));
    else if (key == "weights") weight_values = parse_doubles(value);
    else if (key == "pos_rows") pos_rows = std::stoll(std::string(value));
    else if (key == "pos_cols") pos_cols = std::stoll(std::string(value));
    else if (key == "positions") pos_values = parse_doubles(value);
  }

  if (weight_rows < 0 || weight_cols < 0 ||
      static_cast<std::int64_t>(weight_values.size()) != weight_rows * weight_cols) {
    return Status::shape_mismatch(
        "nodeb::runtime::EmbeddingTable::load_state_lines: malformed token embedding matrix.");
  }
  if (options.use_positions) {
    if (pos_rows < 0 || pos_cols < 0 ||
        static_cast<std::int64_t>(pos_values.size()) != pos_rows * pos_cols) {
      return Status::shape_mismatch(
          "nodeb::runtime::EmbeddingTable::load_state_lines: malformed position embedding matrix.");
    }
  }

  options_ = options;
  vocab_size_ = vocab_size;
  pad_id_ = pad_id;
  // make_series infers dtype from the vector element type (always `double`
  // here via parse_doubles), ignoring options_.cfg.dtype — cast explicitly.
  weights_ = pluss::make_series(
                 std::move(weight_values),
                 {weight_rows, weight_cols},
                 options_.cfg)
                 .to(options_.cfg.platform, options_.cfg.dtype);
  if (options_.use_positions) {
    position_weights_ = pluss::make_series(
                             std::move(pos_values),
                             {pos_rows, pos_cols},
                             options_.cfg)
                             .to(options_.cfg.platform, options_.cfg.dtype);
  } else {
    position_weights_ = Series();
  }
  return Status::success();
}

TransformerTextEncoder::TransformerTextEncoder(pluss::nn::TransformerEncoderOptions options)
    : encoder_(std::move(options)) {}

Series TransformerTextEncoder::forward(const Series& embedded,
                                       const SequenceBatch& batch,
                                       bool causal) const {
  PLUSS_CALL_GUARD();
  const Status shape = validate_encoded_shape(
      embedded,
      batch,
      "nodeb::runtime::TransformerTextEncoder::forward");
  if (!shape.ok()) {
    ::pluss::ai::nodeb::detail::fail(shape.message);
  }
  if (encoder_->options().batch_first != batch.batch_first) {
    ::pluss::ai::nodeb::detail::fail(
        "nodeb::runtime::TransformerTextEncoder::forward: batch_first must match between SequenceBatch and TransformerEncoderOptions.");
  }
  if (embedded.sizes().back() != encoder_->options().d_model) {
    ::pluss::ai::nodeb::detail::fail(
        "nodeb::runtime::TransformerTextEncoder::forward: embedding width must equal d_model.");
  }

  const Series mask = causal ? batch.causal_mask().to(embedded.platform(), ScalarType::Bool)
                             : Series();
  return encoder_->forward(
      embedded,
      mask,
      batch.key_padding_mask.to(embedded.platform(), ScalarType::Bool),
      causal);
}

Series TransformerTextEncoder::forward(const EmbeddingTable& table,
                                       const SequenceBatch& batch,
                                       bool causal) const {
  PLUSS_CALL_GUARD();
  return forward(table.forward(batch), batch, causal);
}

pluss::nn::TransformerEncoder& TransformerTextEncoder::model() noexcept {
  return encoder_;
}

const pluss::nn::TransformerEncoder& TransformerTextEncoder::model() const noexcept {
  return encoder_;
}

LstmTextEncoder::LstmTextEncoder(pluss::nn::LSTMOptions options)
    : lstm_(std::move(options)) {}

Series LstmTextEncoder::forward(const Series& embedded,
                                const SequenceBatch& batch) const {
  PLUSS_CALL_GUARD();
  const Status shape =
      validate_encoded_shape(embedded, batch, "nodeb::runtime::LstmTextEncoder::forward");
  if (!shape.ok()) {
    ::pluss::ai::nodeb::detail::fail(shape.message);
  }
  if (lstm_->options().batch_first != batch.batch_first) {
    ::pluss::ai::nodeb::detail::fail(
        "nodeb::runtime::LstmTextEncoder::forward: batch_first must match between SequenceBatch and LSTMOptions.");
  }
  if (embedded.sizes().back() != lstm_->options().input_size) {
    ::pluss::ai::nodeb::detail::fail(
        "nodeb::runtime::LstmTextEncoder::forward: embedding width must equal input_size.");
  }

  const auto packed = batch.pack(embedded);
  auto [packed_out, _state] = lstm_->forward(packed, {});
  auto [padded, _lengths] = pluss::nn::pad_packed_sequence(
      packed_out,
      batch.batch_first);
  return padded;
}

Series LstmTextEncoder::forward(const EmbeddingTable& table,
                                const SequenceBatch& batch) const {
  PLUSS_CALL_GUARD();
  return forward(table.forward(batch), batch);
}

pluss::nn::LSTM& LstmTextEncoder::model() noexcept {
  return lstm_;
}

const pluss::nn::LSTM& LstmTextEncoder::model() const noexcept {
  return lstm_;
}

Series mean_pool(const Series& encoded,
                 const SequenceBatch& batch) {
  PLUSS_CALL_GUARD();
  const Status shape =
      validate_encoded_shape(encoded, batch, "nodeb::runtime::mean_pool");
  if (!shape.ok()) {
    ::pluss::ai::nodeb::detail::fail(shape.message);
  }

  const Series mask = sequence_valid_mask(batch, encoded);
  const std::int64_t seq_dim = batch.batch_first ? 1 : 0;
  const Series numer = encoded.mul(mask).sum(seq_dim, false);
  const Series denom = batch.lengths
                           .to(numer.platform(), numer.scalar_type())
                           .reshape({batch.batch_size, 1})
                           .clamp_min(1.0);
  return numer.div(denom);
}

Series last_pool(const Series& encoded,
                 const SequenceBatch& batch) {
  PLUSS_CALL_GUARD();
  const Status shape =
      validate_encoded_shape(encoded, batch, "nodeb::runtime::last_pool");
  if (!shape.ok()) {
    ::pluss::ai::nodeb::detail::fail(shape.message);
  }

  Series batch_first = batch.batch_first ? encoded : encoded.transpose(0, 1);
  const auto sizes = batch_first.sizes();
  const std::int64_t width = sizes[2];
  const Series flat = batch_first.reshape({batch.batch_size * batch.max_length, width});
  const Series idx = last_step_index(batch).to(flat.platform(), ScalarType::Int64);
  return flat.index_select(0, idx);
}

} // namespace pluss::ai::nodeb::runtime
