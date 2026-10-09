#include <pluss/nlp.h>

#include <iostream>
#include <vector>

int main() {
  namespace nlp_api = pluss::ai::nlp;
  using namespace nlp_api;

  Workflow tokenizer{
      .lang = Language::English,
      .stages = {stage::tokenize()},
  };

  std::vector<std::string> texts{};
  std::vector<std::vector<std::string>> tags{};
  for (int i = 0; i < 20; ++i) {
    texts.push_back("the cat sleeps");
    tags.push_back({"DET", "NOUN", "VERB"});
    texts.push_back("the dog runs");
    tags.push_back({"DET", "NOUN", "VERB"});
  }

  auto docs = tokenizer.infer(texts);
  for (std::size_t i = 0; i < docs.size(); ++i) {
    for (std::size_t t = 0; t < docs[i].tokens.size(); ++t) {
      docs[i].tokens[t].pos = tags[i][t];
    }
  }

  Workflow tagger_flow{
      .lang = Language::English,
      .stages = {
          stage::tokenize(),
          std::make_shared<TaggerStage>(TaggerOptions{
              .prefer_learned = false,
              .use_heuristics = false,
              .backend = TaskBackend::Sequence,
              .sequence_width = 24,
              .sequence_seed = 5,
          }),
      },
  };
  tagger_flow.learn(docs, TrainOptions{.random = {.seed = 5, .deterministic = true}});

  const auto predicted = tagger_flow.infer("the cat runs");

  std::cout << "== PlussAI NLP Sequence Tagger ==\n";
  for (const auto& token : predicted.tokens) {
    std::cout << "  " << token.orth << " -> " << token.pos << "\n";
  }

  return 0;
}
