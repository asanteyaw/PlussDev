// Scaling sensor readings to a share of the largest one. The same callable runs over every reading,
// and transmit returns a new vector of the results, so the original readings are untouched.
#include <pluss/tils/aux/transmit.h>

#include <algorithm>
#include <iostream>
#include <vector>

int main() {
  const std::vector<double> readings{12.0, 48.0, 30.0, 6.0};
  const double peak = *std::max_element(readings.begin(), readings.end());

  const auto shares = pluss::tils::transmit([peak](double r) { return r / peak; }, readings);

  for (std::size_t i = 0; i < shares.size(); ++i) {
    std::cout << "reading " << readings[i] << " is " << shares[i] * 100.0 << "% of peak\n";
  }
  std::cout << "original still holds " << readings.size() << " readings\n";
  return 0;
}
