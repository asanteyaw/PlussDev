#include <pluss/nlp.h>

#include <filesystem>
#include <iostream>
#include <vector>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

namespace {

using namespace pluss::ai::nodeb;
using pluss::ai::nodeb::embed::EmbedOptions;
using pluss::ai::nodeb::embed::Embedder;

// A small corpus with two clearly separated co-occurrence clusters (pets vs
// vehicles), repeated enough times that skip-gram-with-negative-sampling
// has a real signal to learn from even at a tiny scale.
[[nodiscard]] std::vector<std::string> make_corpus() {
  std::vector<std::string> docs{};
  for (int i = 0; i < 60; ++i) {
    docs.push_back("cat dog pet animal");
    docs.push_back("dog cat animal pet");
    docs.push_back("car truck vehicle machine");
    docs.push_back("truck car machine vehicle");
  }
  return docs;
}

bool run_nodeb_embed_checks(std::ostream* err = nullptr) {
  Workflow tokenizer{.lang = Language::English, .stages = {stage::tokenize()}};
  const auto docs = tokenizer.infer(make_corpus());

  EmbedOptions options{};
  options.width = 16;
  options.window = 3;
  options.negative_samples = 5;
  options.epochs = 150;
  options.batch_size = 64;
  options.learning_rate = 0.05;
  options.seed = 42;

  Embedder embedder(options);
  const auto learn_status = embedder.learn(docs);
  if (!learn_status.ok()) {
    if (err) *err << "nodeb embedder should train successfully on a valid corpus: " << learn_status.message << "\n";
    return false;
  }
  if (!embedder.ready() || embedder.width() != options.width) {
    if (err) *err << "nodeb embedder should report itself ready with the configured width after learn()\n";
    return false;
  }
  for (const auto* term : {"cat", "dog", "car", "truck"}) {
    if (!embedder.contains(term)) {
      if (err) *err << "nodeb embedder vocabulary should contain '" << term << "'\n";
      return false;
    }
  }

  const auto cat = embedder.vector_of("cat");
  const auto dog = embedder.vector_of("dog");
  const auto car = embedder.vector_of("car");
  const auto truck = embedder.vector_of("truck");
  if (!cat.ok() || !dog.ok() || !car.ok() || !truck.ok()) {
    if (err) *err << "nodeb embedder vector_of should succeed for in-vocabulary tokens\n";
    return false;
  }

  const double cat_dog = Embedder::similarity(cat.value(), dog.value());
  const double car_truck = Embedder::similarity(car.value(), truck.value());
  const double cat_car = Embedder::similarity(cat.value(), car.value());
  const double cat_truck = Embedder::similarity(cat.value(), truck.value());
  const double dog_car = Embedder::similarity(dog.value(), car.value());
  const double dog_truck = Embedder::similarity(dog.value(), truck.value());

  const double within_cluster_min = std::min(cat_dog, car_truck);
  const double cross_cluster_max = std::max({cat_car, cat_truck, dog_car, dog_truck});
  if (within_cluster_min <= cross_cluster_max) {
    if (err) {
      *err << "nodeb embedder should learn tighter within-cluster similarity than cross-cluster:\n"
           << "  cat-dog=" << cat_dog << " car-truck=" << car_truck << " (within-cluster)\n"
           << "  cat-car=" << cat_car << " cat-truck=" << cat_truck
           << " dog-car=" << dog_car << " dog-truck=" << dog_truck << " (cross-cluster)\n";
    }
    return false;
  }

  const auto nearest_to_cat = embedder.nearest("cat", 1);
  if (!nearest_to_cat.ok() || nearest_to_cat.value().empty() || nearest_to_cat.value().front().first != "dog") {
    if (err) {
      *err << "nodeb embedder nearest('cat') should rank 'dog' first; got ";
      if (nearest_to_cat.ok() && !nearest_to_cat.value().empty()) {
        *err << nearest_to_cat.value().front().first;
      } else {
        *err << "<error>";
      }
      *err << "\n";
    }
    return false;
  }

  if (embedder.vector_of("spaceship").ok()) {
    if (err) *err << "nodeb embedder vector_of should fail for out-of-vocabulary tokens\n";
    return false;
  }

  const auto doc_embedding = embedder.embed(docs.front());
  if (!doc_embedding.ok() || doc_embedding.value().size() != static_cast<std::size_t>(options.width)) {
    if (err) *err << "nodeb embedder embed(doc) should pool a document-level vector of the configured width\n";
    return false;
  }

  // min_count should filter out a hapax legomenon.
  auto with_rare_word = make_corpus();
  with_rare_word.push_back("a lonely giraffe wandered by");
  const auto rare_docs = tokenizer.infer(with_rare_word);
  EmbedOptions filtered_options = options;
  filtered_options.min_count = 2;
  Embedder filtered_embedder(filtered_options);
  if (!filtered_embedder.learn(rare_docs).ok()) {
    if (err) *err << "nodeb embedder should still train successfully with min_count filtering active\n";
    return false;
  }
  if (filtered_embedder.contains("giraffe")) {
    if (err) *err << "nodeb embedder min_count should filter out words seen only once\n";
    return false;
  }

  // Error paths.
  Embedder empty_embedder(options);
  if (empty_embedder.learn({}).ok()) {
    if (err) *err << "nodeb embedder learn() should reject an empty document set\n";
    return false;
  }
  if (embedder.embed(Doc{}).ok()) {
    if (err) *err << "nodeb embedder embed() should fail for a document with no in-vocabulary tokens\n";
    return false;
  }
  Embedder untrained(options);
  if (untrained.vector_of("cat").ok() || untrained.ready()) {
    if (err) *err << "nodeb embedder should report not-ready and fail lookups before learn()\n";
    return false;
  }

  // Save/load round trip must preserve vectors and rankings exactly.
  const auto tmp = std::filesystem::temp_directory_path() / "nodeb_embed_roundtrip.bin";
  embedder.save(tmp.string());
  const auto restored = Embedder::load(tmp.string());
  std::filesystem::remove(tmp);

  if (!restored.ready() || restored.width() != options.width || !restored.contains("cat")) {
    if (err) *err << "nodeb embedder save/load should restore a ready, correctly-shaped model\n";
    return false;
  }
  const auto restored_cat = restored.vector_of("cat");
  if (!restored_cat.ok() || restored_cat.value() != cat.value()) {
    if (err) *err << "nodeb embedder save/load should preserve learned vectors exactly\n";
    return false;
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("nodeb embed tranche", "[nodeb][embed]") {
  if (!run_nodeb_embed_checks(&std::cerr)) {
    FAIL("nodeb embed checks failed");
  }
}
#else
int main() {
  if (!run_nodeb_embed_checks(&std::cerr)) {
    return 1;
  }
  std::cout << "nodeb embed checks passed\n";
  return 0;
}
#endif
