#include <pluss/tils/kit/data_loader.h>
#include <pluss/pluss.h>

#include <iostream>
#include <vector>

int main() {
  namespace loader = pluss::loader;

  // Seven samples with two fields. Field x holds a row of two values per sample;
  // field y holds one label per sample. Both fields share the same batch dimension.
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::make_series(std::vector<double>{
      0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13}, {7, 2}, cfg);
  const pluss::Series y = pluss::make_series(std::vector<double>{0, 1, 0, 1, 0, 1, 0}, {7}, cfg);

  loader::DataLoaderOptions options{};
  options.batch_size = 3;
  loader::DataLoader batches_source({x, y}, options);

  std::cout << "samples: " << batches_source.sample_count() << '\n';
  std::cout << "batches: " << batches_source.batch_count() << '\n';

  // Sequential order visits samples 0..6 in turn. The last batch is smaller.
  for (std::size_t i = 0; i < batches_source.batch_count(); ++i) {
    const auto batch = batches_source.batch_at(i);
    std::cout << "batch " << i << " rows: " << batch.x().sizes()[0]
              << ", label sum: " << batch.y().sum().item<double>() << '\n';
  }
}
