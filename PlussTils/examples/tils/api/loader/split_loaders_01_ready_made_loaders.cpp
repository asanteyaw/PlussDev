// split_loaders does the same partition as split_fields and wraps each part in its own DataLoader,
// so you get three ready-to-use loaders in one call instead of three field lists to wrap yourself.
#include <pluss/tils/auxiliary/data_loader.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  std::vector<double> values(10);
  for (std::size_t i = 0; i < values.size(); ++i) values[i] = static_cast<double>(i);
  const pluss::Series x = pluss::make_series(values, {10}, cfg);

  loader::DataSplitOptions split_options{};
  split_options.train_ratio = 0.6;
  split_options.val_ratio = 0.2;
  split_options.seed = 3;

  loader::DataLoaderOptions train_options{};
  train_options.batch_size = 2;

  const loader::DataLoaderSplit loaders = loader::split_loaders({x}, split_options, train_options);
  std::cout << "train samples: " << loaders.train.sample_count() << '\n';
  std::cout << "val samples: " << loaders.val.sample_count() << '\n';
  std::cout << "test samples: " << loaders.test.sample_count() << '\n';
  std::cout << "train batches of two: " << loaders.train.batch_count() << '\n';
  return 0;
}
