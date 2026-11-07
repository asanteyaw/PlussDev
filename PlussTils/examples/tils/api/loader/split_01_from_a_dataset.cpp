// split_fields also accepts a Dataset or a SeriesDataset directly, reading its batch_dim from the
// dataset instead of the options, so a dataset you already built can be split without unpacking it.
#include <pluss/tils/kit/data_loader.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  std::vector<double> values(10);
  for (std::size_t i = 0; i < values.size(); ++i) values[i] = static_cast<double>(i);
  const pluss::Series x = pluss::make_series(values, {10}, cfg);
  const loader::SeriesDataset dataset({x});

  loader::DataSplitOptions options{};
  options.train_ratio = 0.7;
  options.val_ratio = 0.1;
  const loader::DataSplit split = loader::split_fields(dataset, options);
  std::cout << "train rows: " << split.train_fields[0].numel() << '\n';
  std::cout << "val rows: " << split.val_fields[0].numel() << '\n';
  std::cout << "test rows: " << split.test_fields[0].numel() << '\n';
  return 0;
}
