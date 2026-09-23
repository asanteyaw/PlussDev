#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

namespace {
void print_row(const std::string& label, const pluss::Series& row) {
  const std::vector<double> vec = row.to(pluss::pCPU, pluss::ScalarType::Float64).contiguous().to_vec<double>();
  std::cout << label << ": [";
  for (double v : vec) std::cout << v << " ";
  std::cout << "]\n";
}
}  // namespace

int main() {
  namespace cls = pluss::ai::classic;

  cls::HashMapOptions options{};
  options.n_features = 8;
  cls::HashMapper positive_only(options);

  options.alternate_sign = true;
  cls::HashMapper signed_mapper(options);

  for (const std::string& term : {"cat", "dog", "bird", "fish"}) {
    print_row(term + " no-alternate-sign", positive_only.map_dense({term}).value().x);
    print_row(term + " alternate-sign   ", signed_mapper.map_dense({term}).value().x);
  }

  return 0;
}
