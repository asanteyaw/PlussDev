// A batch holds one Series per field, in the order the dataset or the loader produced them. at()
// and operator[] read a field by position, with the same bounds check either way.
#include <pluss/tils/aux/data_loader.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  loader::DataBatch batch{};
  batch.fields.push_back(pluss::make_series(std::vector<double>{1, 2, 3}, {3}, cfg));
  batch.fields.push_back(pluss::make_series(std::vector<double>{0, 1, 0}, {3}, cfg));

  std::cout << "size: " << batch.size() << '\n';
  std::cout << "at(0) numel: " << batch.at(0).numel() << '\n';
  std::cout << "operator[](1) numel: " << batch[1].numel() << '\n';
  return 0;
}
