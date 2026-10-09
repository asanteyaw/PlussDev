#include <pluss/tils/aux/data_loader.h>
#include <pluss/pluss.h>

#include <algorithm>
#include <iostream>
#include <vector>

int main() {
  std::cout << std::boolalpha;
  namespace loader = pluss::loader;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  std::vector<double> values(10);
  for (std::size_t i = 0; i < values.size(); ++i) values[i] = static_cast<double>(i);
  const pluss::Series x = pluss::make_series(values, {10}, cfg);
  std::vector<double> tens(values);
  for (auto& v : tens) v *= 10.0;
  const pluss::Series y = pluss::make_series(tens, {10}, cfg);

  // Sixty percent train, twenty percent validation. The remaining twenty percent is test.
  loader::DataSplitOptions options{};
  options.train_ratio = 0.6;
  options.val_ratio = 0.2;
  options.seed = 7;
  const loader::DataSplit split = loader::split_fields({x, y}, options);

  std::cout << "train rows: " << split.train_fields[0].sizes()[0] << '\n';
  std::cout << "val rows: " << split.val_fields[0].sizes()[0] << '\n';
  std::cout << "test rows: " << split.test_fields[0].sizes()[0] << '\n';

  // The three splits partition the input: every row appears exactly once.
  std::vector<double> all;
  for (const auto* part : {&split.train_fields[0], &split.val_fields[0], &split.test_fields[0]}) {
    const auto v = part->reshape({part->numel()}).to_vec<double>();
    all.insert(all.end(), v.begin(), v.end());
  }
  std::sort(all.begin(), all.end());
  std::cout << "partition covers all rows once: " << (all == values) << '\n';

  // Fields stay aligned: y was built as ten times x, so every train row keeps that relation.
  const auto train_x = split.train_fields[0].reshape({split.train_fields[0].numel()}).to_vec<double>();
  const auto train_y = split.train_fields[1].reshape({split.train_fields[1].numel()}).to_vec<double>();
  bool aligned = train_x.size() == train_y.size();
  for (std::size_t i = 0; aligned && i < train_x.size(); ++i) aligned = train_y[i] == 10.0 * train_x[i];
  std::cout << "fields stay aligned: " << aligned << '\n';

  // The same seed gives the same split.
  const loader::DataSplit again = loader::split_fields({x, y}, options);
  std::cout << "same seed, same split: "
            << (again.train_fields[0].reshape({again.train_fields[0].numel()}).to_vec<double>() == train_x) << '\n';
}
