#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

namespace {
void print_row(const std::string& label, const pluss::Series& row) {
  const std::vector<double> vec = row.to(pluss::pCPU, pluss::ScalarType::Float64).contiguous().host<double>();
  std::cout << label << ": [";
  for (double v : vec) std::cout << v << " ";
  std::cout << "]\n";
}
}  // namespace

int main() {
  namespace cls = pluss::ai::classic;

  cls::HashMapOptions unigram{};
  unigram.n_features = 16;
  cls::HashMapper unigram_mapper(unigram);

  cls::HashMapOptions bigram{};
  bigram.n_features = 16;
  bigram.min_ngram = 2;
  bigram.max_ngram = 2;
  cls::HashMapper bigram_mapper(bigram);

  const std::string doc = "the cat sat";
  print_row("unigram", unigram_mapper.map_dense({doc}).value().x);
  print_row("bigram ", bigram_mapper.map_dense({doc}).value().x);

  return 0;
}
