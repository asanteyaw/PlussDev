#include <pluss/nlp.h>

#include <iostream>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

namespace {

using namespace pluss::ai::nodeb;

std::string lemma_for(const Doc& doc, std::string_view orth) {
  for (const auto& token : doc.tokens) {
    if (token.orth == orth) return token.lemma;
  }
  return {};
}

bool run_nodeb_multilingual_checks(std::ostream* err = nullptr) {
  const auto langs = resources::available_languages();
  if (langs.size() < 4) {
    if (err) *err << "nodeb resources should expose multiple installed language packs\n";
    return false;
  }

  Workflow french{
      .lang = Language::French,
      .stages = {
          stage::tokenize(),
          stage::sentencize(),
          stage::lemmatize(),
      },
  };
  const auto fr = french.infer("M. Dupont arrive. Nous sommes ici.");
  if (fr.tokens.empty() || fr.tokens.front().orth != "M.") {
    if (err) *err << "nodeb french tokenizer should honor language token-exception merges like M.\n";
    return false;
  }
  if (fr.sentences.size() != 2) {
    if (err) *err << "nodeb french sentencizer should not break after abbreviation titles\n";
    return false;
  }
  if (lemma_for(fr, "sommes") != "etre") {
    if (err) *err << "nodeb french lemmatizer should use language lookup tables\n";
    return false;
  }

  Workflow spanish{
      .lang = Language::Spanish,
      .stages = {
          stage::tokenize(),
          stage::sentencize(),
          stage::lemmatize(),
      },
  };
  const auto es = spanish.infer("Sr. Garcia fue a Madrid. Estamos bien.");
  if (es.tokens.empty() || es.tokens.front().orth != "Sr.") {
    if (err) *err << "nodeb spanish tokenizer should honor language token-exception merges like Sr.\n";
    return false;
  }
  if (es.sentences.size() != 2) {
    if (err) *err << "nodeb spanish sentencizer should keep abbreviation-led sentences intact\n";
    return false;
  }
  if (lemma_for(es, "fue") != "ser") {
    if (err) *err << "nodeb spanish lemmatizer should use language lookup tables\n";
    return false;
  }

  if (resources::language_code(Language::German) != "de" ||
      resources::parse_language_code("fr") != Language::French ||
      resources::language_name(Language::Spanish) != "Spanish") {
    if (err) *err << "nodeb language-pack metadata helpers should round-trip core locale ids\n";
    return false;
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("nodeb multilingual tranche", "[nodeb][multilingual]") {
  if (!run_nodeb_multilingual_checks(&std::cerr)) {
    FAIL("nodeb multilingual checks failed");
  }
}
#else
int main() {
  if (!run_nodeb_multilingual_checks(&std::cerr)) {
    return 1;
  }
  std::cout << "nodeb multilingual checks passed\n";
  return 0;
}
#endif
