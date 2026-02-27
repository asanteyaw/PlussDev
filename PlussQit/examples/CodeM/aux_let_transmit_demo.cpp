// If you ever want to disable let macro behavior in a TU, define PLUSS_DISABLE_LET_AUTO before including pluss.h>
// #define PLUSS_DISABLE_LET_AUTO
#include <pluss/pluss.h>

#include <iostream>
#include <vector>

pluss::Series calc_tip (pluss::Series bills, double tip_percent) {
  return bills.mul(tip_percent / 100.0);
}

int main() {
  let mps_ready = pluss::mps::is_ready();
  let platform = mps_ready ? pluss::pMPS : pluss::pCPU;
  std::cout << (mps_ready ? "MPS ready. Training on GPU." : "Training on CPU.") << '\n';

  auto doubled = suppose(std::vector<int>{1, 2, 3, 4})
                     .then([](std::vector<int> v) {
                       for (auto& x : v) x *= 2;
                       return v;
                     })
                     .value();

  auto zipped = pluss::transmit([](int a, int b, std::size_t i) {
                 return a + b + static_cast<int>(i);
               },
               doubled,
               std::vector<int>{10, 20, 30, 40});

  auto scores = pluss::make_series(std::vector<double>{79.4, 82.1, 78.5, 85.0}, {4});
  auto squared = pluss::transmit([](const pluss::Series& s) { return s.mul(s); }, scores);

  std::cout << "zipped: ";
  for (auto v : zipped) std::cout << v << " ";
  std::cout << "\n";

  std::cout << "squared scores: ";
  for (auto v : squared.to_vec<double>()) std::cout << v << " ";
  std::cout << "\n";

  let bills = pluss::make_series({42.50, 35.85, 50.03, 15.21, 5.45});
  let tips = pluss::transmit(calc_tip, bills);
  
  return 0;
}
