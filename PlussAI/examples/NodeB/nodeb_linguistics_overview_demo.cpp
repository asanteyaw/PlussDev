#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  using namespace pluss::ai::nodeb;

  Workflow nlp{
      .lang = Language::English,
      .stages = {
          stage::tokenize(),
          stage::sentencize(),
          stage::lemmatize(),
          stage::tagger(),
          stage::matcher({
              MatcherPattern{.label = "STACK", .tokens = {"clean", "c++"}, .use_lemmas = false},
          }),
          stage::ner({
              EntityPattern{.label = "GPE", .tokens = {"New", "York"}, .use_lemmas = false},
          }),
          stage::syntax(),
      },
  };

  const std::string text =
      "Dr. Laurel writes clean C++ APIs for NASA in New York. "
      "Email help@pluss.dev for support.";
  const auto doc = nlp.infer(text);
  const auto report = nlp.assess(std::vector<std::string>{text}, AssessOptions{.require_sentences = true});

  std::cout << "== NodeB Linguistics Overview ==\n";
  std::cout << "docs=" << report.doc_count
            << " tokens=" << report.token_count
            << " sentences=" << report.sentence_count << "\n\n";

  std::cout << "Tokens\n";
  for (const auto& token : doc.tokens) {
    std::cout << "  [" << token.index << "] "
              << token.orth
              << " lemma=" << token.lemma
              << " pos=" << token.pos
              << " role=" << token.role
              << " chief=" << token.chief;
    if (!token.entity_label.empty()) {
      std::cout << " entity=" << token.entity_label;
    }
    std::cout << "\n";
  }

  std::cout << "\nSentences\n";
  for (const auto& sentence : doc.sentence_texts()) {
    std::cout << "  - " << sentence << "\n";
  }

  std::cout << "\nMatches\n";
  for (const auto& match : doc.matches) {
    std::cout << "  - " << match.label << ": " << match.text(doc) << "\n";
  }

  std::cout << "\nEntities\n";
  for (const auto& entity : doc.entities) {
    std::cout << "  - " << entity.label << ": " << entity.text(doc) << "\n";
  }

  std::cout << "\nPhrases\n";
  for (const auto& phrase : doc.phrase_texts()) {
    std::cout << "  - " << phrase << "\n";
  }

  return 0;
}
