#include <pluss/nlp.h>

#include <algorithm>
#include <iostream>
#include <string_view>
#include <vector>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

namespace {

using namespace pluss::ai::nodeb;

pluss::msharp::RangeView::Of<std::string_view> as_views(
    const std::vector<std::string>& lines,
    std::vector<std::string_view>& views) {
  views.clear();
  views.reserve(lines.size());
  for (const auto& line : lines) {
    views.push_back(line);
  }
  return pluss::msharp::RangeView::Of<std::string_view>(views);
}

bool run_nodeb_sequence_runtime_checks(std::ostream* err = nullptr) {
  Workflow nlp{
      .lang = Language::English,
      .stages = {
          stage::tokenize(),
          stage::lemmatize(),
      },
  };
  const auto docs = nlp.infer(std::vector<std::string>{
      "New York builds APIs.",
      "APIs build systems.",
  });

  runtime::Indexer indexer(runtime::SequenceOptions{
      .use_lemmas = true,
      .skip_punct = true,
      .lowercase = true,
      .add_bos = true,
      .add_eos = true,
      .batch_first = true,
  });
  if (!indexer.learn(docs).ok()) {
    if (err) *err << "nodeb runtime Indexer::learn should succeed on Doc batches\n";
    return false;
  }

  const auto& vocab = indexer.vocabulary();
  if (std::find(vocab.begin(), vocab.end(), "build") == vocab.end() ||
      std::find(vocab.begin(), vocab.end(), "api") == vocab.end() ||
      indexer.pad_id() != 0 ||
      indexer.unk_id() != 1) {
    if (err) *err << "nodeb runtime Indexer should learn lemma-based vocabulary with stable special ids\n";
    return false;
  }

  auto batch_or = indexer.batch(docs);
  if (!batch_or.ok()) {
    if (err) *err << "nodeb runtime Indexer::batch should succeed after learn\n";
    return false;
  }
  const auto& batch = batch_or.value();
  if (batch.batch_size != 2 ||
      batch.max_length != 6 ||
      batch.lengths.host<std::int64_t>() != std::vector<std::int64_t>({6, 5}) ||
      batch.token_ids.sizes() != std::vector<std::int64_t>({2, 6}) ||
      batch.valid_mask.to(pluss::pCPU, pluss::ScalarType::Int64).sum().item<std::int64_t>() != 11 ||
      batch.key_padding_mask.to(pluss::pCPU, pluss::ScalarType::Int64).sum().item<std::int64_t>() != 1) {
    if (err) *err << "nodeb runtime SequenceBatch should preserve batch/length/mask structure\n";
    return false;
  }

  const auto index_state = indexer.dump_state_lines();
  runtime::Indexer restored_indexer;
  std::vector<std::string_view> index_views;
  if (!restored_indexer.load_state_lines(as_views(index_state, index_views)).ok()) {
    if (err) *err << "nodeb runtime Indexer::load_state_lines should restore serialized state\n";
    return false;
  }
  auto restored_batch_or = restored_indexer.batch(docs);
  if (!restored_batch_or.ok() ||
      !restored_batch_or.value().token_ids.eq(batch.token_ids).all().item<bool>()) {
    if (err) *err << "nodeb runtime restored Indexer should reproduce token id batches\n";
    return false;
  }

  runtime::EmbeddingTable table(runtime::EmbeddingOptions{
      .width = 8,
      .use_positions = true,
      .learned_positions = true,
      .max_positions = 16,
      .cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pCPU),
      .seed = 23ULL,
  });
  table.reset(static_cast<std::int64_t>(vocab.size()), indexer.pad_id());
  const pluss::Series embedded = table.forward(batch);
  if (embedded.sizes() != std::vector<std::int64_t>({2, 6, 8})) {
    if (err) *err << "nodeb runtime EmbeddingTable should emit [batch, seq, width] tensors\n";
    return false;
  }
  const auto padded_tail = embedded.select(0, 1).select(0, 5).abs().sum().item<double>();
  if (padded_tail > 1e-9) {
    if (err) *err << "nodeb runtime EmbeddingTable should zero padded sequence positions after lookup\n";
    return false;
  }

  const auto emb_state = table.dump_state_lines();
  runtime::EmbeddingTable restored_table;
  std::vector<std::string_view> emb_views;
  if (!restored_table.load_state_lines(as_views(emb_state, emb_views)).ok()) {
    if (err) *err << "nodeb runtime EmbeddingTable::load_state_lines should restore serialized embeddings\n";
    return false;
  }
  const auto restored_embedded = restored_table.forward(batch);
  const double emb_diff = embedded.sub(restored_embedded).abs().max().item<double>();
  if (emb_diff > 1e-9) {
    if (err) *err << "nodeb runtime restored embeddings should reproduce the same lookup output\n";
    return false;
  }

  runtime::TransformerTextEncoder encoder(pluss::nn::TransformerEncoderOptions{
      .d_model = 8,
      .nhead = 2,
      .num_layers = 2,
      .dim_feedforward = 16,
      .dropout = 0.0,
      .activation = pluss::nn::TransformerActivation::ReLU,
      .layer_norm_eps = 1e-5,
      .batch_first = true,
      .norm_first = false,
      .bias = true,
      .final_norm = true,
      .cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pCPU),
      .seed = 41ULL,
      .training = true,
  });
  const auto encoded = encoder.forward(embedded, batch, false);
  if (encoded.sizes() != std::vector<std::int64_t>({2, 6, 8})) {
    if (err) *err << "nodeb runtime TransformerTextEncoder should preserve [batch, seq, d_model] shape\n";
    return false;
  }
  const auto pooled = runtime::mean_pool(encoded, batch);
  if (pooled.sizes() != std::vector<std::int64_t>({2, 8})) {
    if (err) *err << "nodeb runtime mean_pool should reduce sequence tensors to one row per document\n";
    return false;
  }
  const auto last = runtime::last_pool(encoded, batch);
  if (last.sizes() != std::vector<std::int64_t>({2, 8})) {
    if (err) *err << "nodeb runtime last_pool should gather the final valid token state per document\n";
    return false;
  }

  runtime::LstmTextEncoder lstm(pluss::nn::LSTMOptions{
      .input_size = 8,
      .hidden_size = 6,
      .num_layers = 1,
      .bias = true,
      .batch_first = true,
      .dropout = 0.0,
      .bidirectional = true,
      .split_bias = false,
      .proj_size = 0,
      .cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pCPU),
      .seed = 59ULL,
      .training = true,
  });
  const auto lstm_out = lstm.forward(embedded, batch);
  if (lstm_out.sizes() != std::vector<std::int64_t>({2, 6, 12})) {
    if (err) *err << "nodeb runtime LstmTextEncoder should restore padded bidirectional sequence output\n";
    return false;
  }
  const auto lstm_last = runtime::last_pool(lstm_out, batch);
  if (lstm_last.sizes() != std::vector<std::int64_t>({2, 12})) {
    if (err) *err << "nodeb runtime last_pool should work on bidirectional LSTM outputs too\n";
    return false;
  }

  runtime::Indexer time_major_indexer(runtime::SequenceOptions{
      .use_lemmas = true,
      .skip_punct = true,
      .lowercase = true,
      .add_bos = false,
      .add_eos = false,
      .batch_first = false,
  });
  if (!time_major_indexer.learn(docs).ok()) {
    if (err) *err << "nodeb runtime time-major Indexer::learn should succeed\n";
    return false;
  }
  auto time_major_batch_or = time_major_indexer.batch(docs);
  if (!time_major_batch_or.ok()) {
    if (err) *err << "nodeb runtime time-major Indexer::batch should succeed\n";
    return false;
  }
  const auto& time_major_batch = time_major_batch_or.value();
  if (time_major_batch.token_ids.sizes() != std::vector<std::int64_t>({4, 2}) ||
      time_major_batch.lengths.host<std::int64_t>() != std::vector<std::int64_t>({4, 3})) {
    if (err) *err << "nodeb runtime should support time-major sequence layout too\n";
    return false;
  }

  runtime::EmbeddingTable time_major_table(runtime::EmbeddingOptions{
      .width = 8,
      .use_positions = true,
      .learned_positions = false,
      .max_positions = 16,
      .cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pCPU),
      .seed = 67ULL,
  });
  time_major_table.reset(
      static_cast<std::int64_t>(time_major_indexer.vocabulary().size()),
      time_major_indexer.pad_id());
  const auto time_major_embedded = time_major_table.forward(time_major_batch);
  if (time_major_embedded.sizes() != std::vector<std::int64_t>({4, 2, 8})) {
    if (err) *err << "nodeb runtime time-major embeddings should emit [seq, batch, width] tensors\n";
    return false;
  }

  runtime::TransformerTextEncoder time_major_encoder(pluss::nn::TransformerEncoderOptions{
      .d_model = 8,
      .nhead = 2,
      .num_layers = 1,
      .dim_feedforward = 16,
      .dropout = 0.0,
      .activation = pluss::nn::TransformerActivation::GELU,
      .layer_norm_eps = 1e-5,
      .batch_first = false,
      .norm_first = true,
      .bias = true,
      .final_norm = true,
      .cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pCPU),
      .seed = 71ULL,
      .training = true,
  });
  const auto time_major_encoded = time_major_encoder.forward(time_major_embedded, time_major_batch, false);
  if (time_major_encoded.sizes() != std::vector<std::int64_t>({4, 2, 8}) ||
      runtime::mean_pool(time_major_encoded, time_major_batch).sizes() != std::vector<std::int64_t>({2, 8})) {
    if (err) *err << "nodeb runtime transformer path should also work for time-major batches\n";
    return false;
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("nodeb sequence runtime tranche", "[nodeb][runtime]") {
  if (!run_nodeb_sequence_runtime_checks(&std::cerr)) {
    FAIL("nodeb sequence runtime checks failed");
  }
}
#else
int main() {
  if (!run_nodeb_sequence_runtime_checks(&std::cerr)) {
    return 1;
  }
  std::cout << "nodeb sequence runtime checks passed\n";
  return 0;
}
#endif
