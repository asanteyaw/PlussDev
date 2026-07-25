#include <pluss/nlp.h>

#include <filesystem>
#include <iostream>
#include <vector>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

namespace {

using namespace pluss::ai::nodeb;

bool run_nodeb_syntax_checks(std::ostream* err = nullptr) {
  Workflow nlp{
      .lang = Language::English,
      .stages = {
          stage::tokenize(),
          stage::sentencize(),
          stage::lemmatize(),
          stage::tagger(),
          stage::syntax(),
      },
  };

  const auto doc = nlp.infer("Dr. Laurel writes APIs for NASA in New York.");
  if (doc.tokens.size() != 10) {
    if (err) *err << "nodeb syntax baseline expected 10 tokens in the probe sentence\n";
    return false;
  }
  if (doc.tokens[2].orth != "writes" ||
      doc.tokens[2].chief != 2 ||
      doc.tokens[2].role != "root") {
    if (err) *err << "nodeb syntax should select 'writes' as the sentence root\n";
    return false;
  }
  if (doc.tokens[1].chief != 2 || doc.tokens[1].role != "nsubj") {
    if (err) *err << "nodeb syntax should attach Laurel as the nominal subject\n";
    return false;
  }
  if (doc.tokens[0].chief != 1 || doc.tokens[0].role != "compound") {
    if (err) *err << "nodeb syntax should keep title-like nominal prefixes inside the same phrase\n";
    return false;
  }
  if (doc.tokens[3].chief != 2 || doc.tokens[3].role != "obj") {
    if (err) *err << "nodeb syntax should attach APIs as the object of writes\n";
    return false;
  }
  if (doc.tokens[4].chief != 2 || doc.tokens[4].role != "prep" ||
      doc.tokens[5].chief != 4 || doc.tokens[5].role != "pobj") {
    if (err) *err << "nodeb syntax should create preposition/object structure for 'for NASA'\n";
    return false;
  }
  if (doc.tokens[6].chief != 2 || doc.tokens[6].role != "prep" ||
      doc.tokens[8].chief != 6 || doc.tokens[8].role != "pobj" ||
      doc.tokens[7].chief != 8 || doc.tokens[7].role != "compound") {
    if (err) *err << "nodeb syntax should keep 'New York' as a phrase headed by the object of 'in'\n";
    return false;
  }
  if (doc.tokens[9].role != "punct") {
    if (err) *err << "nodeb syntax should attach sentence punctuation explicitly\n";
    return false;
  }

  const std::vector<std::string> want_phrases = {
      "Dr. Laurel",
      "APIs",
      "NASA",
      "New York",
  };
  if (doc.phrase_texts() != want_phrases) {
    if (err) {
      *err << "nodeb syntax should expose phrase spans built from the dependency baseline\n";
      for (const auto& phrase : doc.phrase_texts()) {
        *err << " {" << phrase << "}";
      }
      *err << "\n";
    }
    return false;
  }

  const auto root_children = doc.children_of(2);
  const std::vector<std::int64_t> want_children = {1, 3, 4, 6, 9};
  if (root_children != want_children) {
    if (err) *err << "nodeb syntax children_of(root) should expose the direct dependents in token order\n";
    return false;
  }

  const auto lineage = doc.lineage_of(8);
  const std::vector<std::int64_t> want_lineage = {8, 6, 2};
  if (lineage != want_lineage) {
    if (err) *err << "nodeb syntax lineage_of(New York head) should walk to the root\n";
    return false;
  }

  const auto branch = doc.branch_of(8);
  if (branch.text(doc) != "New York" ||
      doc.left_edge_of(8) != 7 ||
      doc.right_edge_of(8) != 8) {
    if (err) *err << "nodeb syntax branch/edge helpers should recover the nominal phrase extent\n";
    return false;
  }

  const auto tmp = std::filesystem::temp_directory_path() / "nodeb_syntax_v1.bin";
  nlp.save(tmp.string());
  const auto restored = Workflow::load(tmp.string());
  const auto restored_doc = restored.infer("Dr. Laurel writes APIs for NASA in New York.");
  std::filesystem::remove(tmp);

  if (restored_doc.phrase_texts() != want_phrases ||
      restored_doc.tokens[2].role != "root" ||
      restored_doc.tokens[8].chief != 6) {
    if (err) *err << "nodeb syntax save/load should preserve stage behavior and options\n";
    return false;
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("nodeb syntax tranche", "[nodeb][syntax]") {
  if (!run_nodeb_syntax_checks(&std::cerr)) {
    FAIL("nodeb syntax checks failed");
  }
}
#else
int main() {
  if (!run_nodeb_syntax_checks(&std::cerr)) {
    return 1;
  }
  std::cout << "nodeb syntax checks passed\n";
  return 0;
}
#endif
