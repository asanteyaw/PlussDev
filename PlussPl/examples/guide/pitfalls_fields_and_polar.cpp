// Pitfalls: fields and polar plots. Each call is made inside a try block so the message it produces can be printed.
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
  report("a field from a flat z", [&] { auto f = studio("f"); f.pane2("a").fieldmap(x, x, y); });
  report("contour lines from a flat z", [&] { auto f = studio("f"); f.pane2("a").contour(x, x, y, 5); });
  report("an angle and a radius of different lengths", [&] { auto f = studio("f"); f.pane2("a").trend(x, y3); });
}
