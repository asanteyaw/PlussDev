#pragma once

#include "pluss/ai/nodeb/workflow.h"
#include "pluss/ai/nodej/status.h"

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace pluss::ai::nodeb::embed {

struct EmbedOptions {
  bool use_lemmas = true;
  bool lowercase = true;
  bool skip_punct = true;
  std::int64_t width = 100;
  std::int64_t window = 5;
  std::int64_t negative_samples = 5;
  std::int64_t min_count = 1;
  std::int64_t epochs = 5;
  std::int64_t batch_size = 256;
  double learning_rate = 0.01;
  std::uint64_t seed = 0;
};

// A standalone, directly-callable text embedding model.
//
// This is distinct from the sequence-encoder heads used internally by
// `TaggerStage`/`NerStage`/`TextCatStage` (`TaskBackend::Sequence`): those
// pool a randomly-initialized, never-trained `runtime::EmbeddingTable` and
// only ever become meaningful as a byproduct of that task's own supervised
// objective. `Embedder::learn` instead runs genuine unsupervised
// skip-gram-with-negative-sampling training (word2vec-style) over raw,
// unlabeled text via real backpropagation, producing static per-token
// vectors that are meaningful on their own, callable without any labels or
// downstream task at all.
class Embedder {
public:
  explicit Embedder(EmbedOptions options = {});

  // Trains the embedding table from scratch over `docs`. `docs` should
  // already be tokenized (and lemmatized, if `options.use_lemmas` is set)
  // via a `Workflow`, e.g. `Workflow{.stages = {stage::tokenize(),
  // stage::lemmatize()}}.infer(texts)`.
  [[nodiscard]] nodej::Status learn(const std::vector<Doc>& docs);

  [[nodiscard]] bool ready() const noexcept;
  [[nodiscard]] std::int64_t width() const noexcept;
  [[nodiscard]] const std::vector<std::string>& vocabulary() const noexcept;
  [[nodiscard]] bool contains(std::string_view token) const noexcept;

  // The learned static vector for a single vocabulary token.
  [[nodiscard]] nodej::StatusOr<std::vector<double>> vector_of(std::string_view token) const;

  // Mean-pools the learned vectors of a document's in-vocabulary tokens
  // into a single document-level embedding.
  [[nodiscard]] nodej::StatusOr<std::vector<double>> embed(const Doc& doc) const;
  [[nodiscard]] nodej::StatusOr<std::vector<std::vector<double>>> embed(const std::vector<Doc>& docs) const;

  [[nodiscard]] static double similarity(const std::vector<double>& lhs,
                                         const std::vector<double>& rhs) noexcept;

  // The `k` vocabulary tokens whose vectors are most cosine-similar to
  // `token`'s, excluding `token` itself, ranked descending.
  [[nodiscard]] nodej::StatusOr<std::vector<std::pair<std::string, double>>> nearest(
      std::string_view token, std::int64_t k = 10) const;

  void save(std::string_view filepath) const;
  [[nodiscard]] static Embedder load(std::string_view filepath);

private:
  struct Impl;
  std::shared_ptr<Impl> impl_{};
  EmbedOptions options_{};
};

struct ContextualEmbedOptions {
  bool use_lemmas = true;
  bool lowercase = true;
  bool skip_punct = true;
  std::int64_t width = 64;
  std::int64_t num_layers = 2;
  std::int64_t num_heads = 4;
  std::int64_t feedforward_width = 128;
  std::int64_t epochs = 30;
  std::int64_t batch_size = 8;
  double learning_rate = 1e-3;
  std::uint64_t seed = 0;
};

struct GenerateOptions {
  std::int64_t max_new_tokens = 20;
  // Argmax decoding when true. When false, samples from the (temperature-
  // and, if set, top_k-filtered) next-token distribution instead.
  bool greedy = true;
  // Divides logits before sampling; < 1 sharpens the distribution (closer
  // to greedy), > 1 flattens it. Has no effect when `greedy` is true (an
  // argmax is invariant to dividing by a positive constant).
  double temperature = 1.0;
  // Restricts sampling to the `top_k` highest-probability tokens at each
  // step, renormalized. 0 disables the restriction (sample over the full
  // vocabulary). Ignored when `greedy` is true.
  std::int64_t top_k = 0;
  // Only consulted when `greedy` is false.
  std::uint64_t seed = 0;
};

// A contextual (context-sensitive) text encoder: the same token gets a
// different vector depending on the sentence it appears in, unlike
// `Embedder`'s static per-token vectors. `learn` trains a Transformer
// encoder from scratch via genuine causal language-model pretraining
// (next-token prediction, real backpropagation through the embedding
// table, the Transformer stack, and an output projection head) over raw,
// unlabeled text — no labels or downstream task required.
class ContextualEmbedder {
public:
  explicit ContextualEmbedder(ContextualEmbedOptions options = {});

  // See `Embedder::learn` for the expected shape of `docs`.
  [[nodiscard]] nodej::Status learn(const std::vector<Doc>& docs);

  [[nodiscard]] bool ready() const noexcept;
  [[nodiscard]] std::int64_t width() const noexcept;

  // Per-token contextual vectors for `doc`, one row per surviving
  // (non-punct, in-vocabulary) token, in document order.
  [[nodiscard]] nodej::StatusOr<std::vector<std::vector<double>>> encode(const Doc& doc) const;

  // Mean-pools `encode(doc)` into a single document-level vector.
  [[nodiscard]] nodej::StatusOr<std::vector<double>> embed(const Doc& doc) const;

  // Autoregressively continues `prompt` using the causal-LM objective
  // `learn()` trained: repeatedly encodes the sequence so far, projects the
  // last position through the output head, picks a next token (greedy or
  // sampled per `options`), and appends it — stopping at end-of-sequence,
  // `options.max_new_tokens`, or the model's position-embedding capacity,
  // whichever comes first. Returns just the newly generated token strings
  // (not the prompt), in order; `prompt` may be empty (unconditional
  // generation, seeded with only the model's start-of-sequence token).
  [[nodiscard]] nodej::StatusOr<std::vector<std::string>> generate(
      const Doc& prompt, const GenerateOptions& options = {}) const;

  void save(std::string_view filepath) const;
  [[nodiscard]] static ContextualEmbedder load(std::string_view filepath);

private:
  struct Impl;
  std::shared_ptr<Impl> impl_{};
  ContextualEmbedOptions options_{};
};

} // namespace pluss::ai::nodeb::embed
