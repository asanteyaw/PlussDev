// Pitfalls: distributions and groups. Each call is made inside a try block so the message it produces can be printed.
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
  report("a histogram of an empty series", [&] { auto f = studio("d"); flash::histplot(f.pane2("a"), empty); });
  report("a histogram with no bins", [&] { auto f = studio("d"); flash::histplot(f.pane2("a"), y, 0); });
  report("three groups for five values", [&] { auto f = studio("d"); flash::boxplot(f.pane2("a"), groups, y); });
  report("a plot kind that does not exist", [&] { auto f = studio("d"); flash::catplot(f.pane2("a"), groups, y3, "cake"); });
}
