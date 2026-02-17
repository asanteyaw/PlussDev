#include <pluss/pluss.h>

#include <cstdint>
#include <iostream>
#include <vector>

int main() {
  pluss::Series a = pluss::value({1.0, 2.0, 3.0});
  std::vector<std::int64_t> raw{10, 20, 30, 40};
  pluss::Series b = pluss::make_series<std::int64_t>(raw, {2, 2});

  auto c = a.add(2.0).mul(3.0);
  auto d = b.sum();

  std::cout << "a numel=" << a.numel() << " c numel=" << c.numel() << "\n";
  std::cout << "b shape=(2,2) d=" << d.to_vec<std::int64_t>()[0] << "\n";
  return 0;
}
