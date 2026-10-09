#include "pluss/ai/nodeb/tasks.h"
#include "pluss/ai/nodeb/resources.h"

#include "CodeN/native/cpu/TextKernels.h"

namespace pluss::ai::nodeb {

LemmatizerStage::LemmatizerStage(LemmatizerOptions options)
    : options_(options) {}

std::string_view LemmatizerStage::name() const noexcept {
  return "lemmatizer";
}

void LemmatizerStage::apply(Doc& doc) const {
  PLUSS_CALL_GUARD();
  auto& lexicon = doc.ensure_lexicon();
  for (auto& token : doc.tokens) {
    if (token.is_punct) {
      token.lemma = options_.fill_from_orth_when_empty ? token.orth : std::string{};
      token.lemma_id = token.lemma.empty() ? 0 : lexicon.intern(token.lemma);
      continue;
    }
    // Language-table lemmas are already canonical, so `lowercase` governs only
    // the rule-based fallback: lowercased when on, the surface form when off.
    if (options_.use_language_lookup) {
      token.lemma = resources::lookup_lemma(doc.lang, token.orth);
    }
    if (token.lemma.empty()) {
      token.lemma = options_.lowercase
                        ? ::pluss::coden::native::cpu::simple_lemma_ascii(token.orth)
                        : token.orth;
    }
    if (options_.fill_from_orth_when_empty && token.lemma.empty()) {
      token.lemma = token.orth;
    }
    token.lemma_id = token.lemma.empty() ? 0 : lexicon.intern(token.lemma);
  }
}

std::vector<std::string> LemmatizerStage::dump_state_lines() const {
  PLUSS_CALL_GUARD();
  return {
      std::string("lowercase=") + (options_.lowercase ? "1" : "0"),
      std::string("fill_from_orth_when_empty=") + (options_.fill_from_orth_when_empty ? "1" : "0"),
      std::string("use_language_lookup=") + (options_.use_language_lookup ? "1" : "0"),
  };
}

void LemmatizerStage::load_state_lines(pluss::msharp::RangeView::Of<std::string_view> lines) {
  PLUSS_CALL_GUARD();
  for (std::size_t i = 0; i < lines.size(); ++i) {
    const std::string_view line = lines[i];
    if (line == "lowercase=1") {
      options_.lowercase = true;
      continue;
    }
    if (line == "lowercase=0") {
      options_.lowercase = false;
      continue;
    }
    if (line == "fill_from_orth_when_empty=1") {
      options_.fill_from_orth_when_empty = true;
      continue;
    }
    if (line == "fill_from_orth_when_empty=0") {
      options_.fill_from_orth_when_empty = false;
      continue;
    }
    if (line == "use_language_lookup=1") {
      options_.use_language_lookup = true;
      continue;
    }
    if (line == "use_language_lookup=0") {
      options_.use_language_lookup = false;
      continue;
    }
    ::pluss::ai::nodeb::detail::fail(
        "nlp::LemmatizerStage::load_state_lines: unrecognized line '" + std::string(line) + "'.");
  }
}

namespace stage {

Stage lemmatize(LemmatizerOptions options) {
  return std::make_shared<LemmatizerStage>(options);
}

} // namespace stage

} // namespace pluss::ai::nodeb
