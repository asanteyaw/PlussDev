#include <pluss/nlp.h>

#include <iostream>
#include <vector>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

namespace {

using namespace pluss::ai::nodeb;

bool run_nodeb_tokenizer_checks(std::ostream* err = nullptr) {
  Workflow nlp{
      .lang = Language::English,
      .stages = {stage::tokenize()},
  };

  const auto doc = nlp.infer("Laurel's code-review works, yes.");
  const std::vector<std::string> want = {
      "Laurel's", "code-review", "works", ",", "yes", ".",
  };
  if (doc.token_texts() != want) {
    if (err) *err << "nodeb tokenizer should preserve apostrophes, hyphens, and punctuation tokens\n";
    return false;
  }
  if (doc.tokens.size() != 6 || doc.tokens[3].is_punct != true || doc.tokens[0].is_punct != false) {
    if (err) *err << "nodeb tokenizer should classify punctuation tokens correctly\n";
    return false;
  }

  const auto technical = nlp.infer("C++ and C# support live at https://pluss.dev/docs plus help@pluss.dev.");
  const std::vector<std::string> want_technical = {
      "C++", "and", "C#", "support", "live", "at",
      "https://pluss.dev/docs", "plus", "help@pluss.dev", ".",
  };
  if (technical.token_texts() != want_technical) {
    if (err) {
      *err << "nodeb tokenizer should preserve C++, C#, URLs, and emails as whole tokens; got:";
      for (const auto& token : technical.token_texts()) {
        *err << " [" << token << "]";
      }
      *err << "\n";
    }
    return false;
  }

  // Multi-byte letters stay inside their word; non-letter Unicode punctuation
  // becomes its own token; non-breaking spaces separate tokens.
  const auto accented = nlp.infer("Il est là. Café naïve, « oui » — l'été.");
  const std::vector<std::string> want_accented = {
      "Il", "est", "là", ".", "Café", "naïve", ",", "«", "oui", "»", "—", "l'été", ".",
  };
  if (accented.token_texts() != want_accented) {
    if (err) {
      *err << "nodeb tokenizer should keep UTF-8 letters whole; got:";
      for (const auto& token : accented.token_texts()) {
        *err << " [" << token << "]";
      }
      *err << "\n";
    }
    return false;
  }

  const auto batch = nlp.infer(pluss::make_note({"Alpha beta.", "Gamma!"}));
  if (batch.size() != 2 || batch[0].tokens.size() != 3 || batch[1].tokens.size() != 2) {
    if (err) *err << "nodeb tokenizer should infer Note batches document-by-document\n";
    return false;
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("nodeb tokenizer tranche", "[nodeb][tokenize]") {
  if (!run_nodeb_tokenizer_checks(&std::cerr)) {
    FAIL("nodeb tokenizer checks failed");
  }
}
#else
int main() {
  if (!run_nodeb_tokenizer_checks(&std::cerr)) {
    return 1;
  }
  std::cout << "nodeb tokenizer checks passed\n";
  return 0;
}
#endif
