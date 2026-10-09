#include <pluss/nlp.h>

#include <cmath>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace {

namespace nlp = pluss::ai::nlp;
namespace embed = nlp::embed;

std::vector<std::string> static_corpus() {
  std::vector<std::string> text;
  for (int i = 0; i < 40; ++i) {
    text.push_back("cat dog pet animal");
    text.push_back("dog cat animal pet");
    text.push_back("car truck vehicle machine");
    text.push_back("truck car machine vehicle");
  }
  return text;
}

std::vector<std::string> contextual_corpus() {
  std::vector<std::string> text;
  for (int i = 0; i < 40; ++i) {
    text.push_back("the bank raised interest rates");
    text.push_back("the bank increased interest rates");
    text.push_back("the river bank flooded overnight");
    text.push_back("the muddy bank flooded overnight");
  }
  return text;
}

template <class T>
void print_tokens(const T& tokens) {
  if (tokens.empty()) {
    std::cout << "<empty>";
    return;
  }
  for (std::size_t i = 0; i < tokens.size(); ++i) {
    if (i != 0) std::cout << ' ';
    std::cout << tokens[i];
  }
}

} // namespace

int main() {
  std::cout << std::boolalpha << std::fixed << std::setprecision(3);

  nlp::Workflow words{.lang = nlp::Language::English,
                      .stages = {nlp::stage::tokenize(), nlp::stage::lemmatize()}};
  const auto static_docs = words.infer(static_corpus());

  embed::EmbedOptions static_options{};
  static_options.width = 12;
  static_options.window = 3;
  static_options.negative_samples = 4;
  static_options.epochs = 100;
  static_options.batch_size = 64;
  static_options.learning_rate = 0.05;
  static_options.seed = 42;
  static_options.use_lemmas = false;
  static_options.lowercase = false;

  embed::Embedder static_model(static_options);
  std::cout << "== static lifecycle ==\n";
  std::cout << "before ready=" << static_model.ready() << " width=" << static_model.width() << '\n';
  const auto static_status = static_model.learn(static_docs);
  std::cout << "learn ok=" << static_status.ok() << " ready=" << static_model.ready()
            << " width=" << static_model.width() << " vocab=" << static_model.vocabulary().size() << '\n';
  std::cout << "contains cat=" << static_model.contains("cat")
            << " CAT=" << static_model.contains("CAT") << " moon=" << static_model.contains("moon") << '\n';

  const auto cat = static_model.vector_of("cat");
  const auto dog = static_model.vector_of("dog");
  const auto doc = static_model.embed(static_docs.front());
  const auto batch = static_model.embed(std::vector<nlp::Doc>{static_docs[0], static_docs[2]});
  const auto near = static_model.nearest("cat", 3);
  std::cout << "cat vector width=" << cat.value().size() << " doc width=" << doc.value().size()
            << " batch=" << batch.value().size() << 'x' << batch.value().front().size() << '\n';
  std::cout << "nearest cat=";
  for (const auto& [term, score] : near.value()) std::cout << term << ':' << score << ' ';
  std::cout << '\n';

  std::cout << "== cosine cases ==\n";
  std::cout << "same=" << embed::Embedder::similarity({1.0, 0.0}, {1.0, 0.0})
            << " orthogonal=" << embed::Embedder::similarity({1.0, 0.0}, {0.0, 1.0})
            << " opposite=" << embed::Embedder::similarity({1.0, 0.0}, {-1.0, 0.0}) << '\n';
  std::cout << "empty=" << embed::Embedder::similarity({}, {})
            << " mismatch=" << embed::Embedder::similarity({1.0}, {1.0, 2.0})
            << " zero=" << embed::Embedder::similarity({0.0, 0.0}, {1.0, 0.0}) << '\n';
  std::cout << "learned cosine finite="
            << std::isfinite(embed::Embedder::similarity(cat.value(), dog.value())) << '\n';

  std::cout << "== static errors ==\n";
  std::cout << "unknown=" << static_model.vector_of("moon").status().message << '\n';
  std::cout << "negative k=" << static_model.nearest("cat", -1).status().message << '\n';
  std::cout << "empty doc=" << static_model.embed(nlp::Doc{}).status().message << '\n';
  embed::EmbedOptions bad_static = static_options;
  bad_static.learning_rate = std::numeric_limits<double>::quiet_NaN();
  std::cout << "bad options=" << embed::Embedder(bad_static).learn(static_docs).message << '\n';
  bool static_save_rejected = false;
  try {
    embed::Embedder{}.save("./unused.embed");
  } catch (const std::exception&) {
    static_save_rejected = true;
  }
  std::cout << "unready static save rejected=" << static_save_rejected << '\n';

  const auto static_path = std::filesystem::temp_directory_path() / "pluss_embed_docs_static.bin";
  static_model.save(static_path.string());
  const auto static_copy = embed::Embedder::load(static_path.string());
  std::filesystem::remove(static_path);
  std::cout << "static restored=" << static_copy.ready()
            << " exact=" << (static_copy.vector_of("cat").value() == cat.value())
            << " preserves case=" << !static_copy.embed(words.infer("CAT DOG")).ok() << '\n';

  const auto contextual_docs = words.infer(contextual_corpus());
  embed::ContextualEmbedOptions context_options{};
  context_options.width = 16;
  context_options.num_layers = 1;
  context_options.num_heads = 4;
  context_options.feedforward_width = 32;
  context_options.epochs = 100;
  context_options.learning_rate = 0.005;
  context_options.seed = 3;

  embed::ContextualEmbedder context_model(context_options);
  std::cout << "== contextual lifecycle ==\n";
  std::cout << "before ready=" << context_model.ready() << " width=" << context_model.width() << '\n';
  const auto context_status = context_model.learn(contextual_docs);
  std::cout << "learn ok=" << context_status.ok() << " ready=" << context_model.ready()
            << " width=" << context_model.width() << '\n';

  const auto sentence = words.infer("the bank raised interest rates");
  const auto rows = context_model.encode(sentence);
  const auto pooled = context_model.embed(sentence);
  std::cout << "encode rows=" << rows.value().size() << " cols=" << rows.value().front().size()
            << " pooled=" << pooled.value().size()
            << " repeat exact=" << (context_model.encode(sentence).value() == rows.value()) << '\n';
  std::cout << "same token changes with context="
            << (rows.value()[1] != context_model.encode(words.infer("the river bank flooded overnight")).value()[2])
            << '\n';

  const auto prompt = words.infer("the bank raised interest");
  embed::GenerateOptions greedy{.max_new_tokens = 3, .greedy = true};
  const auto greedy_tokens = context_model.generate(prompt, greedy);
  std::cout << "greedy=";
  print_tokens(greedy_tokens.value());
  std::cout << '\n';

  embed::GenerateOptions sampled{.max_new_tokens = 4,
                                 .greedy = false,
                                 .temperature = 0.8,
                                 .top_k = 3,
                                 .seed = 7};
  const auto sample_a = context_model.generate(prompt, sampled);
  const auto sample_b = context_model.generate(prompt, sampled);
  std::cout << "sampled=";
  print_tokens(sample_a.value());
  std::cout << " repeat exact=" << (sample_a.value() == sample_b.value()) << '\n';

  embed::GenerateOptions ignored{.max_new_tokens = 3,
                                 .greedy = true,
                                 .temperature = std::numeric_limits<double>::quiet_NaN(),
                                 .top_k = -1};
  std::cout << "greedy ignores sampling fields="
            << (context_model.generate(prompt, ignored).value() == greedy_tokens.value()) << '\n';
  sampled.temperature = std::numeric_limits<double>::quiet_NaN();
  std::cout << "bad temperature=" << context_model.generate(prompt, sampled).status().message << '\n';
  sampled.temperature = 1.0;
  sampled.top_k = -1;
  std::cout << "bad top_k=" << context_model.generate(prompt, sampled).status().message << '\n';
  bool context_save_rejected = false;
  try {
    embed::ContextualEmbedder{}.save("./unused.ctx");
  } catch (const std::exception&) {
    context_save_rejected = true;
  }
  std::cout << "unready context save rejected=" << context_save_rejected << '\n';

  const auto context_path = std::filesystem::temp_directory_path() / "pluss_embed_docs_context.bin";
  context_model.save(context_path.string());
  const auto context_copy = embed::ContextualEmbedder::load(context_path.string());
  std::filesystem::remove(context_path);
  std::cout << "context restored=" << context_copy.ready()
            << " encode exact=" << (context_copy.encode(sentence).value() == rows.value()) << '\n';
}
