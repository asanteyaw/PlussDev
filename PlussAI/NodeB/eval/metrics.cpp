#include "pluss/ai/nodeb/tasks.h"

#include <unordered_set>

namespace pluss::ai::nodeb::assess {

namespace {

[[nodiscard]] std::string span_key(const Span& span) {
  return std::to_string(span.token_start) + "|" +
         std::to_string(span.token_end) + "|" + span.label;
}

void require_token_alignment(const Doc& predicted,
                             const Doc& gold,
                             std::string_view where) {
  if (predicted.tokens.size() != gold.tokens.size()) {
    ::pluss::ai::nodeb::detail::fail(
        std::string(where) + ": predicted/gold token counts must match per document.");
  }
  for (std::size_t i = 0; i < predicted.tokens.size(); ++i) {
    const auto& lhs = predicted.tokens[i];
    const auto& rhs = gold.tokens[i];
    if (lhs.char_start != rhs.char_start || lhs.char_end != rhs.char_end ||
        lhs.orth != rhs.orth) {
      ::pluss::ai::nodeb::detail::fail(
          std::string(where) + ": predicted/gold token sequences must align exactly.");
    }
  }
}

} // namespace

TagScore tagger(const std::vector<Doc>& predicted,
                const std::vector<Doc>& gold) {
  PLUSS_CALL_GUARD();
  if (predicted.size() != gold.size()) {
    ::pluss::ai::nodeb::detail::fail(
        "nlp::assess::tagger: predicted/gold document counts must match.");
  }

  TagScore out{};
  out.doc_count = static_cast<std::int64_t>(predicted.size());
  for (std::size_t i = 0; i < predicted.size(); ++i) {
    const auto& pred = predicted[i];
    const auto& ref = gold[i];
    require_token_alignment(pred, ref, "nlp::assess::tagger");
    for (std::size_t j = 0; j < pred.tokens.size(); ++j) {
      ++out.token_count;
      if (pred.tokens[j].pos == ref.tokens[j].pos) {
        ++out.correct_count;
      }
    }
  }
  if (out.token_count > 0) {
    out.accuracy = static_cast<double>(out.correct_count) /
                   static_cast<double>(out.token_count);
  }
  return out;
}

EntityScore ner(const std::vector<Doc>& predicted,
                const std::vector<Doc>& gold) {
  PLUSS_CALL_GUARD();
  if (predicted.size() != gold.size()) {
    ::pluss::ai::nodeb::detail::fail(
        "nlp::assess::ner: predicted/gold document counts must match.");
  }

  EntityScore out{};
  out.doc_count = static_cast<std::int64_t>(predicted.size());
  for (std::size_t i = 0; i < predicted.size(); ++i) {
    const auto& pred = predicted[i];
    const auto& ref = gold[i];
    require_token_alignment(pred, ref, "nlp::assess::ner");
    out.predicted_count += static_cast<std::int64_t>(pred.entities.size());
    out.gold_count += static_cast<std::int64_t>(ref.entities.size());
    std::unordered_multiset<std::string> gold_spans{};
    gold_spans.reserve(ref.entities.size());
    for (const auto& span : ref.entities) {
      gold_spans.insert(span_key(span));
    }
    for (const auto& span : pred.entities) {
      const auto it = gold_spans.find(span_key(span));
      if (it != gold_spans.end()) {
        ++out.correct_count;
        gold_spans.erase(it);
      }
    }
  }

  if (out.predicted_count > 0) {
    out.precision = static_cast<double>(out.correct_count) /
                    static_cast<double>(out.predicted_count);
  }
  if (out.gold_count > 0) {
    out.recall = static_cast<double>(out.correct_count) /
                 static_cast<double>(out.gold_count);
  }
  if (out.precision + out.recall > 0.0) {
    out.f1 = 2.0 * out.precision * out.recall /
             (out.precision + out.recall);
  }
  return out;
}

} // namespace pluss::ai::nodeb::assess
