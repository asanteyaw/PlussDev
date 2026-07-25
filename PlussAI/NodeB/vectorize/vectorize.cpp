#include "pluss/ai/nodeb/vectorize.h"

#include "pluss/codem/series_factory.h"

#include <algorithm>
#include <unordered_map>

namespace pluss::ai::nodeb::vectorize {

namespace {

[[nodiscard]] std::vector<std::string> docs_to_feature_text(const std::vector<Doc>& docs,
                                                            const DocMapOptions& options) {
  std::vector<std::string> out;
  out.reserve(docs.size());
  for (const auto& doc : docs) {
    std::string text;
    for (const auto& token : doc.tokens) {
      if (options.skip_punct && token.is_punct) continue;
      const std::string_view piece =
          (options.use_lemmas && !token.lemma.empty()) ? std::string_view(token.lemma)
                                                       : std::string_view(token.orth);
      if (piece.empty()) continue;
      if (!text.empty()) text.push_back(' ');
      text.append(piece.data(), piece.size());
    }
    if (text.empty()) text = doc.text;
    out.push_back(std::move(text));
  }
  return out;
}

[[nodiscard]] std::vector<std::string> texts_to_owned(pluss::msharp::RangeView::Of<std::string_view> texts) {
  std::vector<std::string> out;
  out.reserve(texts.size());
  for (std::size_t i = 0; i < texts.size(); ++i) {
    out.emplace_back(texts[i]);
  }
  return out;
}

} // namespace

Mapper::Mapper(nodej::LexiconMapOptions map_options,
               DocMapOptions doc_options)
    : mapper_(map_options),
      doc_options_(doc_options) {}

nodej::Status Mapper::learn(const std::vector<Doc>& docs) {
  PLUSS_CALL_GUARD();
  const auto feature_docs = docs_to_feature_text(docs, doc_options_);
  return mapper_.learn(pluss::msharp::RangeView::Of<std::string>(feature_docs));
}

nodej::Status Mapper::learn(pluss::msharp::RangeView::Of<std::string_view> texts) {
  PLUSS_CALL_GUARD();
  auto owned = texts_to_owned(texts);
  return mapper_.learn(pluss::msharp::RangeView::Of<std::string>(owned));
}

nodej::Status Mapper::learn(const std::vector<std::string>& texts) {
  PLUSS_CALL_GUARD();
  return mapper_.learn(pluss::msharp::RangeView::Of<std::string>(texts));
}

nodej::Status Mapper::learn(const pluss::Note& texts) {
  PLUSS_CALL_GUARD();
  std::vector<std::string> owned;
  owned.reserve(static_cast<std::size_t>(texts.numel()));
  for (std::int64_t i = 0; i < texts.numel(); ++i) {
    owned.emplace_back(texts.at(i));
  }
  return mapper_.learn(pluss::msharp::RangeView::Of<std::string>(owned));
}

nodej::StatusOr<nodej::FeatureBatch> Mapper::count_dense(const std::vector<Doc>& docs) const {
  PLUSS_CALL_GUARD();
  const auto feature_docs = docs_to_feature_text(docs, doc_options_);
  return mapper_.map_dense(pluss::msharp::RangeView::Of<std::string>(feature_docs));
}

nodej::StatusOr<nodej::SparseTripletMatrix> Mapper::count_sparse(const std::vector<Doc>& docs) const {
  PLUSS_CALL_GUARD();
  const auto feature_docs = docs_to_feature_text(docs, doc_options_);
  return mapper_.map_sparse(pluss::msharp::RangeView::Of<std::string>(feature_docs));
}

nodej::StatusOr<nodej::FeatureBatch> Mapper::tfidf_dense(const std::vector<Doc>& docs,
                                                         const nodej::TfIdfMapOptions& options) const {
  PLUSS_CALL_GUARD();
  const auto feature_docs = docs_to_feature_text(docs, doc_options_);
  return mapper_.map_tfidf_dense(pluss::msharp::RangeView::Of<std::string>(feature_docs), options);
}

nodej::StatusOr<nodej::SparseTripletMatrix> Mapper::tfidf_sparse(const std::vector<Doc>& docs,
                                                                 const nodej::TfIdfMapOptions& options) const {
  PLUSS_CALL_GUARD();
  const auto feature_docs = docs_to_feature_text(docs, doc_options_);
  return mapper_.map_tfidf_sparse(pluss::msharp::RangeView::Of<std::string>(feature_docs), options);
}

const std::vector<std::string>& Mapper::vocabulary() const noexcept {
  return mapper_.vocabulary();
}

nodej::LexiconMapperState Mapper::dump_state() const {
  PLUSS_CALL_GUARD();
  return mapper_.dump_state();
}

nodej::Status Mapper::load_state(const nodej::LexiconMapperState& state) {
  PLUSS_CALL_GUARD();
  return mapper_.load_state(state);
}

Hasher::Hasher(nodej::HashMapOptions map_options,
               DocMapOptions doc_options)
    : mapper_(map_options),
      doc_options_(doc_options) {}

nodej::StatusOr<nodej::FeatureBatch> Hasher::dense(const std::vector<Doc>& docs) const {
  PLUSS_CALL_GUARD();
  const auto feature_docs = docs_to_feature_text(docs, doc_options_);
  return mapper_.map_dense(pluss::msharp::RangeView::Of<std::string>(feature_docs));
}

nodej::StatusOr<nodej::SparseTripletMatrix> Hasher::sparse(const std::vector<Doc>& docs) const {
  PLUSS_CALL_GUARD();
  const auto feature_docs = docs_to_feature_text(docs, doc_options_);
  return mapper_.map_sparse(pluss::msharp::RangeView::Of<std::string>(feature_docs));
}

nodej::StatusOr<nodej::FeatureBatch> Hasher::dense(pluss::msharp::RangeView::Of<std::string_view> texts) const {
  PLUSS_CALL_GUARD();
  auto owned = texts_to_owned(texts);
  return mapper_.map_dense(pluss::msharp::RangeView::Of<std::string>(owned));
}

nodej::StatusOr<nodej::SparseTripletMatrix> Hasher::sparse(pluss::msharp::RangeView::Of<std::string_view> texts) const {
  PLUSS_CALL_GUARD();
  auto owned = texts_to_owned(texts);
  return mapper_.map_sparse(pluss::msharp::RangeView::Of<std::string>(owned));
}

CorpusStats describe(const std::vector<Doc>& docs,
                     DocMapOptions options) {
  PLUSS_CALL_GUARD();
  CorpusStats out{};
  out.doc_count = static_cast<std::int64_t>(docs.size());
  out.sentence_count = 0;
  out.phrase_count = 0;

  std::vector<double> doc_token_counts{};
  std::vector<double> doc_unique_counts{};
  doc_token_counts.reserve(docs.size());
  doc_unique_counts.reserve(docs.size());

  std::unordered_map<std::string, std::int64_t> df{};
  for (const auto& doc : docs) {
    out.sentence_count += static_cast<std::int64_t>(doc.sentences.size());
    out.phrase_count += static_cast<std::int64_t>(doc.phrases.size());

    std::vector<std::string> terms{};
    terms.reserve(doc.tokens.size());
    for (const auto& token : doc.tokens) {
      if (options.skip_punct && token.is_punct) continue;
      const std::string_view piece =
          (options.use_lemmas && !token.lemma.empty()) ? std::string_view(token.lemma)
                                                       : std::string_view(token.orth);
      if (piece.empty()) continue;
      terms.emplace_back(piece);
    }
    out.token_count += static_cast<std::int64_t>(terms.size());
    doc_token_counts.push_back(static_cast<double>(terms.size()));

    std::sort(terms.begin(), terms.end());
    const auto unique_end = std::unique(terms.begin(), terms.end());
    const auto unique_count = static_cast<std::int64_t>(std::distance(terms.begin(), unique_end));
    doc_unique_counts.push_back(static_cast<double>(unique_count));
    for (auto it = terms.begin(); it != unique_end; ++it) {
      ++df[*it];
    }
  }

  std::vector<std::pair<std::string, std::int64_t>> ranked{};
  ranked.reserve(df.size());
  for (const auto& kv : df) ranked.emplace_back(kv.first, kv.second);
  std::sort(ranked.begin(), ranked.end(), [](const auto& a, const auto& b) {
    if (a.second != b.second) return a.second > b.second;
    return a.first < b.first;
  });

  out.unique_term_count = static_cast<std::int64_t>(ranked.size());
  std::vector<std::string> vocab{};
  std::vector<double> doc_freq{};
  vocab.reserve(ranked.size());
  doc_freq.reserve(ranked.size());
  for (const auto& [term, freq] : ranked) {
    vocab.push_back(term);
    doc_freq.push_back(static_cast<double>(freq));
  }

  out.document_token_counts = pluss::make_series(std::move(doc_token_counts));
  out.document_unique_term_counts = pluss::make_series(std::move(doc_unique_counts));
  out.vocabulary = pluss::make_note(std::move(vocab));
  out.document_frequency = pluss::make_series(std::move(doc_freq));
  return out;
}

} // namespace pluss::ai::nodeb::vectorize
