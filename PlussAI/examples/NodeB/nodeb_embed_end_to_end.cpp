#include <pluss/nlp.h>

#include <iostream>
#include <vector>

int main() {
  using namespace pluss::ai::nodeb;
  using pluss::ai::nodeb::embed::EmbedOptions;
  using pluss::ai::nodeb::embed::Embedder;

  std::vector<std::string> corpus{};
  for (int i = 0; i < 40; ++i) {
    corpus.push_back("cat dog pet animal");
    corpus.push_back("dog cat animal pet");
    corpus.push_back("car truck vehicle machine");
    corpus.push_back("truck car machine vehicle");
  }

  Workflow prep{
      .lang = Language::English,
      .stages = {stage::tokenize()},
  };
  const auto docs = prep.infer(corpus);

  EmbedOptions options{};
  options.width = 16;
  options.window = 3;
  options.negative_samples = 5;
  options.epochs = 120;
  options.batch_size = 64;
  options.learning_rate = 0.05;
  options.seed = 42;

  Embedder embedder(options);
  const auto status = embedder.learn(docs);
  if (!status.ok()) {
    std::cerr << "embed learn failed: " << status.message << "\n";
    return 1;
  }

  const auto nearest = embedder.nearest("cat", 3);
  const auto cat = embedder.vector_of("cat");
  const auto dog = embedder.vector_of("dog");
  const auto car = embedder.vector_of("car");
  if (!nearest.ok() || !cat.ok() || !dog.ok() || !car.ok()) {
    std::cerr << "embed query failed\n";
    return 1;
  }

  std::cout << "== NodeB Static Embeddings ==\n";
  std::cout << "similarity(cat, dog)=" << Embedder::similarity(cat.value(), dog.value()) << "\n";
  std::cout << "similarity(cat, car)=" << Embedder::similarity(cat.value(), car.value()) << "\n";
  std::cout << "nearest(cat):\n";
  for (const auto& [term, score] : nearest.value()) {
    std::cout << "  " << term << " score=" << score << "\n";
  }

  return 0;
}
