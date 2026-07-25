#include <pluss/nlp.h>

#include <filesystem>
#include <iostream>
#include <vector>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

namespace {

using namespace pluss::ai::nodeb;

bool run_nodeb_workflow_checks(std::ostream* err = nullptr) {
  Workflow nlp{
      .lang = Language::English,
      .stages = {
          stage::tokenize(),
          stage::sentencize(),
          stage::lemmatize(),
      },
  };

  const std::vector<std::string> docs = {
      "Dr. Laurel writes APIs. Second sentence!",
      "We keep examples, e.g. docs for APIs.\nFourth line?",
  };

  nlp.learn(docs, TrainOptions{.allow_empty_docs = false});
  const auto inferred = nlp.infer(docs);
  if (inferred.size() != 2 || inferred[0].sentences.size() != 2 || inferred[1].sentences.size() != 2) {
    if (err) {
      *err << "nodeb workflow should tokenize and sentencize each document; got sentence counts="
           << inferred[0].sentences.size() << ", " << inferred[1].sentences.size() << "\n";
      for (std::size_t i = 0; i < inferred.size(); ++i) {
        *err << "doc[" << i << "] tokens:";
        for (const auto& token : inferred[i].token_texts()) {
          *err << " [" << token << "]";
        }
        *err << "\n";
        *err << "doc[" << i << "] sentences:";
        for (const auto& sentence : inferred[i].sentence_texts()) {
          *err << " {" << sentence << "}";
        }
        *err << "\n";
      }
    }
    return false;
  }
  if (!inferred[0].tokens.front().sentence_start || !inferred[0].tokens[4].sentence_end) {
    if (err) *err << "nodeb sentencizer should mark token sentence boundaries\n";
    return false;
  }
  if (inferred[0].sentence_texts().size() != 2 || inferred[1].sentence_texts().size() != 2) {
    if (err) *err << "nodeb sentencizer should avoid splitting inside abbreviations like Dr. and e.g.\n";
    return false;
  }
  const std::vector<std::string> want_lemmas = {"dr.", "laurel", "write", "api", ".", "second", "sentence", "!"};
  if (inferred[0].lemma_texts() != want_lemmas) {
    if (err) *err << "nodeb lemmatizer should populate simple lemma forms alongside tokens\n";
    return false;
  }

  const auto report = nlp.assess(docs, AssessOptions{.require_sentences = true});
  if (report.doc_count != 2 || report.sentence_count != 4 || report.token_count <= 0) {
    if (err) *err << "nodeb assess should summarize workflow output counts\n";
    return false;
  }

  const auto tmp = std::filesystem::temp_directory_path() / "nodeb_workflow_tranche1.bin";
  nlp.save(tmp.string());
  const auto restored = Workflow::load(tmp.string());
  const auto restored_docs = restored.infer(docs);
  if (restored_docs[0].token_texts() != inferred[0].token_texts() ||
      restored_docs[1].sentence_texts() != inferred[1].sentence_texts()) {
    if (err) *err << "nodeb save/load should preserve stage execution behavior\n";
    return false;
  }
  std::filesystem::remove(tmp);

  Workflow textcat{
      .lang = Language::English,
      .stages = {
          stage::tokenize(),
          stage::lemmatize(),
          stage::textcat(),
      },
  };
  const std::vector<std::string> train_docs = {
      "Compiler errors break builds",
      "C++ headers need cleanup",
      "Fresh bread and soup",
      "Bake a sweet cake",
  };
  const std::vector<std::string> train_labels = {
      "tech",
      "tech",
      "food",
      "food",
  };
  textcat.learn(train_docs, train_labels);
  const auto predicted = textcat.infer(std::vector<std::string>{
      "Headers and compiler warnings",
      "Bread soup and dinner",
  });
  if (predicted.size() != 2 ||
      predicted[0].doc_label != "tech" ||
      predicted[1].doc_label != "food") {
    if (err) *err << "nodeb textcat should learn simple document labels from lemmatized text\n";
    return false;
  }

  const auto tmp_textcat = std::filesystem::temp_directory_path() / "nodeb_textcat_persist_v2.bin";
  textcat.save(tmp_textcat.string());
  const auto restored_textcat = Workflow::load(tmp_textcat.string());
  const auto restored_predicted = restored_textcat.infer(std::vector<std::string>{
      "Headers and compiler warnings",
      "Bread soup and dinner",
  });
  if (restored_predicted.size() != 2 ||
      restored_predicted[0].doc_label != predicted[0].doc_label ||
      restored_predicted[1].doc_label != predicted[1].doc_label) {
    if (err) {
      *err << "nodeb textcat save/load should preserve trained document classification behavior\n";
      *err << "before: [" << predicted[0].doc_label << ", " << predicted[1].doc_label << "]\n";
      *err << "after : [" << restored_predicted[0].doc_label << ", " << restored_predicted[1].doc_label << "]\n";
      *err << "scores: [" << restored_predicted[0].doc_score << ", " << restored_predicted[1].doc_score << "]\n";
    }
    return false;
  }
  std::filesystem::remove(tmp_textcat);

  Workflow matcher{
      .lang = Language::English,
      .stages = {
          stage::tokenize(),
          stage::lemmatize(),
          stage::matcher({
              MatcherPattern{.label = "lang", .tokens = {"c++"}, .use_lemmas = false},
              MatcherPattern{.label = "action", .tokens = {"write", "api"}, .use_lemmas = true},
          }),
      },
  };
  const auto matched = matcher.infer("Laurel writes APIs in C++.");
  if (matched.matches.size() != 2 ||
      matched.matches[0].label != "action" ||
      matched.matches[0].text(matched) != "writes APIs" ||
      matched.matches[1].label != "lang" ||
      matched.matches[1].text(matched) != "C++") {
    if (err) *err << "nodeb matcher should label token-span matches from orth and lemma patterns\n";
    return false;
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("nodeb workflow tranche", "[nodeb][workflow]") {
  if (!run_nodeb_workflow_checks(&std::cerr)) {
    FAIL("nodeb workflow checks failed");
  }
}
#else
int main() {
  if (!run_nodeb_workflow_checks(&std::cerr)) {
    return 1;
  }
  std::cout << "nodeb workflow checks passed\n";
  return 0;
}
#endif
