#include <pluss/nlp.h>

#include <filesystem>
#include <algorithm>
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

  // The Sequence backend must take effect even with heuristics enabled. The
  // tags ZZZ and YYY are ones the heuristics never produce, so seeing only
  // them proves the sequence model ran. Training needs two distinct labels.
  {
    Workflow base{.lang = Language::English, .stages = {stage::tokenize(), stage::lemmatize()}};
    std::vector<Doc> train;
    for (const char* text : {"Ada writes books.", "Kofi reads notes."}) {
      Doc doc = base.infer(text);
      for (auto& token : doc.tokens) token.pos = token.index == 0 ? "ZZZ" : "YYY";
      train.push_back(doc);
    }
    auto tagger = std::make_shared<TaggerStage>(TaggerOptions{
        .prefer_learned = false,
        .use_heuristics = true,
        .backend = TaskBackend::Sequence,
        .sequence_width = 16,
        .sequence_seed = 5,
    });
    tagger->learn(train, {});
    Workflow tagged{.lang = Language::English, .stages = {stage::tokenize(), stage::lemmatize(), tagger}};
    const auto doc = tagged.infer("Ada writes books.");
    for (const auto& token : doc.tokens) {
      if (token.pos != "ZZZ" && token.pos != "YYY") {
        if (err) *err << "nodeb tagger Sequence backend should win over heuristics; got [" << token.orth << "=" << token.pos << "]\n";
        return false;
      }
    }
  }

  // sequence_learning_rate must reach the single-label Sequence trainer: a
  // tiny rate barely moves the model, so its score stays near chance.
  {
    const std::vector<std::string> texts = {"bake bread flour", "cook pasta sauce"};
    const std::vector<std::string> labels = {"food", "tech"};
    auto score_with_rate = [&](double rate) {
      auto textcat = std::make_shared<TextCatStage>(TextCatOptions{
          .backend = TaskBackend::Sequence,
          .sequence_width = 8,
          .sequence_learning_rate = rate,
          .sequence_seed = 3,
      });
      Workflow wf{.lang = Language::English, .stages = {stage::tokenize(), textcat}};
      wf.learn(texts, labels, TrainOptions{});
      return wf.infer("bake bread flour").doc_score;
    };
    const double slow = score_with_rate(1e-9);
    const double fast = score_with_rate(5e-2);
    if (!(slow < 0.6 && fast > 0.9)) {
      if (err) *err << "nodeb textcat sequence_learning_rate should control training; slow=" << slow << " fast=" << fast << "\n";
      return false;
    }
  }

  // allow_overlap=false rejects a match that intersects any earlier match,
  // including one produced by a different pattern.
  for (const bool overlap : {true, false}) {
    Workflow matched{
        .lang = Language::English,
        .stages = {stage::tokenize(),
                   stage::matcher({{.label = "CITY", .tokens = {"New", "York"}, .use_lemmas = false},
                                   {.label = "PAPER", .tokens = {"York", "Times"}, .use_lemmas = false}},
                                  MatcherOptions{.lowercase = true, .allow_overlap = overlap})},
    };
    const auto doc = matched.infer("New York Times said");
    const std::size_t want = overlap ? 2U : 1U;
    if (doc.matches.size() != want) {
      if (err) {
        *err << "nodeb matcher with allow_overlap=" << overlap << " should keep " << want
             << " match(es); got " << doc.matches.size() << "\n";
      }
      return false;
    }
  }

  // skip_punct=false keeps punctuation marks as vectorizer terms, and
  // describe() counts the same terms the mapper learns.
  {
    Workflow punct{.lang = Language::English, .stages = {stage::tokenize(), stage::lemmatize()}};
    const auto docs = punct.infer(std::vector<std::string>{"Ada? Yes, Ada."});
    for (const bool skip : {true, false}) {
      vectorize::Mapper mapper{pluss::ai::nodej::LexiconMapOptions{}, vectorize::DocMapOptions{.skip_punct = skip}};
      if (!mapper.learn(docs).ok()) {
        if (err) *err << "nodeb vectorize Mapper::learn should succeed on the punctuation corpus\n";
        return false;
      }
      const bool has_mark = std::find(mapper.vocabulary().begin(), mapper.vocabulary().end(), "?") != mapper.vocabulary().end();
      if (has_mark == skip) {
        if (err) *err << "nodeb vectorize skip_punct=" << skip << " should " << (skip ? "drop" : "keep") << " punctuation terms\n";
        return false;
      }
      const auto stats = vectorize::describe(docs, vectorize::DocMapOptions{.skip_punct = skip});
      if (stats.unique_term_count != static_cast<std::int64_t>(mapper.vocabulary().size())) {
        if (err) *err << "nodeb vectorize describe should count the mapper's terms (skip_punct=" << skip << ")\n";
        return false;
      }
    }
  }

  // lowercase=false keeps the surface form for the rule fallback, but the
  // language lookup still applies: a table lemma is canonical already.
  Workflow cased{
      .lang = Language::English,
      .stages = {stage::tokenize(),
                 stage::lemmatize(LemmatizerOptions{.lowercase = false})},
  };
  const auto cased_doc = cased.infer("They WROTE Books.");
  if (cased_doc.tokens[1].lemma != "write" || cased_doc.tokens[2].lemma != "Books") {
    if (err) {
      *err << "nodeb lemmatizer with lowercase=false should apply language lookup and keep surface case; got ["
           << cased_doc.tokens[1].lemma << "] [" << cased_doc.tokens[2].lemma << "]\n";
    }
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
