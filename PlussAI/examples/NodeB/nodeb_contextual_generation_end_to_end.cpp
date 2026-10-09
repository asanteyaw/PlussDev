#include <pluss/nlp.h>

#include <iostream>
#include <vector>

int main() {
  namespace nlp_api = pluss::ai::nlp;
  using namespace nlp_api;
  using nlp_api::embed::ContextualEmbedOptions;
  using nlp_api::embed::ContextualEmbedder;
  using nlp_api::embed::Embedder;

  std::vector<std::string> corpus{};
  for (int i = 0; i < 40; ++i) {
    corpus.push_back("the bank raised interest rates");
    corpus.push_back("the bank increased interest rates");
    corpus.push_back("the river bank flooded overnight");
    corpus.push_back("the muddy bank flooded overnight");
  }

  Workflow prep{
      .lang = Language::English,
      .stages = {stage::tokenize(), stage::lemmatize()},
  };
  const auto docs = prep.infer(corpus);

  ContextualEmbedOptions options{};
  options.width = 24;
  options.num_layers = 2;
  options.num_heads = 4;
  options.feedforward_width = 48;
  options.epochs = 120;
  options.learning_rate = 5e-3;
  options.seed = 3;

  ContextualEmbedder model(options);
  const auto status = model.learn(docs);
  if (!status.ok()) {
    std::cerr << "contextual learn failed: " << status.message << "\n";
    return 1;
  }

  const auto financial = prep.infer("the bank raised interest rates");
  const auto geographic = prep.infer("the river bank flooded overnight");
  const auto fin_rows = model.encode(financial);
  const auto geo_rows = model.encode(geographic);
  if (!fin_rows.ok() || !geo_rows.ok()) {
    std::cerr << "contextual encode failed\n";
    return 1;
  }

  const double bank_similarity =
      Embedder::similarity(fin_rows.value()[1], geo_rows.value()[2]);

  const auto prompt = prep.infer("the bank raised interest");
  const auto generated = model.generate(prompt, embed::GenerateOptions{
                                                    .max_new_tokens = 3,
                                                    .greedy = true,
                                                });
  if (!generated.ok()) {
    std::cerr << "generation failed: " << generated.status().message << "\n";
    return 1;
  }

  std::cout << "== PlussAI NLP Contextual Embeddings and Generation ==\n";
  std::cout << "cross-context similarity(bank_financial, bank_geographic)="
            << bank_similarity << "\n";
  std::cout << "prompt: the bank raised interest\n";
  std::cout << "generated:";
  for (const auto& token : generated.value()) {
    std::cout << " [" << token << "]";
  }
  std::cout << "\n";

  return 0;
}
