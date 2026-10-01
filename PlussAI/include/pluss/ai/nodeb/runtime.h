#pragma once

#include "pluss/ai/nodeb/workflow.h"
#include "pluss/ai/nodej/status.h"
#include "pluss/nn.h"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace pluss::ai::nodeb::runtime {

struct SequenceOptions {
  bool use_lemmas = true;
  bool skip_punct = true;
  bool lowercase = true;
  bool add_bos = false;
  bool add_eos = false;
  bool batch_first = true;
  std::string pad_token = "<pad>";
  std::string unk_token = "<unk>";
  std::string bos_token = "<bos>";
  std::string eos_token = "<eos>";
};

struct SequenceBatch {
  Series token_ids = pluss::series::undefined();
  Series position_ids = pluss::series::undefined();
  Series valid_mask = pluss::series::undefined();
  Series key_padding_mask = pluss::series::undefined();
  Series lengths = pluss::series::undefined();
  std::int64_t batch_size = 0;
  std::int64_t max_length = 0;
  bool batch_first = true;

  [[nodiscard]] bool empty() const noexcept;
  [[nodiscard]] pluss::nn::PackedSequence pack(const Series& values) const;
  [[nodiscard]] Series causal_mask() const;
};

class Indexer {
public:
  explicit Indexer(SequenceOptions options = {});

  [[nodiscard]] pluss::ai::nodej::Status learn(const std::vector<Doc>& docs);
  [[nodiscard]] pluss::ai::nodej::StatusOr<SequenceBatch> batch(const std::vector<Doc>& docs) const;

  [[nodiscard]] const SequenceOptions& options() const noexcept;
  [[nodiscard]] const std::vector<std::string>& vocabulary() const noexcept;
  [[nodiscard]] std::int64_t pad_id() const noexcept;
  [[nodiscard]] std::int64_t unk_id() const noexcept;
  [[nodiscard]] std::int64_t bos_id() const noexcept;
  [[nodiscard]] std::int64_t eos_id() const noexcept;

  [[nodiscard]] std::vector<std::string> dump_state_lines() const;
  [[nodiscard]] pluss::ai::nodej::Status load_state_lines(
      pluss::msharp::RangeView::Of<std::string_view> lines);

private:
  SequenceOptions options_{};
  std::vector<std::string> vocabulary_{};
  std::unordered_map<std::string, std::int64_t> token_to_id_{};
  std::int64_t pad_id_ = -1;
  std::int64_t unk_id_ = -1;
  std::int64_t bos_id_ = -1;
  std::int64_t eos_id_ = -1;
};

struct EmbeddingOptions {
  std::int64_t width = 128;
  bool use_positions = true;
  bool learned_positions = true;
  std::int64_t max_positions = 4096;
  Config cfg = Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
  std::uint64_t seed = 0;
};

class EmbeddingTable {
public:
  explicit EmbeddingTable(EmbeddingOptions options = {});

  void reset(std::int64_t vocab_size,
             std::int64_t pad_id = -1);

  [[nodiscard]] Series forward(const SequenceBatch& batch) const;
  [[nodiscard]] const EmbeddingOptions& options() const noexcept;
  [[nodiscard]] const Series& weights() const noexcept;
  [[nodiscard]] const Series& position_weights() const noexcept;
  [[nodiscard]] std::int64_t vocab_size() const noexcept;
  [[nodiscard]] std::int64_t pad_id() const noexcept;

  [[nodiscard]] std::vector<std::string> dump_state_lines() const;
  [[nodiscard]] pluss::ai::nodej::Status load_state_lines(
      pluss::msharp::RangeView::Of<std::string_view> lines);

private:
  EmbeddingOptions options_{};
  Series weights_ = pluss::series::undefined();
  Series position_weights_ = pluss::series::undefined();
  std::int64_t vocab_size_ = 0;
  std::int64_t pad_id_ = -1;
};

class TransformerTextEncoder {
public:
  explicit TransformerTextEncoder(pluss::nn::TransformerEncoderOptions options = {});

  [[nodiscard]] Series forward(const Series& embedded,
                               const SequenceBatch& batch,
                               bool causal = false) const;
  [[nodiscard]] Series forward(const EmbeddingTable& table,
                               const SequenceBatch& batch,
                               bool causal = false) const;

  [[nodiscard]] pluss::nn::TransformerEncoder& model() noexcept;
  [[nodiscard]] const pluss::nn::TransformerEncoder& model() const noexcept;

private:
  pluss::nn::TransformerEncoder encoder_;
};

class LstmTextEncoder {
public:
  explicit LstmTextEncoder(pluss::nn::LSTMOptions options = {});

  [[nodiscard]] Series forward(const Series& embedded,
                               const SequenceBatch& batch) const;
  [[nodiscard]] Series forward(const EmbeddingTable& table,
                               const SequenceBatch& batch) const;

  [[nodiscard]] pluss::nn::LSTM& model() noexcept;
  [[nodiscard]] const pluss::nn::LSTM& model() const noexcept;

private:
  pluss::nn::LSTM lstm_;
};

[[nodiscard]] Series mean_pool(const Series& encoded,
                               const SequenceBatch& batch);
[[nodiscard]] Series last_pool(const Series& encoded,
                               const SequenceBatch& batch);

} // namespace pluss::ai::nodeb::runtime
