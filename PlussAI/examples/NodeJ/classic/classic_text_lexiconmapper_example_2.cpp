#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::LexiconMapper mapper;
  mapper.learn({"the cat sat", "the dog sat", "cats and dogs", "the cat ran"});

  const cls::FeatureBatch dense = mapper.map_dense({"the cat sat again", "dogs ran"}).value();
  const cls::SparseTripletMatrix sparse = mapper.map_sparse({"the cat sat again", "dogs ran"}).value();
  const cls::SparseCsrMatrix csr = cls::sparse_triplet_to_csr(sparse).value();

  std::vector<double> dense_from_sparse(static_cast<std::size_t>(csr.rows * csr.cols), 0.0);
  for (std::int64_t r = 0; r < csr.rows; ++r) {
    for (std::int64_t p = csr.row_ptr[static_cast<std::size_t>(r)]; p < csr.row_ptr[static_cast<std::size_t>(r + 1)]; ++p) {
      dense_from_sparse[static_cast<std::size_t>(r * csr.cols + csr.col_ind[static_cast<std::size_t>(p)])] = csr.values[static_cast<std::size_t>(p)];
    }
  }
  const pluss::Series reconstructed = pluss::make_series(
      dense_from_sparse, {csr.rows, csr.cols}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU));
  const double diff = (dense.x - reconstructed).abs().amax(0, false).amax(0, false).item<double>();
  std::cout << "max abs diff (dense vs. reconstructed-from-sparse) = " << diff << "\n";

  return 0;
}
