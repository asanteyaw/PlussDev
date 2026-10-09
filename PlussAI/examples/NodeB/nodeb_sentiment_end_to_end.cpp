#include <pluss/nlp.h>

#include <iostream>
#include <vector>

int main() {
  namespace nlp_api = pluss::ai::nlp;
  using namespace nlp_api;

  Workflow sentiment{
      .lang = Language::English,
      .stages = {
          stage::tokenize(),
          stage::sentencize(),
          stage::lemmatize(),
          stage::textcat(TextCatOptions{
              .backend = TaskBackend::Sequence,
              .sequence_width = 32,
              .sequence_seed = 42,
          }),
      },
  };

  const std::vector<std::string> train_docs = {
      "I love this framework and the ergonomics are excellent.",
      "The results are stable, clean, and reliable.",
      "This compiler error ruined my evening.",
      "The build is broken, frustrating, and unreliable.",
      "The API is okay and the output is acceptable.",
      "The behavior is average and mostly neutral.",
  };
  const std::vector<std::string> labels = {
      "positive",
      "positive",
      "negative",
      "negative",
      "neutral",
      "neutral",
  };

  sentiment.learn(
      train_docs,
      labels,
      TrainOptions{.random = {.seed = 42, .deterministic = true}, .allow_empty_docs = false});

  const std::vector<std::string> probes = {
      "The API is clean and reliable.",
      "This broken build is frustrating.",
      "The output is acceptable and fairly average.",
  };
  const auto inferred = sentiment.infer(probes);

  std::cout << "== PlussAI NLP Sentiment / Text Classification ==\n";
  for (std::size_t i = 0; i < probes.size(); ++i) {
    std::cout << "text: " << probes[i] << "\n";
    std::cout << "  label=" << inferred[i].doc_label
              << " score=" << inferred[i].doc_score << "\n";
  }

  return 0;
}
