#include <pluss/nlp.h>

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <vector>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

namespace {

using namespace pluss::ai::nodeb;

bool has_label(const Doc& doc, std::string_view label) {
  return std::ranges::any_of(doc.doc_labels, [&](const auto& entry) { return entry.first == label; });
}

bool run_nodeb_textcat_checks(std::ostream* err = nullptr) {
  Workflow tokenizer{.lang = Language::English, .stages = {stage::tokenize(), stage::lemmatize()}};

  // --- Sequence backend: real gradient-trained single-label classification ---
  Workflow single{
      .lang = Language::English,
      .stages = {stage::tokenize(), stage::lemmatize(),
                stage::textcat(TextCatOptions{
                    .backend = TaskBackend::Sequence,
                    .sequence_width = 24,
                    .sequence_seed = 7,
                })},
  };
  const std::vector<std::string> docs = {
      "Compiler warnings break builds",
      "Unit tests guard regressions",
      "Beaches make quiet holidays",
      "Flights land near coastal towns",
  };
  const std::vector<std::string> labels = {"tech", "tech", "travel", "travel"};
  single.learn(docs, labels);
  const auto predicted = single.infer(docs);
  if (predicted[0].doc_label != "tech" || predicted[1].doc_label != "tech" ||
      predicted[2].doc_label != "travel" || predicted[3].doc_label != "travel") {
    if (err) {
      *err << "nodeb textcat Sequence backend should learn a real classifier, not a fixed guess. Got: "
           << predicted[0].doc_label << ", " << predicted[1].doc_label << ", " << predicted[2].doc_label << ", "
           << predicted[3].doc_label << "\n";
    }
    return false;
  }
  for (const auto& doc : predicted) {
    if (!(doc.doc_score > 0.5)) {
      if (err) *err << "nodeb textcat Sequence backend should report a confident softmax score above 0.5\n";
      return false;
    }
  }

  const auto tmp = std::filesystem::temp_directory_path() / "nodeb_textcat_sequence_roundtrip.bin";
  single.save(tmp.string());
  const auto restored = Workflow::load(tmp.string());
  std::filesystem::remove(tmp);
  const auto restored_predicted = restored.infer(docs);
  for (std::size_t i = 0; i < docs.size(); ++i) {
    if (restored_predicted[i].doc_label != predicted[i].doc_label) {
      if (err) *err << "nodeb textcat Sequence backend save/load should preserve the trained classifier\n";
      return false;
    }
  }

  // --- Multi-label classification ---
  Workflow multi{
      .lang = Language::English,
      .stages = {stage::tokenize(), stage::lemmatize(),
                stage::textcat(TextCatOptions{
                    .multi_label = true,
                    .backend = TaskBackend::Sequence,
                    .sequence_width = 24,
                    .sequence_seed = 11,
                })},
  };
  const std::vector<std::string> multi_docs = {
      "Compiler warnings break the build",
      "The urgent compiler warnings need a hotfix tonight",
      "Beaches make quiet holidays",
      "The urgent flight delay ruined the holiday",
  };
  const std::vector<std::string> multi_labels = {
      "tech",
      "tech|urgent",
      "travel",
      "travel|urgent",
  };
  multi.learn(multi_docs, multi_labels);
  const auto multi_predicted = multi.infer(multi_docs);
  if (!has_label(multi_predicted[0], "tech") || has_label(multi_predicted[0], "urgent")) {
    if (err) *err << "nodeb textcat multi-label doc 0 should be tagged tech only\n";
    return false;
  }
  if (!has_label(multi_predicted[1], "tech") || !has_label(multi_predicted[1], "urgent")) {
    if (err) *err << "nodeb textcat multi-label doc 1 should be tagged both tech and urgent\n";
    return false;
  }
  if (!has_label(multi_predicted[2], "travel") || has_label(multi_predicted[2], "urgent")) {
    if (err) *err << "nodeb textcat multi-label doc 2 should be tagged travel only\n";
    return false;
  }
  if (!has_label(multi_predicted[3], "travel") || !has_label(multi_predicted[3], "urgent")) {
    if (err) *err << "nodeb textcat multi-label doc 3 should be tagged both travel and urgent\n";
    return false;
  }
  for (const auto& doc : multi_predicted) {
    if (!doc.doc_label.empty()) {
      if (err) *err << "nodeb textcat multi-label predictions should leave the single-label doc_label unset\n";
      return false;
    }
  }

  const auto multi_tmp = std::filesystem::temp_directory_path() / "nodeb_textcat_multilabel_roundtrip.bin";
  multi.save(multi_tmp.string());
  const auto multi_restored = Workflow::load(multi_tmp.string());
  std::filesystem::remove(multi_tmp);
  const auto multi_restored_predicted = multi_restored.infer(multi_docs);
  for (std::size_t i = 0; i < multi_docs.size(); ++i) {
    if (multi_restored_predicted[i].doc_labels.size() != multi_predicted[i].doc_labels.size()) {
      if (err) *err << "nodeb textcat multi-label save/load should preserve predicted tag sets\n";
      return false;
    }
  }

  // --- Error paths ---
  bool threw = false;
  try {
    Workflow bad{
        .lang = Language::English,
        .stages = {stage::tokenize(),
                  stage::textcat(TextCatOptions{.multi_label = true, .backend = TaskBackend::Classic})},
    };
    bad.learn(multi_docs, multi_labels);
  } catch (const std::exception&) {
    threw = true;
  }
  if (!threw) {
    if (err) *err << "nodeb textcat should reject multi_label with the Classic backend\n";
    return false;
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("nodeb textcat tranche", "[nodeb][textcat]") {
  if (!run_nodeb_textcat_checks(&std::cerr)) {
    FAIL("nodeb textcat checks failed");
  }
}
#else
int main() {
  if (!run_nodeb_textcat_checks(&std::cerr)) {
    return 1;
  }
  std::cout << "nodeb textcat checks passed\n";
  return 0;
}
#endif
