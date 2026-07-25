#include "pluss/ai/nodeb/tasks.h"

#include <unordered_set>

namespace pluss::ai::nodeb::assess {

namespace {

[[nodiscard]] std::string span_key(const Span& span) {
  return std::to_string(span.token_start) + "|" +
         std::to_string(span.token_end) + "|" + span.label;
}

} // namespace

TagScore tagger(const std::vector<Doc>& predicted,
                const std::vector<Doc>& gold) {
  PLUSS_CALL_GUARD();
  if (predicted.size() != gold.size()) {
    ::pluss::ai::nodeb::detail::fail(
        "nodeb::assess::tagger: predicted/gold document counts must match.");
  }

  TagScore out{};
  out.doc_count = static_cast<std::int64_t>(predicted.size());
  for (std::size_t i = 0; i < predicted.size(); ++i) {
    const auto& pred = predicted[i];
    const auto& ref = gold[i];
    if (pred.tokens.size() != ref.tokens.size()) {
      ::pluss::ai::nodeb::detail::fail(
          "nodeb::assess::tagger: predicted/gold token counts must match per document.");
    }
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
        "nodeb::assess::ner: predicted/gold document counts must match.");
  }

  EntityScore out{};
  out.doc_count = static_cast<std::int64_t>(predicted.size());
  for (std::size_t i = 0; i < predicted.size(); ++i) {
    const auto& pred = predicted[i];
    const auto& ref = gold[i];
    out.predicted_count += static_cast<std::int64_t>(pred.entities.size());
    out.gold_count += static_cast<std::int64_t>(ref.entities.size());
    std::unordered_set<std::string> gold_spans{};
    gold_spans.reserve(ref.entities.size());
    for (const auto& span : ref.entities) {
      gold_spans.insert(span_key(span));
    }
    for (const auto& span : pred.entities) {
      if (gold_spans.contains(span_key(span))) {
        ++out.correct_count;
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
