#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>
#include <vector>

pluss::Series calc_tip (pluss::Series bills, double tip_percent) {
  return bills.mul(tip_percent / 100.0);
}

int main() {
  const auto mps_ready = pluss::mps::is_ready();
  const auto platform = mps_ready ? pluss::pMPS : pluss::pCPU;
  std::cout << (mps_ready ? "MPS ready. Training on GPU." : "Training on CPU.") << '\n';

  auto doubled = suppose(std::vector<int>{1, 2, 3, 4})
                     .then([](std::vector<int> v) {
                       for (auto& x : v) x *= 2;
                       return v;
                     })
                     .value();

  auto zipped = pluss::tils::transmit([](int a, int b, std::size_t i) {
                 return a + b + static_cast<int>(i);
               },
               doubled,
               std::vector<int>{10, 20, 30, 40});

  auto scores = pluss::make_series(std::vector<double>{79.4, 82.1, 78.5, 85.0}, {4});
  auto squared = pluss::tils::transmit([](const pluss::Series& s) { return s.mul(s); }, scores);

  std::cout << "zipped: ";
  for (auto v : zipped) std::cout << v << " ";
  std::cout << "\n";

  std::cout << "squared scores: ";
  for (auto v : squared.to_vec<double>()) std::cout << v << " ";
  std::cout << "\n";

  let bills = pluss::make_series({42.50, 35.85, 50.03, 15.21, 5.45});
  auto tips = pluss::tils::transmit(calc_tip, bills);
  std::cout << "tips: ";
  for (auto v : tips.to_vec<double>()) std::cout << v << " ";
  std::cout << "\n";
  
  return 0;
}
