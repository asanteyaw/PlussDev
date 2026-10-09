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
  std::vector<std::pair<std::int64_t, std::int64_t>> spans{};
  for (int i = 0; i < 20; ++i) {
    texts.push_back("alice works at acme");
    spans.emplace_back(3, 4);
    texts.push_back("bob works at initech");
    spans.emplace_back(3, 4);
  }

  auto docs = tokenizer.infer(texts);
  for (std::size_t i = 0; i < docs.size(); ++i) {
    docs[i].entities.push_back(Span{
        .token_start = spans[i].first,
        .token_end = spans[i].second,
        .label = "ORG",
    });
  }

  Workflow ner_flow{
      .lang = Language::English,
      .stages = {
          stage::tokenize(),
          std::make_shared<NerStage>(
              std::vector<EntityPattern>{},
              NerOptions{
                  .allow_overlap = false,
                  .use_heuristics = false,
                  .prefer_learned = false,
                  .backend = TaskBackend::Sequence,
                  .sequence_width = 24,
                  .sequence_max_span_width = 3,
                  .sequence_min_score = 0.0,
                  .sequence_seed = 9,
              }),
      },
  };
  ner_flow.learn(docs, TrainOptions{.random = {.seed = 9, .deterministic = true}});

  const auto predicted = ner_flow.infer("carol works at acme");

  std::cout << "== PlussAI NLP Named Entity Recognition ==\n";
  std::cout << "text: carol works at acme\n";
  for (const auto& entity : predicted.entities) {
    std::cout << "  " << entity.label << ": " << entity.text(predicted) << "\n";
  }

  return 0;
}
