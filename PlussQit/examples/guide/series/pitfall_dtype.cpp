// The dtype of a Series decides how much precision a result carries, and the mean of integers is not an integer.
#include <pluss/pluss.h>

#include <iomanip>
#include <iostream>

int main() {
  using pluss::ScalarType;
  std::cout << std::setprecision(12);
  // A million readings of 0.1. Float32 keeps about seven digits, so the running total drifts; Float64 keeps about sixteen.
  auto exact = pluss::series::full({1000000}, 0.1, ScalarType::Float64);
  std::cout << "sum in Float64: " << exact.sum().item<double>() << "\n";
  std::cout << "sum in Float32: " << exact.to(ScalarType::Float32).sum().item<double>() << "\n";

  // The mean of an integer Series is a floating-point Series, and it is Float32 unless you convert first.
  auto ids = pluss::make_series({100000001, 100000002});
  auto narrow = ids.mean();
  auto wide = ids.to(ScalarType::Float64).mean();
  std::cout << "mean of integers is " << narrow.scalar_type() << ": " << narrow.item<double>() << "\n";
  std::cout << "after to(Float64) it is " << wide.scalar_type() << ": " << wide.item<double>() << "\n";
  std::cout << "the true mean is 100000001.5\n";
}
