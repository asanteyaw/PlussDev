#pragma once

#include "pluss/ai/nodeb/runtime.h"

namespace pluss::ai::nodeb::detail {

struct SequenceTaskState {
  runtime::Indexer indexer{};
  runtime::EmbeddingTable table{};
  SequenceHead head = SequenceHead::Mean;
  SequencePooling pooling = SequencePooling::Mean;
  std::int64_t width = 0;
  bool ready = false;
};

[[nodiscard]] std::string_view to_string(TaskBackend value) noexcept;
[[nodiscard]] std::string_view to_string(SequenceHead value) noexcept;
[[nodiscard]] std::string_view to_string(SequencePooling value) noexcept;
[[nodiscard]] std::string_view to_string(QuantizationHint value) noexcept;
[[nodiscard]] std::string_view to_string(DistillationHint value) noexcept;
[[nodiscard]] std::string hex_encode_string(std::string_view value);
[[nodiscard]] std::string hex_decode_string(std::string_view value);
[[nodiscard]] std::vector<double> parse_doubles_line(std::string_view value);
[[nodiscard]] std::string join_doubles_line(const std::vector<double>& values);

[[nodiscard]] TaskBackend parse_task_backend(std::string_view value);
[[nodiscard]] SequenceHead parse_sequence_head(std::string_view value);
[[nodiscard]] SequencePooling parse_sequence_pooling(std::string_view value);
[[nodiscard]] QuantizationHint parse_quantization_hint(std::string_view value);
[[nodiscard]] DistillationHint parse_distillation_hint(std::string_view value);

void initialize_sequence_state(SequenceTaskState& state,
                               const std::vector<Doc>& docs,
                               bool use_lemmas,
                               bool lowercase,
                               bool skip_punct,
                               SequenceHead head,
                               SequencePooling pooling,
                               std::int64_t width,
                               std::uint64_t seed);

[[nodiscard]] std::vector<std::vector<std::vector<double>>> encode_docs(
    const SequenceTaskState& state,
    const std::vector<Doc>& docs);

[[nodiscard]] std::vector<double> pool_document_rows(
    const std::vector<std::vector<double>>& rows,
    SequencePooling pooling);

[[nodiscard]] std::vector<double> pool_span_rows(
    const std::vector<std::vector<double>>& rows,
    std::int64_t token_start,
    std::int64_t token_end);

[[nodiscard]] double cosine_similarity(const std::vector<double>& lhs,
                                       const std::vector<double>& rhs) noexcept;

[[nodiscard]] std::vector<std::string> dump_sequence_state_lines(
    const char* prefix,
    const SequenceTaskState& state);

void load_sequence_state_lines(const char* prefix,
                               pluss::msharp::RangeView::Of<std::string_view> lines,
                               SequenceTaskState& state);

} // namespace pluss::ai::nodeb::detail
