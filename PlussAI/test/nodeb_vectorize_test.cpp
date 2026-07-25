#include <pluss/nlp.h>

#include <algorithm>
#include <iostream>
#include <vector>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

namespace {

using namespace pluss::ai::nodeb;

bool run_nodeb_vectorize_checks(std::ostream* err = nullptr) {
  Workflow nlp{
      .lang = Language::English,
      .stages = {
          stage::tokenize(),
          stage::sentencize(),
          stage::lemmatize(),
      },
  };
  const auto docs = nlp.infer(std::vector<std::string>{
      "New York builds APIs.",
      "Accra builds systems.",
  });

  vectorize::Mapper mapper(
      pluss::ai::nodej::LexiconMapOptions{
          .lowercase = true,
          .max_features = 32,
          .min_document_frequency = 1,
          .binary = false,
          .min_ngram = 1,
          .max_ngram = 2,
      },
      vectorize::DocMapOptions{
          .use_lemmas = true,
          .skip_punct = true,
      });
  if (!mapper.learn(docs).ok()) {
    if (err) *err << "nodeb vectorize Mapper::learn should succeed on Doc batches\n";
    return false;
  }

  const auto& vocab = mapper.vocabulary();
  if (std::find(vocab.begin(), vocab.end(), "new york") == vocab.end() ||
      std::find(vocab.begin(), vocab.end(), "build api") == vocab.end()) {
    if (err) *err << "nodeb vectorize mapper should learn ngram features from lemmatized docs\n";
    return false;
  }

  auto count_dense_or = mapper.count_dense(docs);
  if (!count_dense_or.ok()) {
    if (err) *err << "nodeb vectorize Mapper::count_dense should succeed\n";
    return false;
  }
  if (count_dense_or.value().sample_count() != 2 ||
      count_dense_or.value().feature_count() != static_cast<std::int64_t>(vocab.size())) {
    if (err) *err << "nodeb vectorize dense count map should match vocabulary width\n";
    return false;
  }

  auto tfidf_sparse_or = mapper.tfidf_sparse(docs);
  if (!tfidf_sparse_or.ok()) {
    if (err) *err << "nodeb vectorize Mapper::tfidf_sparse should succeed\n";
    return false;
  }
  if (tfidf_sparse_or.value().rows != 2 ||
      tfidf_sparse_or.value().cols != static_cast<std::int64_t>(vocab.size())) {
    if (err) *err << "nodeb vectorize sparse tfidf map should preserve row/feature counts\n";
    return false;
  }

  vectorize::Hasher hasher(
      pluss::ai::nodej::HashMapOptions{
          .lowercase = true,
          .n_features = 64,
          .binary = false,
          .alternate_sign = false,
          .min_ngram = 1,
          .max_ngram = 2,
      },
      vectorize::DocMapOptions{
          .use_lemmas = true,
          .skip_punct = true,
      });
  auto hashed_dense_or = hasher.dense(docs);
  if (!hashed_dense_or.ok()) {
    if (err) *err << "nodeb vectorize Hasher::dense should succeed on Doc batches\n";
    return false;
  }
  if (hashed_dense_or.value().feature_count() != 64) {
    if (err) *err << "nodeb vectorize hasher should respect configured hash width\n";
    return false;
  }

  const auto stats = vectorize::describe(docs, vectorize::DocMapOptions{
      .use_lemmas = true,
      .skip_punct = true,
  });
  if (stats.doc_count != 2 ||
      stats.token_count != 7 ||
      stats.unique_term_count < 6 ||
      stats.document_token_counts.numel() != 2 ||
      stats.document_unique_term_counts.numel() != 2) {
    if (err) *err << "nodeb vectorize describe should emit corpus-wide and per-document text statistics\n";
    return false;
  }
  std::vector<std::string> vocab_terms{};
  vocab_terms.reserve(static_cast<std::size_t>(stats.vocabulary.numel()));
  for (std::int64_t i = 0; i < stats.vocabulary.numel(); ++i) {
    vocab_terms.emplace_back(stats.vocabulary.at(i));
  }
  if (std::find(vocab_terms.begin(), vocab_terms.end(), "build") == vocab_terms.end()) {
    if (err) *err << "nodeb vectorize describe should expose ranked vocabulary terms\n";
    return false;
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("nodeb vectorize tranche", "[nodeb][vectorize]") {
  if (!run_nodeb_vectorize_checks(&std::cerr)) {
    FAIL("nodeb vectorize checks failed");
  }
}
#else
int main() {
  if (!run_nodeb_vectorize_checks(&std::cerr)) {
    return 1;
  }
  std::cout << "nodeb vectorize checks passed\n";
  return 0;
}
#endif
