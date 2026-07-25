#include <pluss/nlp.h>

#include <filesystem>
#include <iostream>
#include <vector>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

namespace {

using namespace pluss::ai::nodeb;

bool run_nodeb_linguistics_checks(std::ostream* err = nullptr) {
  Workflow nlp{
      .lang = Language::English,
      .stages = {
          stage::tokenize(),
          stage::sentencize(),
          stage::lemmatize(),
          stage::tagger(),
          stage::ner({
              EntityPattern{.label = "GPE", .tokens = {"New", "York"}, .use_lemmas = false},
          }),
      },
  };

  const auto doc = nlp.infer("Dr. Laurel writes APIs for NASA in New York. Email help@pluss.dev.");
  if (doc.tokens.size() < 12) {
    if (err) *err << "nodeb linguistics workflow should produce a non-trivial token stream\n";
    return false;
  }
  if (doc.tokens[1].pos != "PROPN" ||
      doc.tokens[2].pos != "VERB" ||
      doc.tokens[4].pos != "ADP" ||
      doc.tokens[5].pos != "PROPN" ||
      doc.tokens.back().pos != "PUNCT") {
    if (err) {
      *err << "nodeb tagger should assign stable coarse POS labels\n";
      for (const auto& token : doc.tokens) {
        *err << " [" << token.orth << "/" << token.pos << "]";
      }
      *err << "\n";
    }
    return false;
  }

  const std::vector<std::string> want_entity_texts = {
      "Dr. Laurel",
      "NASA",
      "New York",
      "help@pluss.dev",
  };
  const std::vector<std::string> want_entity_labels = {
      "PERSON",
      "ORG",
      "GPE",
      "EMAIL",
  };
  if (doc.entity_texts() != want_entity_texts || doc.entities.size() != want_entity_labels.size()) {
    if (err) {
      *err << "nodeb ner should emit ordered entity spans with expected texts\n";
      for (const auto& entity : doc.entities) {
        *err << " {" << entity.label << ":" << entity.text(doc) << "}";
      }
      *err << "\n";
    }
    return false;
  }
  for (std::size_t i = 0; i < want_entity_labels.size(); ++i) {
    if (doc.entities[i].label != want_entity_labels[i]) {
      if (err) *err << "nodeb ner should preserve entity labels in document order\n";
      return false;
    }
  }
  auto token_entity = [&](std::string_view orth) -> std::string {
    for (const auto& token : doc.tokens) {
      if (token.orth == orth) return token.entity_label;
    }
    return {};
  };
  if (token_entity("NASA") != "ORG" || token_entity("help@pluss.dev") != "EMAIL") {
    if (err) *err << "nodeb ner should annotate entity-bearing tokens with entity labels\n";
    return false;
  }

  const auto tmp = std::filesystem::temp_directory_path() / "nodeb_linguistics_v2.bin";
  nlp.save(tmp.string());
  const auto restored = Workflow::load(tmp.string());
  const auto restored_doc = restored.infer("Dr. Laurel writes APIs for NASA in New York. Email help@pluss.dev.");
  std::filesystem::remove(tmp);

  if (restored_doc.entity_texts() != want_entity_texts ||
      restored_doc.entities.size() != want_entity_labels.size() ||
      restored_doc.tokens[1].pos != "PROPN" ||
      restored_doc.tokens[2].pos != "VERB") {
    if (err) *err << "nodeb tagger/ner save-load path should preserve options and pattern behavior\n";
    return false;
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("nodeb linguistics tranche", "[nodeb][linguistics]") {
  if (!run_nodeb_linguistics_checks(&std::cerr)) {
    FAIL("nodeb linguistics checks failed");
  }
}
#else
int main() {
  if (!run_nodeb_linguistics_checks(&std::cerr)) {
    return 1;
  }
  std::cout << "nodeb linguistics checks passed\n";
  return 0;
}
#endif
