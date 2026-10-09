#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <algorithm>
#include <cctype>
#include <iostream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

namespace {

std::vector<int> double_all(std::vector<int> values) {
  for (auto& v : values) v *= 2;
  return values;
}

int sum_all(const std::vector<int>& values) {
  int total = 0;
  for (const auto v : values) total += v;
  return total;
}

std::string label_total(int total) { return "total=" + std::to_string(total); }

void require_non_empty(const std::vector<int>& values) {
  if (values.empty()) throw std::runtime_error("require_non_empty: input cannot be empty");
}

double half_value(double x) { return x / 2.0; }

struct ClampAtLeast {
  int min_value{0};
  int operator()(int v) const { return (v < min_value) ? min_value : v; }
};

struct Celsius {
  explicit Celsius(double v) : value(v) {}
  double value{0.0};
};

struct ToFahrenheit {
  double operator()(Celsius c) const { return (c.value * 9.0 / 5.0) + 32.0; }
};

} // namespace

int main() {
  std::cout << "=== suppose / let demo ===\n\n";

  std::cout << "1) suppose as a single-value pipeline (free functions)\n";
  const auto tagged = pluss::tils::suppose(std::vector<int>{1, 2, 3, 4})
                          .tap(require_non_empty)
                          .then(double_all)
                          .then(sum_all)
                          .then(label_total)
                          .value();
  std::cout << "pipeline result: " << tagged << "\n\n";

  std::cout << "2) transform + into + function object\n";
  const auto numeric = pluss::tils::suppose(21)
                           .transform(ClampAtLeast{50})
                           .into<double>()
                           .then(half_value)
                           .value();
  std::cout << "suppose(21) -> clamp>=50 -> into<double> -> half = " << numeric << "\n\n";

  std::cout << "3) suppose.as<T>(value) for explicit typed starts\n";
  const auto temp_f = pluss::tils::suppose.as<Celsius>(36.5)
                          .then(ToFahrenheit{})
                          .value();
  std::cout << "suppose.as<Celsius>(36.5) -> Fahrenheit = " << temp_f << "\n\n";

  std::cout << "4) Lambdas in the same pipeline style\n";
  const auto shout = pluss::tils::suppose(std::string("pluss queue + suppose"))
                         .then([](std::string s) {
                           std::transform(s.begin(), s.end(), s.begin(),
                                          [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
                           return s;
                         })
                         .then([](const std::string& s) { return s + "!!!"; })
                         .value();
  std::cout << "lambda pipeline result: " << shout << "\n\n";

  std::cout << "5) `let` for concise Pluss payload naming\n";
  auto payload = pluss::make_series(std::vector<double>{1, 2, 3}, {3});
  std::cout << "let payload = make_series(...) -> numel=" << payload.numel()
            << ", mean=" << payload.mean().item<double>() << "\n";

  auto queue = pluss::Queue(pluss::make_series(std::vector<double>{1.5, 2.5, 3.5}));
  std::cout << "let queue = pluss::Queue(...) -> numel=" << queue.series().numel()
            << ", front=" << queue.series().to_vec<double>(0) << "\n";

  std::cout << "\nDone.\n";
  return 0;
}
