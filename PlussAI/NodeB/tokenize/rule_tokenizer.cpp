#include "pluss/ai/nodeb/tokenize.h"
#include "pluss/ai/nodeb/resources.h"

#include "CodeN/native/cpu/TextKernels.h"

#include <cctype>
#include <limits>

namespace pluss::ai::nodeb {

RuleTokenizerStage::RuleTokenizerStage(RuleTokenizerOptions options)
    : options_(options) {}

std::string_view RuleTokenizerStage::name() const noexcept {
  return "rule_tokenizer";
}

void RuleTokenizerStage::apply(Doc& doc) const {
  PLUSS_CALL_GUARD();
  doc.tokens.clear();
  doc.sentences.clear();
  doc.phrases.clear();
  doc.matches.clear();
  doc.entities.clear();
  doc.doc_label.clear();
  doc.doc_score = std::numeric_limits<double>::quiet_NaN();
  doc.ensure_lexicon().clear();

  const std::string_view text(doc.text);
  const auto spans = ::pluss::coden::native::cpu::split_nlp_token_spans(text, options_.keep_punct);
  doc.tokens.reserve(spans.size());
  auto& lexicon = doc.ensure_lexicon();
  for (const auto& [start, len] : spans) {
    const std::size_t pos = start + len;
    Token token{};
    token.index = static_cast<std::int64_t>(doc.tokens.size());
    token.char_start = static_cast<std::int64_t>(start);
    token.char_end = static_cast<std::int64_t>(pos);
    token.chief = token.index;
    token.orth = std::string(text.substr(start, len));
    const auto& lexeme = lexicon.observe(token.orth);
    token.orth_id = lexeme.id;
    token.lower_id = lexeme.lower_id;
    token.norm_id = lexeme.norm_id;
    token.prefix_id = lexeme.prefix_id;
    token.suffix_id = lexeme.suffix_id;
    token.morphology_id = lexeme.morphology_id;
    token.morphology = lexeme.morphology;
    token.is_punct = lexeme.morphology.is_punct;
    token.space_after = pos < text.size() &&
                        std::isspace(static_cast<unsigned char>(text[pos])) != 0;
    if (options_.use_language_exceptions && !doc.tokens.empty()) {
      std::string merged{};
      if (resources::try_merge_token_exception(doc.lang,
                                               doc.tokens.back().orth,
                                               token.orth,
                                               merged)) {
        auto& prev = doc.tokens.back();
        prev.char_end = token.char_end;
        prev.orth = std::move(merged);
        const auto& merged_lexeme = lexicon.observe(prev.orth);
        prev.orth_id = merged_lexeme.id;
        prev.lower_id = merged_lexeme.lower_id;
        prev.norm_id = merged_lexeme.norm_id;
        prev.prefix_id = merged_lexeme.prefix_id;
        prev.suffix_id = merged_lexeme.suffix_id;
        prev.morphology_id = merged_lexeme.morphology_id;
        prev.morphology = merged_lexeme.morphology;
        prev.is_punct = merged_lexeme.morphology.is_punct;
        prev.space_after = token.space_after;
        continue;
      }
    }
    doc.tokens.push_back(std::move(token));
  }
}

std::vector<std::string> RuleTokenizerStage::dump_state_lines() const {
  PLUSS_CALL_GUARD();
  return {
      std::string("keep_punct=") + (options_.keep_punct ? "1" : "0"),
      std::string("use_language_exceptions=") + (options_.use_language_exceptions ? "1" : "0"),
  };
}

void RuleTokenizerStage::load_state_lines(pluss::msharp::RangeView::Of<std::string_view> lines) {
  PLUSS_CALL_GUARD();
  for (std::size_t i = 0; i < lines.size(); ++i) {
    const std::string_view line = lines[i];
    if (line == "keep_punct=1") {
      options_.keep_punct = true;
      continue;
    }
    if (line == "keep_punct=0") {
      options_.keep_punct = false;
      continue;
    }
    if (line == "use_language_exceptions=1") {
      options_.use_language_exceptions = true;
      continue;
    }
    if (line == "use_language_exceptions=0") {
      options_.use_language_exceptions = false;
      continue;
    }
    ::pluss::ai::nodeb::detail::fail(
        "nlp::RuleTokenizerStage::load_state_lines: unrecognized line '" + std::string(line) + "'.");
  }
}

namespace stage {

Stage tokenize(RuleTokenizerOptions options) {
  return std::make_shared<RuleTokenizerStage>(options);
}

} // namespace stage

} // namespace pluss::ai::nodeb
