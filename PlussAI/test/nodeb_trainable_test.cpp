#include <pluss/nlp.h>

#include <filesystem>
#include <iostream>
#include <vector>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

namespace {

using namespace pluss::ai::nodeb;

Doc make_gold_doc(std::string_view text,
                  pluss::msharp::RangeView::Of<std::string_view> pos_tags,
                  std::initializer_list<Span> entities) {
  Workflow prep{
      .lang = Language::English,
      .stages = {
          stage::tokenize(),
          stage::sentencize(),
          stage::lemmatize(),
      },
  };
  auto doc = prep.infer(text);
  if (doc.tokens.size() != static_cast<std::int64_t>(pos_tags.size())) {
    ::pluss::ai::nodeb::detail::fail(
        "nodeb trainable test: token count and gold POS count must match.");
  }
  for (std::size_t i = 0; i < pos_tags.size(); ++i) {
    doc.tokens[i].pos = std::string(pos_tags[i]);
  }
  doc.entities.assign(entities.begin(), entities.end());
  for (const auto& entity : doc.entities) {
    for (std::int64_t i = entity.token_start; i < entity.token_end; ++i) {
      doc.tokens[static_cast<std::size_t>(i)].entity_label = entity.label;
    }
  }
  return doc;
}

bool run_nodeb_trainable_checks(std::ostream* err = nullptr) {
  std::vector<std::string_view> pos1 = {"PROPN", "VERB", "PROPN", "PUNCT"};
  std::vector<std::string_view> pos2 = {"PROPN", "VERB", "PROPN", "PROPN", "PUNCT"};
  const auto gold_docs = std::vector<Doc>{
      make_gold_doc("Laurel mentors Accra.",
                    pluss::msharp::RangeView::Of<std::string_view>(pos1),
                    {Span{.token_start = 0, .token_end = 1, .label = "PERSON"},
                     Span{.token_start = 2, .token_end = 3, .label = "GPE"}}),
      make_gold_doc("Kofi visits New York.",
                    pluss::msharp::RangeView::Of<std::string_view>(pos2),
                    {Span{.token_start = 0, .token_end = 1, .label = "PERSON"},
                     Span{.token_start = 2, .token_end = 4, .label = "GPE"}}),
  };

  Workflow nlp{
      .lang = Language::English,
      .stages = {
          stage::tokenize(),
          stage::sentencize(),
          stage::lemmatize(),
          stage::tagger(TaggerOptions{
              .lowercase = true,
              .use_lemmas = true,
              .prefer_learned = true,
              .use_heuristics = false,
          }),
          stage::ner({}, NerOptions{
              .lowercase = true,
              .allow_overlap = false,
              .use_heuristics = false,
              .prefer_learned = true,
          }),
      },
  };

  nlp.learn(gold_docs, TrainOptions{.allow_empty_docs = false});

  const auto predicted = nlp.infer(std::vector<std::string>{
      "Laurel mentors Accra.",
      "Kofi visits New York.",
  });

  const auto tag_score = assess::tagger(predicted, gold_docs);
  if (tag_score.correct_count != tag_score.token_count || tag_score.accuracy != 1.0) {
    if (err) *err << "nodeb trainable tagger should recover memorized gold labels exactly on seen patterns\n";
    return false;
  }

  const auto entity_score = assess::ner(predicted, gold_docs);
  if (entity_score.correct_count != entity_score.gold_count ||
      entity_score.precision != 1.0 ||
      entity_score.recall != 1.0 ||
      entity_score.f1 != 1.0) {
    if (err) *err << "nodeb trainable ner should recover memorized gold entity spans exactly on seen patterns\n";
    return false;
  }

  if (predicted[1].entity_texts() != std::vector<std::string>{"Kofi", "New York"}) {
    if (err) *err << "nodeb trainable ner should learn multi-token entity phrases\n";
    return false;
  }

  const auto tmp = std::filesystem::temp_directory_path() / "nodeb_trainable_v1.bin";
  nlp.save(tmp.string());
  const auto restored = Workflow::load(tmp.string());
  const auto restored_predicted = restored.infer(std::vector<std::string>{
      "Laurel mentors Accra.",
      "Kofi visits New York.",
  });
  std::filesystem::remove(tmp);

  const auto restored_tags = assess::tagger(restored_predicted, gold_docs);
  const auto restored_ents = assess::ner(restored_predicted, gold_docs);
  if (restored_tags.accuracy != 1.0 || restored_ents.f1 != 1.0) {
    if (err) *err << "nodeb trainable tagger/ner save-load path should preserve learned state\n";
    return false;
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("nodeb trainable tranche", "[nodeb][trainable]") {
  if (!run_nodeb_trainable_checks(&std::cerr)) {
    FAIL("nodeb trainable checks failed");
  }
}
#else
int main() {
  if (!run_nodeb_trainable_checks(&std::cerr)) {
    return 1;
  }
  std::cout << "nodeb trainable checks passed\n";
  return 0;
}
#endif
