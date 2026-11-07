// temporal_split_fields applies the same fold plan to real Series fields, so each fold's train and
// test are ready-to-use data, in time order, with no shuffling anywhere in the process.
#include <pluss/tils/kit/data_loader.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  std::cout << std::boolalpha;
  namespace loader = pluss::loader;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  std::vector<double> values(12);
  for (std::size_t i = 0; i < values.size(); ++i) values[i] = static_cast<double>(i);
  const pluss::Series x = pluss::make_series(values, {12}, cfg);

  loader::TemporalSplitOptions options{};
  options.folds = 2;
  options.test_size = 3;
  const loader::TemporalFieldSplit split = loader::temporal_split_fields({x}, options);

  std::cout << "fold count: " << split.folds.size() << '\n';
  const auto& last_fold = split.folds.back();
  std::cout << "last fold train size: " << last_fold.train_fields[0].numel() << '\n';
  std::cout << "last fold test size: " << last_fold.test_fields[0].numel() << '\n';
  std::cout << "last fold test is the final three values: "
            << (last_fold.test_fields[0].host<double>() == std::vector<double>{9, 10, 11}) << '\n';
  return 0;
}
