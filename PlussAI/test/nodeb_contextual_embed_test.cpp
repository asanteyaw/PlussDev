#include <pluss/nlp.h>

#include <filesystem>
#include <iostream>
#include <optional>
#include <vector>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

namespace {

using namespace pluss::ai::nodeb;
using pluss::ai::nodeb::embed::ContextualEmbedder;
using pluss::ai::nodeb::embed::ContextualEmbedOptions;

// Same corpus structure that made the static Embedder test reliable: two
// clearly separated clusters, repeated for signal. Here the clusters share
// the ambiguous word "bank" but differ in what precedes/follows it, so a
// causal (left-to-right) encoder has real preceding-context signal to learn
// from for the next-token objective.
[[nodiscard]] std::vector<std::string> make_corpus() {
  std::vector<std::string> docs{};
  for (int i = 0; i < 50; ++i) {
    docs.push_back("the bank raised interest rates");
    docs.push_back("the bank increased interest rates");
    docs.push_back("the river bank flooded overnight");
    docs.push_back("the muddy bank flooded overnight");
  }
  return docs;
}

[[nodiscard]] std::optional<std::size_t> index_of(const Doc& doc, std::string_view lemma) {
  for (std::size_t i = 0; i < doc.tokens.size(); ++i) {
    if (doc.tokens[i].lemma == lemma || doc.tokens[i].orth == lemma) return i;
  }
  return std::nullopt;
}

bool run_nodeb_contextual_embed_checks(std::ostream* err = nullptr) {
  Workflow tokenizer{.lang = Language::English, .stages = {stage::tokenize(), stage::lemmatize()}};
  const auto docs = tokenizer.infer(make_corpus());

  ContextualEmbedOptions options{};
  options.width = 24;
  options.num_layers = 2;
  options.num_heads = 4;
  options.feedforward_width = 48;
  options.epochs = 200;
  options.learning_rate = 5e-3;
  options.seed = 3;

  ContextualEmbedder model(options);

  // Error paths before training.
  if (model.ready()) {
    if (err) *err << "nodeb contextual embedder should not report ready before learn()\n";
    return false;
  }
  if (model.embed(docs.front()).ok()) {
    if (err) *err << "nodeb contextual embedder embed() should fail before learn()\n";
    return false;
  }

  const auto status = model.learn(docs);
  if (!status.ok()) {
    if (err) *err << "nodeb contextual embedder should train successfully: " << status.message << "\n";
    return false;
  }
  if (!model.ready() || model.width() != options.width) {
    if (err) *err << "nodeb contextual embedder should report ready with the configured width after learn()\n";
    return false;
  }

  // Two "bank" (financial) sentences and two "bank" (geographic) sentences.
  const auto raised = tokenizer.infer("the bank raised interest rates");
  const auto increased = tokenizer.infer("the bank increased interest rates");
  const auto river = tokenizer.infer("the river bank flooded overnight");
  const auto muddy = tokenizer.infer("the muddy bank flooded overnight");

  const auto raised_idx = index_of(raised, "bank");
  const auto increased_idx = index_of(increased, "bank");
  const auto river_idx = index_of(river, "bank");
  const auto muddy_idx = index_of(muddy, "bank");
  if (!raised_idx || !increased_idx || !river_idx || !muddy_idx) {
    if (err) *err << "nodeb contextual embedder test fixture should contain 'bank' in every sentence\n";
    return false;
  }

  const auto raised_rows = model.encode(raised);
  const auto increased_rows = model.encode(increased);
  const auto river_rows = model.encode(river);
  const auto muddy_rows = model.encode(muddy);
  if (!raised_rows.ok() || !increased_rows.ok() || !river_rows.ok() || !muddy_rows.ok()) {
    if (err) *err << "nodeb contextual embedder encode() should succeed on trained-domain sentences\n";
    return false;
  }
  if (raised_rows.value().size() != raised.tokens.size() ||
      raised_rows.value().front().size() != static_cast<std::size_t>(options.width)) {
    if (err) *err << "nodeb contextual embedder encode() should return one width-sized row per token\n";
    return false;
  }

  const auto& raised_bank = raised_rows.value()[*raised_idx];
  const auto& increased_bank = increased_rows.value()[*increased_idx];
  const auto& river_bank = river_rows.value()[*river_idx];
  const auto& muddy_bank = muddy_rows.value()[*muddy_idx];

  const double financial_vs_financial =
      pluss::ai::nodeb::embed::Embedder::similarity(raised_bank, increased_bank);
  const double geo_vs_geo = pluss::ai::nodeb::embed::Embedder::similarity(river_bank, muddy_bank);
  const double financial_vs_geo = pluss::ai::nodeb::embed::Embedder::similarity(raised_bank, river_bank);
  const double geo_vs_financial = pluss::ai::nodeb::embed::Embedder::similarity(muddy_bank, raised_bank);

  const double same_sense_min = std::min(financial_vs_financial, geo_vs_geo);
  const double cross_sense_max = std::max(financial_vs_geo, geo_vs_financial);
  if (same_sense_min <= cross_sense_max) {
    if (err) {
      *err << "nodeb contextual embedder should give 'bank' a more similar vector across same-sense "
              "contexts than across different-sense contexts:\n"
           << "  financial-vs-financial=" << financial_vs_financial << " geo-vs-geo=" << geo_vs_geo
           << " (same-sense)\n"
           << "  financial-vs-geo=" << financial_vs_geo << " geo-vs-financial=" << geo_vs_financial
           << " (cross-sense)\n";
    }
    return false;
  }

  // Determinism: re-encoding the same document must give identical vectors.
  const auto raised_rows_again = model.encode(raised);
  if (!raised_rows_again.ok() || raised_rows_again.value() != raised_rows.value()) {
    if (err) *err << "nodeb contextual embedder encode() should be deterministic for a fixed model\n";
    return false;
  }

  // Structural sanity: within one sentence, different tokens generally get
  // different vectors (this is not a static per-word lookup).
  if (raised_rows.value()[0] == raised_rows.value()[*raised_idx]) {
    if (err) *err << "nodeb contextual embedder should not collapse distinct tokens to the same vector\n";
    return false;
  }

  const auto pooled = model.embed(raised);
  if (!pooled.ok() || pooled.value().size() != static_cast<std::size_t>(options.width)) {
    if (err) *err << "nodeb contextual embedder embed() should mean-pool encode() into one document vector\n";
    return false;
  }

  // Save/load round trip must restore a model that encodes identically to
  // the one that was saved -- same indexer vocabulary, embedding table,
  // Transformer weights, and output head.
  const auto tmp = std::filesystem::temp_directory_path() / "nodeb_contextual_embed_roundtrip.bin";
  model.save(tmp.string());
  const auto restored = ContextualEmbedder::load(tmp.string());
  std::filesystem::remove(tmp);

  if (!restored.ready() || restored.width() != options.width) {
    if (err) *err << "nodeb contextual embedder save/load should restore a ready, correctly-shaped model\n";
    return false;
  }
  const auto restored_raised_rows = restored.encode(raised);
  if (!restored_raised_rows.ok() || restored_raised_rows.value() != raised_rows.value()) {
    if (err) *err << "nodeb contextual embedder save/load should preserve encode() output exactly\n";
    return false;
  }

  // generate(): greedy decoding must be deterministic and respect the
  // requested token budget.
  const auto bank_raised_interest = tokenizer.infer("the bank raised interest");
  embed::GenerateOptions greedy_options{};
  greedy_options.max_new_tokens = 3;
  const auto greedy_once = model.generate(bank_raised_interest, greedy_options);
  const auto greedy_twice = model.generate(bank_raised_interest, greedy_options);
  if (!greedy_once.ok() || !greedy_twice.ok()) {
    if (err) *err << "nodeb contextual embedder generate() should succeed for a trained-domain prompt\n";
    return false;
  }
  if (greedy_once.value() != greedy_twice.value()) {
    if (err) *err << "nodeb contextual embedder generate() greedy decoding should be deterministic\n";
    return false;
  }
  if (greedy_once.value().empty() || greedy_once.value().size() > 3) {
    if (err) {
      *err << "nodeb contextual embedder generate() should produce between 1 and max_new_tokens tokens, got "
           << greedy_once.value().size() << "\n";
    }
    return false;
  }

  // Every training sentence completes "... raised interest" with "rates"
  // ("rate" once lemmatized, since options.use_lemmas is on and the model's
  // vocabulary is lemma-based), with no exceptions in the corpus -- greedy
  // decoding from that exact four-token prefix should recover it.
  if (greedy_once.value().front() != "rate") {
    if (err) {
      *err << "nodeb contextual embedder generate() should learn 'the bank raised interest' -> 'rate'; got '"
           << greedy_once.value().front() << "'\n";
    }
    return false;
  }

  // Unconditional generation (empty prompt, seeded only with <bos>) must
  // not crash and must still respect the token budget.
  const auto unconditional = model.generate(Doc{}, greedy_options);
  if (!unconditional.ok() || unconditional.value().size() > 3) {
    if (err) *err << "nodeb contextual embedder generate() should handle an empty prompt\n";
    return false;
  }

  // Sampling must be reproducible for a fixed seed and respect top_k/
  // temperature without erroring.
  embed::GenerateOptions sample_options{};
  sample_options.max_new_tokens = 4;
  sample_options.greedy = false;
  sample_options.temperature = 0.8;
  sample_options.top_k = 3;
  sample_options.seed = 7;
  const auto sampled_once = model.generate(bank_raised_interest, sample_options);
  const auto sampled_twice = model.generate(bank_raised_interest, sample_options);
  if (!sampled_once.ok() || !sampled_twice.ok()) {
    if (err) *err << "nodeb contextual embedder generate() sampling should succeed\n";
    return false;
  }
  if (sampled_once.value() != sampled_twice.value()) {
    if (err) *err << "nodeb contextual embedder generate() sampling should be reproducible for a fixed seed\n";
    return false;
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("nodeb contextual embed tranche", "[nodeb][embed][contextual]") {
  if (!run_nodeb_contextual_embed_checks(&std::cerr)) {
    FAIL("nodeb contextual embed checks failed");
  }
}
#else
int main() {
  if (!run_nodeb_contextual_embed_checks(&std::cerr)) {
    return 1;
  }
  std::cout << "nodeb contextual embed checks passed\n";
  return 0;
}
#endif
