#pragma once

#include "pluss/ai/nodeb/options.h"
#include "pluss/msharp/msharp.h"
#include "pluss/pluss.h"

#include <cstdint>
#include <limits>
#include <memory>
#include <source_location>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace pluss::ai::nodeb {

namespace detail {

[[noreturn]] inline void fail(
    const std::string& msg,
    const std::source_location& loc = std::source_location::current()) {
  ::pluss::codem::throw_error(msg, loc);
}

} // namespace detail

namespace core {

struct Doc;
using StringId = std::uint64_t;

enum class CaseKind : std::uint8_t {
  None = 0,
  Lower = 1,
  Upper = 2,
  Title = 3,
  Mixed = 4,
};

enum class TokenKind : std::uint8_t {
  Unknown = 0,
  Word = 1,
  Number = 2,
  Url = 3,
  Email = 4,
  Punct = 5,
  Mixed = 6,
};

struct Morphology {
  CaseKind casing = CaseKind::None;
  TokenKind kind = TokenKind::Unknown;
  std::int32_t length = 0;
  bool is_ascii = true;
  bool has_alpha = false;
  bool has_digit = false;
  bool has_hyphen = false;
  bool has_apostrophe = false;
  bool is_titlecase = false;
  bool is_uppercase = false;
  bool is_lowercase = false;
  bool is_number_like = false;
  bool is_url = false;
  bool is_email = false;
  bool is_punct = false;
  std::string norm{};
  std::string shape{};
  std::string prefix{};
  std::string suffix{};
  std::string spec{};
};

struct Lexeme {
  StringId id = 0;
  StringId lower_id = 0;
  StringId norm_id = 0;
  StringId prefix_id = 0;
  StringId suffix_id = 0;
  StringId morphology_id = 0;
  std::string text{};
  std::string lower{};
  std::string lemma{};
  std::string norm{};
  std::string prefix{};
  std::string suffix{};
  Morphology morphology{};
};

class TextPool {
public:
  [[nodiscard]] StringId intern(std::string_view value);
  [[nodiscard]] StringId lookup(std::string_view value) const;
  [[nodiscard]] bool contains(StringId id) const noexcept;
  [[nodiscard]] const std::string& at(StringId id) const;
  [[nodiscard]] std::size_t size() const noexcept;
  void clear();

private:
  std::vector<std::string> values_{};
  std::unordered_map<std::string, StringId> text_to_id_{};
  std::unordered_map<StringId, std::size_t> id_to_index_{};
};

class MorphBank {
public:
  explicit MorphBank(TextPool* text_pool = nullptr) noexcept
      : text_pool_(text_pool) {}

  void attach(TextPool* text_pool) noexcept { text_pool_ = text_pool; }
  [[nodiscard]] StringId intern(const Morphology& value);
  [[nodiscard]] const Morphology* find(StringId id) const noexcept;
  [[nodiscard]] std::size_t size() const noexcept;
  void clear();

private:
  TextPool* text_pool_ = nullptr;
  std::unordered_map<StringId, Morphology> forms_{};
};

class Lexicon {
public:
  [[nodiscard]] StringId intern(std::string_view value);
  [[nodiscard]] StringId lookup(std::string_view value) const;
  [[nodiscard]] const std::string& text(StringId id) const;
  [[nodiscard]] const Lexeme& observe(std::string_view value);
  [[nodiscard]] const Lexeme* find(StringId id) const noexcept;
  [[nodiscard]] const Lexeme* find(std::string_view value) const noexcept;
  [[nodiscard]] StringId intern_morphology(const Morphology& value);
  [[nodiscard]] const Morphology* find_morphology(StringId id) const noexcept;
  [[nodiscard]] std::size_t size() const noexcept;
  [[nodiscard]] const TextPool& texts() const noexcept;
  [[nodiscard]] const MorphBank& morphs() const noexcept;
  void clear();

private:
  TextPool texts_{};
  MorphBank morphs_{&texts_};
  std::unordered_map<StringId, Lexeme> lexemes_{};
};

struct Token {
  std::int64_t index = 0;
  std::int64_t char_start = 0;
  std::int64_t char_end = 0;
  std::int64_t chief = -1;
  StringId orth_id = 0;
  StringId lower_id = 0;
  StringId norm_id = 0;
  StringId prefix_id = 0;
  StringId suffix_id = 0;
  StringId lemma_id = 0;
  StringId morphology_id = 0;
  std::string orth{};
  std::string lemma{};
  std::string pos{};
  std::string role{};
  std::string entity_label{};
  bool is_punct = false;
  bool sentence_start = false;
  bool sentence_end = false;
  bool space_after = false;
  Morphology morphology{};
};

struct Span {
  std::int64_t token_start = 0;
  std::int64_t token_end = 0;
  std::string label{};

  [[nodiscard]] bool empty() const noexcept { return token_end <= token_start; }
  [[nodiscard]] std::string text(const Doc& doc) const;
};

struct Doc {
  std::string text{};
  Language lang = Language::English;
  std::shared_ptr<Lexicon> lexicon = std::make_shared<Lexicon>();
  std::vector<Token> tokens{};
  std::vector<Span> sentences{};
  std::vector<Span> phrases{};
  std::vector<Span> matches{};
  std::vector<Span> entities{};
  std::string doc_label{};
  double doc_score = std::numeric_limits<double>::quiet_NaN();
  // Populated instead of `doc_label`/`doc_score` when a `TextCatStage`
  // configured with `TextCatOptions.multi_label = true` runs: every label
  // whose predicted probability cleared `multi_label_threshold`, paired
  // with that probability, most-confident first.
  std::vector<std::pair<std::string, double>> doc_labels{};

  [[nodiscard]] Lexicon& ensure_lexicon();
  [[nodiscard]] const Lexicon* maybe_lexicon() const noexcept;
  [[nodiscard]] std::vector<std::string> token_texts() const;
  [[nodiscard]] std::vector<std::string> lemma_texts() const;
  [[nodiscard]] std::vector<std::string> sentence_texts() const;
  [[nodiscard]] std::vector<std::string> phrase_texts() const;
  [[nodiscard]] std::vector<std::string> match_texts() const;
  [[nodiscard]] std::vector<std::string> entity_texts() const;
  [[nodiscard]] std::vector<std::int64_t> children_of(std::int64_t token_index) const;
  [[nodiscard]] std::vector<std::int64_t> lineage_of(std::int64_t token_index) const;
  [[nodiscard]] Span branch_of(std::int64_t token_index) const;
  [[nodiscard]] std::int64_t left_edge_of(std::int64_t token_index) const;
  [[nodiscard]] std::int64_t right_edge_of(std::int64_t token_index) const;
};

} // namespace core

using Doc = core::Doc;
using CaseKind = core::CaseKind;
using Lexeme = core::Lexeme;
using Lexicon = core::Lexicon;
using MorphBank = core::MorphBank;
using Morphology = core::Morphology;
using StringId = core::StringId;
using TextPool = core::TextPool;
using Token = core::Token;
using TokenKind = core::TokenKind;
using Span = core::Span;

class IStage {
public:
  virtual ~IStage() = default;
  [[nodiscard]] virtual std::string_view name() const noexcept = 0;
  virtual void apply(Doc& doc) const = 0;
  virtual void learn(const std::vector<Doc>& docs,
                     const TrainOptions& options) {
    static_cast<void>(docs);
    static_cast<void>(options);
  }
  virtual void learn(const std::vector<Doc>& docs,
                     pluss::msharp::RangeView::Of<std::string_view> labels,
                     const TrainOptions& options) {
    static_cast<void>(docs);
    static_cast<void>(labels);
    static_cast<void>(options);
  }
  [[nodiscard]] virtual std::vector<std::string> dump_state_lines() const { return {}; }
  virtual void load_state_lines(pluss::msharp::RangeView::Of<std::string_view> lines) {
    PLUSS_CALL_GUARD();
    if (!lines.empty()) {
      ::pluss::ai::nodeb::detail::fail(
          "nodeb::IStage::load_state_lines: this stage does not accept persisted state.");
    }
  }
};

using Stage = std::shared_ptr<IStage>;

struct WorkflowReport {
  std::int64_t doc_count = 0;
  std::int64_t token_count = 0;
  std::int64_t sentence_count = 0;
  double mean_tokens_per_doc = 0.0;
  double mean_sentences_per_doc = 0.0;
};

struct Workflow {
  Language lang = Language::English;
  std::vector<Stage> stages{};

  Workflow& append_stage(Stage stage);

  Workflow& learn(pluss::msharp::RangeView::Of<std::string_view> texts,
                  const TrainOptions& options = {});
  Workflow& learn(const std::vector<std::string>& texts,
                  const TrainOptions& options = {});
  Workflow& learn(const pluss::Note& texts,
                  const TrainOptions& options = {});
  Workflow& learn(const std::vector<Doc>& docs,
                  const TrainOptions& options = {});
  Workflow& learn(pluss::msharp::RangeView::Of<std::string_view> texts,
                  pluss::msharp::RangeView::Of<std::string_view> labels,
                  const TrainOptions& options = {});
  Workflow& learn(const std::vector<std::string>& texts,
                  const std::vector<std::string>& labels,
                  const TrainOptions& options = {});
  Workflow& learn(const pluss::Note& texts,
                  const pluss::Note& labels,
                  const TrainOptions& options = {});
  Workflow& learn(const std::vector<Doc>& docs,
                  pluss::msharp::RangeView::Of<std::string_view> labels,
                  const TrainOptions& options = {});

  [[nodiscard]] Doc infer(std::string_view text) const;
  [[nodiscard]] std::vector<Doc> infer(pluss::msharp::RangeView::Of<std::string_view> texts) const;
  [[nodiscard]] std::vector<Doc> infer(const std::vector<std::string>& texts) const;
  [[nodiscard]] std::vector<Doc> infer(const pluss::Note& texts) const;

  [[nodiscard]] WorkflowReport assess(pluss::msharp::RangeView::Of<std::string_view> texts,
                                      const AssessOptions& options = {}) const;
  [[nodiscard]] WorkflowReport assess(const std::vector<std::string>& texts,
                                      const AssessOptions& options = {}) const;
  [[nodiscard]] WorkflowReport assess(const pluss::Note& texts,
                                      const AssessOptions& options = {}) const;
  [[nodiscard]] WorkflowReport assess(const std::vector<Doc>& docs,
                                      const AssessOptions& options = {}) const;

  void save(std::string_view filepath) const;
  [[nodiscard]] static Workflow load(std::string_view filepath);
};

} // namespace pluss::ai::nodeb
