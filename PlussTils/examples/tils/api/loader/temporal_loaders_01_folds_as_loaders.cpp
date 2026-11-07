// temporal_split_loaders wraps each fold's train and test fields in their own DataLoader, ready to
// iterate, the temporal counterpart to split_loaders.
#include <pluss/tils/kit/data_loader.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  std::vector<double> values(12);
  for (std::size_t i = 0; i < values.size(); ++i) values[i] = static_cast<double>(i);
  const pluss::Series x = pluss::make_series(values, {12}, cfg);

  loader::TemporalSplitOptions split_options{};
  split_options.folds = 2;
  split_options.test_size = 3;
  loader::DataLoaderOptions train_options{};
  train_options.batch_size = 2;

  const loader::TemporalLoaderSplit split = loader::temporal_split_loaders({x}, split_options, train_options);
  std::cout << "fold count: " << split.folds.size() << '\n';
  std::cout << "fold 0 train batches of two: " << split.folds[0].train.batch_count() << '\n';
  std::cout << "fold 0 test samples: " << split.folds[0].test.sample_count() << '\n';
  return 0;
}
