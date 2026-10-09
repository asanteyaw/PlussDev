#include "pluss/ai/nodeb/tasks.h"
#include "pluss/ai/nodeb/resources.h"

#include "CodeN/native/cpu/TextKernels.h"

namespace pluss::ai::nodeb {

namespace {

bool has_newline_between(const Doc& doc,
                         const Token& left,
                         const Token& right) {
  if (right.char_start < left.char_end) return false;
  const auto start = static_cast<std::size_t>(left.char_end);
  const auto stop = static_cast<std::size_t>(right.char_start);
  for (std::size_t i = start; i < stop && i < doc.text.size(); ++i) {
    if (doc.text[i] == '\n' || doc.text[i] == '\r') {
      return true;
    }
  }
  return false;
}

void push_sentence_span(Doc& doc,
                        std::int64_t token_start,
                        std::int64_t token_end) {
  if (token_end <= token_start) return;
  doc.tokens[static_cast<std::size_t>(token_start)].sentence_start = true;
  doc.tokens[static_cast<std::size_t>(token_end - 1)].sentence_end = true;
  doc.sentences.push_back(Span{
      .token_start = token_start,
      .token_end = token_end,
      .label = "sentence",
  });
}

bool ends_sentence_here(const Doc& doc,
                        const Token& token,
                        bool use_language_abbreviations) {
  if (token.orth.empty()) return false;
  const char tail = token.orth.back();
  if (tail != '.' && tail != '!' && tail != '?') return false;
  if (tail == '.' && use_language_abbreviations &&
      resources::is_abbreviation(doc.lang, token.orth)) {
    return false;
  }
  return true;
}

void split_span_on_newlines(Doc& doc,
                            std::int64_t token_start,
                            std::int64_t token_end,
                            bool break_on_newline) {
  if (!break_on_newline) {
    push_sentence_span(doc, token_start, token_end);
    return;
  }
  std::int64_t start = token_start;
  for (std::int64_t i = token_start; i + 1 < token_end; ++i) {
    if (has_newline_between(doc,
                            doc.tokens[static_cast<std::size_t>(i)],
                            doc.tokens[static_cast<std::size_t>(i + 1)])) {
      push_sentence_span(doc, start, i + 1);
      start = i + 1;
    }
  }
  push_sentence_span(doc, start, token_end);
}

} // namespace

SentencizerStage::SentencizerStage(SentencizerOptions options)
    : options_(options) {}

std::string_view SentencizerStage::name() const noexcept {
  return "sentencizer";
}

void SentencizerStage::apply(Doc& doc) const {
  PLUSS_CALL_GUARD();
  doc.sentences.clear();
  for (auto& token : doc.tokens) {
    token.sentence_start = false;
    token.sentence_end = false;
  }
  if (doc.tokens.empty()) {
    return;
  }

  if (!options_.break_on_punct) {
    split_span_on_newlines(doc, 0, static_cast<std::int64_t>(doc.tokens.size()), options_.break_on_newline);
    return;
  }
  std::int64_t sentence_start = 0;
  for (std::int64_t i = 0; i < static_cast<std::int64_t>(doc.tokens.size()); ++i) {
    const auto& token = doc.tokens[static_cast<std::size_t>(i)];
    const bool punct_break =
        ends_sentence_here(doc, token, options_.use_language_abbreviations);
    const bool newline_break =
        options_.break_on_newline &&
        i + 1 < static_cast<std::int64_t>(doc.tokens.size()) &&
        has_newline_between(doc,
                            token,
                            doc.tokens[static_cast<std::size_t>(i + 1)]);
    if (!punct_break && !newline_break) continue;
    push_sentence_span(doc, sentence_start, i + 1);
    sentence_start = i + 1;
  }
  push_sentence_span(doc, sentence_start, static_cast<std::int64_t>(doc.tokens.size()));
}

std::vector<std::string> SentencizerStage::dump_state_lines() const {
  PLUSS_CALL_GUARD();
  return {
      std::string("break_on_punct=") + (options_.break_on_punct ? "1" : "0"),
      std::string("break_on_newline=") + (options_.break_on_newline ? "1" : "0"),
      std::string("use_language_abbreviations=") + (options_.use_language_abbreviations ? "1" : "0"),
  };
}

void SentencizerStage::load_state_lines(pluss::msharp::RangeView::Of<std::string_view> lines) {
  PLUSS_CALL_GUARD();
  for (std::size_t i = 0; i < lines.size(); ++i) {
    const std::string_view line = lines[i];
    if (line == "break_on_punct=1") {
      options_.break_on_punct = true;
      continue;
    }
    if (line == "break_on_punct=0") {
      options_.break_on_punct = false;
      continue;
    }
    if (line == "break_on_newline=1") {
      options_.break_on_newline = true;
      continue;
    }
    if (line == "break_on_newline=0") {
      options_.break_on_newline = false;
      continue;
    }
    if (line == "use_language_abbreviations=1") {
      options_.use_language_abbreviations = true;
      continue;
    }
    if (line == "use_language_abbreviations=0") {
      options_.use_language_abbreviations = false;
      continue;
    }
    ::pluss::ai::nodeb::detail::fail(
        "nlp::SentencizerStage::load_state_lines: unrecognized line '" + std::string(line) + "'.");
  }
}

namespace stage {

Stage sentencize(SentencizerOptions options) {
  return std::make_shared<SentencizerStage>(options);
}

} // namespace stage

} // namespace pluss::ai::nodeb
