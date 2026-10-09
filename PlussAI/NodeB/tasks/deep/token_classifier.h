#pragma once

#include "pluss/ai/nodeb/runtime.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace pluss::ai::nodeb::detail {

// A bidirectional Transformer-based per-token classifier, jointly trained
// end to end (embedding table + Transformer encoder + linear head) via
// real backpropagation against per-token gold labels. Shared by
// TaggerStage (direct POS labels) and NerStage (BIO-encoded entity
// labels) so both get the same trainable "last resort" tier for
// TaskBackend::Sequence instead of each keeping their own copy of a
// nearest-centroid classifier over randomly-initialized embeddings.
struct TokenClassifierState {
  runtime::Indexer indexer{};
  std::shared_ptr<runtime::EmbeddingTable> embedding{};
  std::shared_ptr<runtime::TransformerTextEncoder> encoder{};
  Series cls_weight = pluss::series::undefined();
  Series cls_bias = pluss::series::undefined();
  std::vector<std::string> labels{};
  std::int64_t width = 0;
  std::int64_t num_layers = 0;
  std::int64_t num_heads = 0;
  std::int64_t feedforward_width = 0;
  bool ready = false;
  // Loss after each training step of the last train_token_classifier call.
  std::vector<double> loss_history{};
};

struct TokenClassifierOptions {
  bool use_lemmas = true;
  bool lowercase = true;
  std::int64_t width = 48;
  std::int64_t num_layers = 2;
  std::int64_t num_heads = 4;
  std::int64_t feedforward_width = 96;
  std::int64_t epochs = 60;
  double learning_rate = 5e-3;
  std::uint64_t seed = 0;
};

// `token_labels[d][t]` is the gold label for `docs[d].tokens[t]`, or an
// empty string for "no supervision at this position" (excluded from the
// loss). Throws (via `detail::fail`) on invalid input or on non-finite
// loss/gradients/parameters during training, rather than silently handing
// back a broken model.
void train_token_classifier(TokenClassifierState& state,
                            const std::vector<Doc>& docs,
                            const std::vector<std::vector<std::string>>& token_labels,
                            const TokenClassifierOptions& options);

// Per-token (label, probability) predictions for every token in `doc`, in
// order. Empty if `!state.ready`.
[[nodiscard]] std::vector<std::pair<std::string, double>> predict_token_labels(
    const TokenClassifierState& state,
    const Doc& doc);

[[nodiscard]] std::vector<std::string> dump_token_classifier_state_lines(
    const char* prefix,
    const TokenClassifierState& state);

void load_token_classifier_state_lines(const char* prefix,
                                       pluss::msharp::RangeView::Of<std::string_view> lines,
                                       TokenClassifierState& state);

} // namespace pluss::ai::nodeb::detail
