// Pitfalls: matrices and many variables. Each call is made inside a try block so the message it produces can be printed.
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
  report("a heat map from a flat z", [&] { auto f = studio("m"); flash::heatmap(f.pane2("a"), x, flat, flat); });
  report("axes that do not match the matrix", [&] { auto f = studio("m"); flash::heatmap(f.pane2("a"), x, x, matrix); });
  report("a pair grid of columns of different lengths", [&] { auto f = studio("m"); flash::pairplot(f, {x, y3}); });
  report("a mask that is not the size of the matrix", [&] { auto f = studio("m"); flash::HeatmapOptions o; o.mask = pluss::make_series({1.0, 0.0}); flash::heatmap(f.pane2("a"), pluss::series::linspace(0.0, 4.0, 5, pluss::ScalarType::Float64), pluss::series::linspace(0.0, 3.0, 4, pluss::ScalarType::Float64), matrix, o); });
}
