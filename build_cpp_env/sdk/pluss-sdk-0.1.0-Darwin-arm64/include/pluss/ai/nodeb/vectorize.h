#pragma once

#include "pluss/ai/nodeb/workflow.h"
#include "pluss/ai/nodej/text.h"

namespace pluss::ai::nodeb::vectorize {

struct DocMapOptions {
  bool use_lemmas = true;
  bool skip_punct = true;
};

struct CorpusStats {
  std::int64_t doc_count = 0;
  std::int64_t token_count = 0;
  std::int64_t sentence_count = 0;
  std::int64_t phrase_count = 0;
  std::int64_t unique_term_count = 0;
  Series document_token_counts = Series::undefined();
  Series document_unique_term_counts = Series::undefined();
  Note vocabulary = Note::undefined();
  Series document_frequency = Series::undefined();
};

class Mapper {
public:
  explicit Mapper(nodej::LexiconMapOptions map_options = {},
                  DocMapOptions doc_options = {});

  nodej::Status learn(const std::vector<Doc>& docs);
  nodej::Status learn(pluss::msharp::RangeView::Of<std::string_view> texts);
  nodej::Status learn(const std::vector<std::string>& texts);
  nodej::Status learn(const pluss::Note& texts);

  nodej::StatusOr<nodej::FeatureBatch> count_dense(const std::vector<Doc>& docs) const;
  nodej::StatusOr<nodej::SparseTripletMatrix> count_sparse(const std::vector<Doc>& docs) const;
  nodej::StatusOr<nodej::FeatureBatch> tfidf_dense(const std::vector<Doc>& docs,
                                                   const nodej::TfIdfMapOptions& options = {}) const;
  nodej::StatusOr<nodej::SparseTripletMatrix> tfidf_sparse(const std::vector<Doc>& docs,
                                                           const nodej::TfIdfMapOptions& options = {}) const;

  [[nodiscard]] const std::vector<std::string>& vocabulary() const noexcept;
  [[nodiscard]] nodej::LexiconMapperState dump_state() const;
  nodej::Status load_state(const nodej::LexiconMapperState& state);

private:
  nodej::LexiconMapper mapper_{};
  DocMapOptions doc_options_{};
};

class Hasher {
public:
  explicit Hasher(nodej::HashMapOptions map_options = {},
                  DocMapOptions doc_options = {});

  nodej::StatusOr<nodej::FeatureBatch> dense(const std::vector<Doc>& docs) const;
  nodej::StatusOr<nodej::SparseTripletMatrix> sparse(const std::vector<Doc>& docs) const;
  nodej::StatusOr<nodej::FeatureBatch> dense(pluss::msharp::RangeView::Of<std::string_view> texts) const;
  nodej::StatusOr<nodej::SparseTripletMatrix> sparse(pluss::msharp::RangeView::Of<std::string_view> texts) const;

private:
  nodej::HashMapper mapper_{};
  DocMapOptions doc_options_{};
};

[[nodiscard]] CorpusStats describe(const std::vector<Doc>& docs,
                                   DocMapOptions options = {});

} // namespace pluss::ai::nodeb::vectorize
