#include <pluss/nlp.h>
#include <pluss/ai/ml.h>

#include <iostream>
#include <vector>

int main() {
  namespace nlp_api = pluss::ai::nlp;
  using namespace nlp_api;

  Workflow prep{
      .lang = Language::English,
      .stages = {
          stage::tokenize(),
          stage::sentencize(),
          stage::lemmatize(),
      },
  };
  const auto docs = prep.infer(std::vector<std::string>{
      "New York builds APIs.",
      "Accra builds systems.",
      "Tema builds data platforms.",
  });

  vectorize::Mapper mapper(
      pluss::ai::classic::LexiconMapOptions{
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
  const auto learn = mapper.learn(docs);
  if (!learn.ok()) {
    std::cerr << "mapper learn failed: " << learn.message << "\n";
    return 1;
  }

  const auto count_dense = mapper.count_dense(docs);
  const auto tfidf_sparse = mapper.tfidf_sparse(docs);
  if (!count_dense.ok() || !tfidf_sparse.ok()) {
    std::cerr << "vectorization failed\n";
    return 1;
  }

  vectorize::Hasher hasher(
      pluss::ai::classic::HashMapOptions{
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
  const auto hashed = hasher.dense(docs);
  if (!hashed.ok()) {
    std::cerr << "hash vectorization failed\n";
    return 1;
  }

  const auto stats = vectorize::describe(docs);

  std::cout << "== PlussAI NLP Vectorization ==\n";
  std::cout << "vocab size=" << mapper.vocabulary().size()
            << " dense rows=" << count_dense.value().sample_count()
            << " dense cols=" << count_dense.value().feature_count()
            << " tfidf nnz=" << tfidf_sparse.value().values.numel()
            << " hashed cols=" << hashed.value().feature_count() << "\n";

  std::cout << "top vocabulary sample:\n";
  for (std::size_t i = 0; i < std::min<std::size_t>(mapper.vocabulary().size(), 10); ++i) {
    std::cout << "  " << mapper.vocabulary()[i] << "\n";
  }

  std::cout << "corpus stats: docs=" << stats.doc_count
            << " tokens=" << stats.token_count
            << " unique_terms=" << stats.unique_term_count << "\n";

  return 0;
}
