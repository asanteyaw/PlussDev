#include "pluss/ma/eng/signal.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>

namespace eng_signal = pluss::subsp::eng::signal;

namespace {

void expect(bool condition, const std::string &message) {
  if (!condition)
    throw std::runtime_error(message);
}

void expect_close(const pluss::Series &actual,
                  const std::vector<double> &expected,
                  double tolerance = 1e-12) {
  const auto values = actual.to(pluss::ScalarType::Float64).to_vec<double>();
  expect(values.size() == expected.size(), "window length mismatch");
  for (std::size_t lane = 0; lane < values.size(); ++lane) {
    const double scale =
        std::max({1.0, std::abs(values[lane]), std::abs(expected[lane])});
    expect(std::abs(values[lane] - expected[lane]) <= tolerance * scale,
           "window value mismatch at lane " + std::to_string(lane));
  }
}

} // namespace

int main() {
  expect_close(eng_signal::hann(7), {0, 0.25, 0.75, 1, 0.75, 0.25, 0});
  expect_close(eng_signal::hann(6, {.mode = eng_signal::WindowMode::Periodic}),
               {0, 0.25, 0.75, 1, 0.75, 0.25});
  expect_close(eng_signal::hamming(5), {0.08, 0.54, 1.0, 0.54, 0.08});
  expect_close(eng_signal::blackman(5), {0.0, 0.34, 1.0, 0.34, 0.0});
  expect_close(eng_signal::bartlett(5), {0.0, 0.5, 1.0, 0.5, 0.0});
  expect_close(eng_signal::boxcar(4), {1, 1, 1, 1});
  expect_close(eng_signal::hann(1), {1});
  expect(eng_signal::hann(0).numel() == 0, "zero-length window must be empty");

  const auto f32 = eng_signal::hann(8, {.dtype = pluss::ScalarType::Float32});
  expect(f32.scalar_type() == pluss::ScalarType::Float32,
         "window dtype was not preserved");

  bool negative = false;
  try {
    (void)eng_signal::hann(-1);
  } catch (const pluss::Error &) {
    negative = true;
  }
  expect(negative, "negative window length was accepted");

  bool integral = false;
  try {
    (void)eng_signal::hann(4, {.dtype = pluss::ScalarType::Int64});
  } catch (const pluss::Error &) {
    integral = true;
  }
  expect(integral, "integral window dtype was accepted");
}
