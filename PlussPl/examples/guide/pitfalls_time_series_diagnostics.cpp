// Pitfalls: time-series diagnostics. Each call is made inside a try block so the message it produces can be printed.
#include <pluss/pl/plusspl.h>
#include <pluss/pluss.h>

#include <iostream>

using namespace pluss::pl;

template <class F> static void report(const char *what, F &&call) {
  try {
    call();
    std::cout << what << ": no error\n";
  } catch (const pluss::Error &e) {
    std::cout << what << ": " << e.message() << "\n";
  }
}

int main() {
  auto x = pluss::series::linspace(0.0, 1.0, 5, pluss::ScalarType::Float64);
  auto y = x.sin();
  auto y3 = pluss::series::linspace(0.0, 1.0, 3, pluss::ScalarType::Float64);
  auto flat = pluss::make_series({1.0, 2.0, 3.0, 4.0});
  auto matrix = pluss::series::linspace(0.0, 1.0, 20, pluss::ScalarType::Float64).reshape({4, 5});
  auto empty = pluss::series::zeros({0}, pluss::ScalarType::Float64);
  auto groups = pluss::make_note({"a", "b", "c"});
  report("an autocorrelation of a single value", [&] { auto f = studio("s"); flash::acfplot(f, pluss::make_series({1.0})); });
  report("a lag plot of a single value", [&] { auto f = studio("s"); flash::lagplot(f, pluss::make_series({1.0})); });
  report("a cross-correlation of series of different lengths", [&] { auto f = studio("s"); flash::ccfplot(f, x, y3); });
}
