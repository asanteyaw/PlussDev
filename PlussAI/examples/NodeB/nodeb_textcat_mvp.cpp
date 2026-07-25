#include <pluss/nlp.h>

#include <iostream>
#include <vector>

int main() {
  using namespace pluss::ai::nodeb;

  Workflow nlp{
      .lang = Language::English,
      .stages = {
          stage::tokenize(),
          stage::sentencize(),
          stage::lemmatize(),
          stage::textcat(),
      },
  };

  const std::vector<std::string> train_docs = {
      "Laurel writes clean C++ code. She also documents APIs.",
      "Kojo fixes compiler errors and linker issues.",
      "Ama bakes bread and cooks soup for dinner.",
      "Kojo reviews tasty recipes and cooks stew.",
  };
  const std::vector<std::string> labels = {
      "tech",
      "tech",
      "food",
      "food",
  };
  const std::vector<std::string> docs = {
      "Laurel writes clean C++ code. She also documents APIs.",
      "Ama cooks bread soup for dinner.",
  };

  nlp.learn(train_docs, labels, TrainOptions{.random = {.seed = 42, .deterministic = true}});
  const auto inferred = nlp.infer(docs);
  const auto report = nlp.assess(docs, AssessOptions{.require_sentences = true});

  std::cout << "nodeb docs=" << report.doc_count
            << " tokens=" << report.token_count
            << " sentences=" << report.sentence_count << "\n";

  for (std::size_t i = 0; i < inferred.size(); ++i) {
    const auto token_text = inferred[i].token_texts();
    const auto lemma_text = inferred[i].lemma_texts();
    std::cout << "doc[" << i << "] tokens:";
    for (const auto& token : token_text) {
      std::cout << " [" << token << "]";
    }
    std::cout << "\n";
    std::cout << "doc[" << i << "] lemmas:";
    for (const auto& lemma : lemma_text) {
      std::cout << " [" << lemma << "]";
    }
    std::cout << "\n";
    std::cout << "doc[" << i << "] label=" << inferred[i].doc_label
              << " score=" << inferred[i].doc_score << "\n";
  }

  return 0;
}
