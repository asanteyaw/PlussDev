#include <pluss/pluss.h>

#include <iostream>
#include <vector>

int main() {
  auto doubled = let(std::vector<int>{1, 2, 3, 4})
                     .then([](std::vector<int> v) {
                       for (auto& x : v) x *= 2;
                       return v;
                     })
                     .value();

  auto zipped = pluss::queue([](int a, int b, std::size_t i) {
                 return a + b + static_cast<int>(i);
               },
               doubled,
               std::vector<int>{10, 20, 30, 40});

  auto scores = pluss::value(std::vector<double>{79.4, 82.1, 78.5, 85.0}, {4});
  auto squared = pluss::queue([](const pluss::Series& s) { return s.mul(s); }, scores);

  std::cout << "zipped: ";
  for (auto v : zipped) std::cout << v << " ";
  std::cout << "\n";

  std::cout << "squared scores: ";
  for (auto v : squared.to_vec<double>()) std::cout << v << " ";
  std::cout << "\n";

  return 0;
}
