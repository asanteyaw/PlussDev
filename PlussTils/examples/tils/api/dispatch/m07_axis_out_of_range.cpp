// Asking for an axis the table does not have. The table is two-dimensional, so axis 2 is refused, and
// the error names the operation so the caller can find the bad call.
#include <pluss/mic.h>
#include <pluss/tils/aux/translate.h>

#include <exception>
#include <iostream>
#include <vector>

int main() {
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const auto table = pluss::make_series(std::vector<double>{1, 2, 3, 4}, {2, 2}, cfg);
  try {
    (void)pluss::tils::map_axis([](const pluss::Series& s) { return s.numel(); }, table, 2);
    std::cout << "axis 2 accepted\n";
  } catch (const std::exception& error) {
    const std::string message = error.what();
    std::cout << "axis 2 refused, message names the axis: " << (message.find("axis out of range") != std::string::npos) << '\n';
  }
  return 0;
}
