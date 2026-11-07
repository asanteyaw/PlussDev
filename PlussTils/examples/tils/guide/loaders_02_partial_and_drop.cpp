#include <pluss/tils/kit/data_loader.h>
#include <pluss/pluss.h>

#include <iostream>
#include <vector>

int main() {
  namespace loader = pluss::loader;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::make_series(std::vector<double>{0, 1, 2, 3, 4, 5, 6}, {7}, cfg);

  // Keeping the partial batch: seven samples in batches of three give 3, 3, 1.
  loader::DataLoaderOptions keep{};
  keep.batch_size = 3;
  keep.drop_last = false;
  loader::DataLoader kept({x}, keep);

  // Dropping it: the one leftover sample is never delivered.
  loader::DataLoaderOptions drop{};
  drop.batch_size = 3;
  drop.drop_last = true;
  loader::DataLoader dropped({x}, drop);

  std::cout << "kept batches: " << kept.batch_count() << '\n';
  std::cout << "dropped batches: " << dropped.batch_count() << '\n';
  std::cout << "last kept batch rows: " << kept.batch_at(kept.batch_count() - 1).x().sizes()[0] << '\n';
}
