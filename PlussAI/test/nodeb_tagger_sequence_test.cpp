#include <pluss/nlp.h>

#include <filesystem>
#include <iostream>
#include <vector>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

namespace {

using namespace pluss::ai::nodeb;

// A tiny hand-tagged corpus with a clear, learnable pattern: "the/DET
// cat/NOUN sleeps/VERB" style sentences, repeated with substitutions so a
// real per-token classifier has signal, while heuristics/lexical-memory
// are disabled so every prediction is forced through the Sequence tier.
bool run_tagger_sequence_checks(std::ostream* err = nullptr) {
  Workflow tokenizer{.lang = Language::English, .stages = {stage::tokenize()}};
  std::vector<std::string> texts{};
  std::vector<std::vector<std::string>> tags{};
  for (int i = 0; i < 30; ++i) {
    texts.push_back("the cat sleeps");
    tags.push_back({"DET", "NOUN", "VERB"});
    texts.push_back("the dog runs");
    tags.push_back({"DET", "NOUN", "VERB"});
  }
  auto docs = tokenizer.infer(texts);
  for (std::size_t i = 0; i < docs.size(); ++i) {
    for (std::size_t t = 0; t < docs[i].tokens.size() && t < tags[i].size(); ++t) {
      docs[i].tokens[t].pos = tags[i][t];
    }
  }

  TaggerStage tagger(TaggerOptions{
      .prefer_learned = false,
      .use_heuristics = false,
      .backend = TaskBackend::Sequence,
      .sequence_width = 24,
      .sequence_seed = 5,
  });
  tagger.learn(docs, TrainOptions{});

  auto test_doc = tokenizer.infer("the cat runs");
  tagger.apply(test_doc);
  if (test_doc.tokens.size() != 3 || test_doc.tokens[0].pos != "DET" ||
      test_doc.tokens[1].pos != "NOUN" || test_doc.tokens[2].pos != "VERB") {
    if (err) {
      *err << "nodeb tagger Sequence backend should learn real per-token tags, got:";
      for (const auto& token : test_doc.tokens) *err << " [" << token.orth << "=" << token.pos << "]";
      *err << "\n";
    }
    return false;
  }

  // Save/load round trip must preserve the trained classifier exactly.
  Workflow wf{.lang = Language::English,
              .stages = {stage::tokenize(),
                        std::make_shared<TaggerStage>(TaggerOptions{
                            .prefer_learned = false,
                            .use_heuristics = false,
                            .backend = TaskBackend::Sequence,
                            .sequence_width = 24,
                            .sequence_seed = 5,
                        })}};
  wf.learn(docs, TrainOptions{});
  const auto before = wf.infer("the dog sleeps");

  const auto tmp = std::filesystem::temp_directory_path() / "nodeb_tagger_sequence_roundtrip.bin";
  wf.save(tmp.string());
  const auto restored = Workflow::load(tmp.string());
  std::filesystem::remove(tmp);
  const auto after = restored.infer("the dog sleeps");

  if (before.tokens.size() != after.tokens.size()) {
    if (err) *err << "nodeb tagger Sequence backend save/load token count mismatch\n";
    return false;
  }
  for (std::size_t i = 0; i < before.tokens.size(); ++i) {
    if (before.tokens[i].pos != after.tokens[i].pos) {
      if (err) *err << "nodeb tagger Sequence backend save/load should preserve predicted tags\n";
      return false;
    }
  }
  if (after.tokens[0].pos != "DET" || after.tokens[1].pos != "NOUN" || after.tokens[2].pos != "VERB") {
    if (err) *err << "nodeb tagger Sequence backend save/load should preserve a genuinely trained classifier\n";
    return false;
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("nodeb tagger sequence tranche", "[nodeb][tagger][sequence]") {
  if (!run_tagger_sequence_checks(&std::cerr)) {
    FAIL("nodeb tagger sequence checks failed");
  }
}
#else
int main() {
  if (!run_tagger_sequence_checks(&std::cerr)) {
    return 1;
  }
  std::cout << "nodeb tagger sequence checks passed\n";
  return 0;
}
#endif
