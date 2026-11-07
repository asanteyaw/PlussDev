// Taking one host number out of a Series. An average computed by a pipeline is a one-element Series,
// and the extraction reads it back as a plain number. A Series with more than one element cannot be
// read as one number, and the extraction refuses it.
#include <pluss/mic.h>
#include <pluss/tils/kit/scalar_cast.h>

#include <exception>
#include <iostream>
#include <vector>

int main() {
  const auto cfg = pluss::Config{}.with_platform(pluss::pCPU);
  const auto average = pluss::make_series(std::vector<double>{42.5}, {}, cfg);
  std::cout << "average: " << pluss::tils::as_f64(average) << '\n';

  const auto column = pluss::make_series(std::vector<double>{1.0, 2.0}, {2}, cfg);
  try {
    (void)pluss::tils::as_f64(column);
    std::cout << "two values read as one\n";
  } catch (const std::exception&) {
    std::cout << "two values refused as one number\n";
  }
  return 0;
}
