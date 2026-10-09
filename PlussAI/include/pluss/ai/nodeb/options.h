#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace pluss::ai::nodeb {

enum class Language : std::uint8_t {
  Unknown = 0,
  English = 1,
  French = 2,
  Spanish = 3,
  German = 4,
};

enum class TaskBackend : std::uint8_t {
  Classic = 0,
  Sequence = 1,
};

enum class SequenceHead : std::uint8_t {
  Mean = 0,
  BiLstm = 1,
  Transformer = 2,
};

enum class SequencePooling : std::uint8_t {
  Mean = 0,
  Last = 1,
};

enum class QuantizationHint : std::uint8_t {
  None = 0,
  Float16Ready = 1,
  Int8Ready = 2,
};

enum class DistillationHint : std::uint8_t {
  None = 0,
  Teacher = 1,
  Student = 2,
};

struct RandomOptions {
  std::uint64_t seed = 0;
  bool deterministic = true;
};

struct TrainOptions {
  RandomOptions random{};
  bool allow_empty_docs = true;
};

struct AssessOptions {
  bool require_sentences = false;
};

// What the most recent learn() call on a trainable stage did. The counts
// describe the training data and what it produced; `loss_history` holds the
// loss after each gradient step of the text classifier's model. The tagger and
// NER classic backends learn count tables with no iterations, so theirs is
// empty; the classic text classifier reports its linear model's iterations.
struct TrainingReport {
  std::int64_t doc_count = 0;
  std::int64_t entry_count = 0;
  std::vector<double> loss_history{};

  [[nodiscard]] std::int64_t steps() const noexcept {
    return static_cast<std::int64_t>(loss_history.size());
  }
};

struct RuleTokenizerOptions {
  bool keep_punct = true;
  bool use_language_exceptions = true;
};

struct SentencizerOptions {
  bool break_on_punct = true;
  bool break_on_newline = true;
  bool use_language_abbreviations = true;
};

struct LemmatizerOptions {
  bool lowercase = true;
  bool fill_from_orth_when_empty = true;
  bool use_language_lookup = true;
};

struct TextCatOptions {
  bool lowercase = true;
  bool use_lemmas = true;
  bool skip_punct = true;
  std::int64_t max_features = 4096;
  std::int64_t min_document_frequency = 1;
  std::int64_t max_iterations = 400;
  double tolerance = 1e-6;
  // Multi-label classification: each training label string is a '|'-joined
  // set of tags (e.g. "tech|urgent") instead of a single class name, and
  // `apply()` populates `Doc::doc_labels` with every tag scoring above
  // `multi_label_threshold`, independent of `doc_label`/`doc_score`. Only
  // supported with `backend = TaskBackend::Sequence`.
  bool multi_label = false;
  double multi_label_threshold = 0.5;
  TaskBackend backend = TaskBackend::Classic;
  std::int64_t sequence_width = 64;
  double sequence_learning_rate = 5e-2;
  double sequence_dropout = 0.0;
  std::uint64_t sequence_seed = 0;
};

struct MatcherOptions {
  bool lowercase = true;
  bool allow_overlap = true;
};

struct TaggerOptions {
  bool lowercase = true;
  bool use_lemmas = true;
  bool prefer_learned = true;
  bool use_heuristics = true;
  TaskBackend backend = TaskBackend::Classic;
  std::int64_t sequence_width = 48;
  std::uint64_t sequence_seed = 0;
};

struct NerOptions {
  bool lowercase = true;
  bool allow_overlap = false;
  bool use_heuristics = true;
  bool prefer_learned = true;
  TaskBackend backend = TaskBackend::Classic;
  std::int64_t sequence_width = 48;
  std::int64_t sequence_max_span_width = 4;
  double sequence_min_score = 0.55;
  std::uint64_t sequence_seed = 0;
};

struct SyntaxOptions {
  bool prefer_verbs = true;
  bool attach_punct = true;
  bool build_phrases = true;
};

} // namespace pluss::ai::nodeb
