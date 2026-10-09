#include <pluss/nlp.h>

#include <filesystem>
#include <iostream>
#include <vector>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

namespace {

using namespace pluss::ai::nodeb;

Doc make_sequence_gold_doc(std::string_view text,
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
        "nodeb deep heads test: token count and POS count must match.");
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

bool run_nodeb_deep_head_checks(std::ostream* err = nullptr) {
  const std::vector<std::string_view> pos1 = {"PROPN", "VERB", "PROPN", "PUNCT"};
  const std::vector<std::string_view> pos2 = {"PROPN", "VERB", "PROPN", "PUNCT"};
  const auto gold_docs = std::vector<Doc>{
      make_sequence_gold_doc("Laurel mentors Accra.",
                             pluss::msharp::RangeView::Of<std::string_view>(pos1),
                             {Span{.token_start = 0, .token_end = 1, .label = "PERSON"},
                              Span{.token_start = 2, .token_end = 3, .label = "GPE"}}),
      make_sequence_gold_doc("Kofi visits Tema.",
                             pluss::msharp::RangeView::Of<std::string_view>(pos2),
                             {Span{.token_start = 0, .token_end = 1, .label = "PERSON"},
                              Span{.token_start = 2, .token_end = 3, .label = "GPE"}}),
  };

  Workflow seq_nlp{
      .lang = Language::English,
      .stages = {
          stage::tokenize(),
          stage::sentencize(),
          stage::lemmatize(),
          stage::tagger(TaggerOptions{
              .lowercase = true,
              .use_lemmas = true,
              .prefer_learned = false,
              .use_heuristics = false,
              .backend = TaskBackend::Sequence,
              .sequence_width = 32,
              .sequence_seed = 7,
          }),
          stage::ner({}, NerOptions{
              .lowercase = true,
              .allow_overlap = false,
              .use_heuristics = false,
              .prefer_learned = false,
              .backend = TaskBackend::Sequence,
              .sequence_width = 32,
              .sequence_max_span_width = 1,
              .sequence_min_score = 0.80,
              .sequence_seed = 11,
          }),
      },
  };

  seq_nlp.learn(gold_docs, TrainOptions{.allow_empty_docs = false});
  const auto predicted = seq_nlp.infer(std::vector<std::string>{
      "Laurel mentors Accra.",
      "Kofi visits Tema.",
  });
  const auto tag_score = assess::tagger(predicted, gold_docs);
  const auto entity_score = assess::ner(predicted, gold_docs);
  if (tag_score.accuracy != 1.0) {
    if (err) *err << "nodeb deep tagger should recover training tags through the sequence backend\n";
    return false;
  }
  if (entity_score.f1 != 1.0) {
    if (err) *err << "nodeb deep ner should recover training entities through the sequence backend\n";
    return false;
  }

  Workflow textcat{
      .lang = Language::English,
      .stages = {
          stage::tokenize(),
          stage::sentencize(),
          stage::lemmatize(),
          stage::textcat(TextCatOptions{
              .lowercase = true,
              .use_lemmas = true,
              .backend = TaskBackend::Sequence,
              .sequence_width = 40,
              .sequence_seed = 13,
          }),
      },
  };

  const std::vector<std::string> textcat_docs{
      "Compiler warnings break builds.",
      "Unit tests guard regressions.",
      "Beaches make quiet holidays.",
      "Flights land near coastal towns.",
  };
  const std::vector<std::string> textcat_labels{
      "tech",
      "tech",
      "travel",
      "travel",
  };
  textcat.learn(textcat_docs, textcat_labels, TrainOptions{.allow_empty_docs = false});
  const auto labeled = textcat.infer(textcat_docs);
  if (labeled[0].doc_label != "tech" ||
      labeled[1].doc_label != "tech" ||
      labeled[2].doc_label != "travel" ||
      labeled[3].doc_label != "travel") {
    if (err) *err << "nodeb deep textcat should classify memorized training examples through the sequence backend\n";
    return false;
  }

  const auto tmp = std::filesystem::temp_directory_path() / "nodeb_deep_heads_v1.bin";
  textcat.save(tmp.string());
  const auto restored = Workflow::load(tmp.string());
  std::filesystem::remove(tmp);
  const auto restored_labeled = restored.infer(textcat_docs);
  if (restored_labeled[0].doc_label != "tech" ||
      restored_labeled[3].doc_label != "travel") {
    if (err) *err << "nodeb deep textcat save-load path should preserve sequence centroids and embeddings\n";
    return false;
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("nodeb deep heads tranche", "[nodeb][deep]") {
  if (!run_nodeb_deep_head_checks(&std::cerr)) {
    FAIL("nodeb deep head checks failed");
  }
}
#else
int main() {
  if (!run_nodeb_deep_head_checks(&std::cerr)) {
    return 1;
  }
  std::cout << "nodeb deep head checks passed\n";
  return 0;
}
#endif
