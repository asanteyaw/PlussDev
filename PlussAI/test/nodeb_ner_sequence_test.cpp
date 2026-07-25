#include <pluss/nlp.h>

#include <filesystem>
#include <iostream>
#include <vector>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

namespace {

using namespace pluss::ai::nodeb;

[[nodiscard]] bool has_entity(const Doc& doc, std::string_view label, std::string_view text) {
  for (const auto& entity : doc.entities) {
    if (entity.label == label && entity.text(doc) == text) return true;
  }
  return false;
}

// A small BIO-learnable corpus: "<person> works at <org>", where <org> is
// always the entity to recognize. Heuristics/lexical-memory/patterns are
// disabled so every prediction is forced through the BIO-decoded Sequence
// tier.
bool run_ner_sequence_checks(std::ostream* err = nullptr) {
  Workflow tokenizer{.lang = Language::English, .stages = {stage::tokenize()}};
  std::vector<std::string> texts{};
  std::vector<std::pair<std::int64_t, std::int64_t>> spans{}; // [token_start, token_end) for the ORG entity
  for (int i = 0; i < 30; ++i) {
    texts.push_back("alice works at acme");
    spans.emplace_back(3, 4);
    texts.push_back("bob works at initech");
    spans.emplace_back(3, 4);
  }
  auto docs = tokenizer.infer(texts);
  for (std::size_t i = 0; i < docs.size(); ++i) {
    docs[i].entities.push_back(Span{
        .token_start = spans[i].first,
        .token_end = spans[i].second,
        .label = "ORG",
    });
  }

  NerStage ner({}, NerOptions{
                       .allow_overlap = false,
                       .use_heuristics = false,
                       .prefer_learned = false,
                       .backend = TaskBackend::Sequence,
                       .sequence_width = 24,
                       .sequence_max_span_width = 3,
                       .sequence_min_score = 0.0,
                       .sequence_seed = 9,
                   });
  ner.learn(docs, TrainOptions{});

  auto test_doc = tokenizer.infer("carol works at acme");
  ner.apply(test_doc);
  if (!has_entity(test_doc, "ORG", "acme")) {
    if (err) {
      *err << "nodeb NER Sequence backend should learn to recognize the ORG entity via BIO tagging; got "
           << test_doc.entities.size() << " entities:";
      for (const auto& entity : test_doc.entities) {
        *err << " [" << entity.label << "=" << entity.text(test_doc) << "]";
      }
      *err << "\n";
    }
    return false;
  }
  if (has_entity(test_doc, "ORG", "carol") || has_entity(test_doc, "ORG", "works")) {
    if (err) *err << "nodeb NER Sequence backend should not tag non-entity tokens as ORG\n";
    return false;
  }

  // Save/load round trip must preserve the trained classifier exactly.
  Workflow wf{.lang = Language::English,
              .stages = {stage::tokenize(),
                        std::make_shared<NerStage>(std::vector<EntityPattern>{}, NerOptions{
                                                       .allow_overlap = false,
                                                       .use_heuristics = false,
                                                       .prefer_learned = false,
                                                       .backend = TaskBackend::Sequence,
                                                       .sequence_width = 24,
                                                       .sequence_max_span_width = 3,
                                                       .sequence_min_score = 0.0,
                                                       .sequence_seed = 9,
                                                   })}};
  wf.learn(docs, TrainOptions{});
  const auto before = wf.infer("dave works at initech");

  const auto tmp = std::filesystem::temp_directory_path() / "nodeb_ner_sequence_roundtrip.bin";
  wf.save(tmp.string());
  const auto restored = Workflow::load(tmp.string());
  std::filesystem::remove(tmp);
  const auto after = restored.infer("dave works at initech");

  if (before.entities.size() != after.entities.size()) {
    if (err) *err << "nodeb NER Sequence backend save/load entity count mismatch\n";
    return false;
  }
  if (!has_entity(after, "ORG", "initech")) {
    if (err) *err << "nodeb NER Sequence backend save/load should preserve a genuinely trained classifier\n";
    return false;
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("nodeb ner sequence tranche", "[nodeb][ner][sequence]") {
  if (!run_ner_sequence_checks(&std::cerr)) {
    FAIL("nodeb ner sequence checks failed");
  }
}
#else
int main() {
  if (!run_ner_sequence_checks(&std::cerr)) {
    return 1;
  }
  std::cout << "nodeb ner sequence checks passed\n";
  return 0;
}
#endif
