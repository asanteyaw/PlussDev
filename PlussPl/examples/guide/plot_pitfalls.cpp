// The mistakes that actually happen when drawing with flash and the message each one produces: data of the wrong shape, an empty input, a
// name that does not exist, and options that contradict each other. Each call is made inside a try block so the message can be printed.
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
  auto y3 = pluss::series::linspace(0.0, 1.0, 3, pluss::ScalarType::Float64);
  auto flat = pluss::make_series({1.0, 2.0, 3.0, 4.0});
  auto matrix = pluss::series::linspace(0.0, 1.0, 20, pluss::ScalarType::Float64).reshape({4, 5});
  auto empty = pluss::series::zeros({0}, pluss::ScalarType::Float64);

  report("x and y of different lengths", [&] { auto f = studio("p1"); flash::scatterplot(f.pane2("a"), x, y3); });
  report("an empty series", [&] { auto f = studio("p2"); flash::lineplot(f.pane2("a"), empty, empty); });
  report("a heat map from a flat z", [&] { auto f = studio("p3"); flash::heatmap(f.pane2("a"), x, flat, flat); });
  report("a heat map with the wrong axis lengths", [&] { auto f = studio("p4"); flash::heatmap(f.pane2("a"), x, x, matrix); });
  report("a mask of the wrong size", [&] {
    auto f = studio("p5");
    flash::HeatmapOptions options;
    options.mask = pluss::make_series({1.0, 0.0});
    flash::heatmap(f.pane2("a"), pluss::series::linspace(0.0, 4.0, 5, pluss::ScalarType::Float64), pluss::series::linspace(0.0, 3.0, 4, pluss::ScalarType::Float64), matrix, options);
  });
  report("a bundled dataset that does not exist", [&] {
    const auto d = flash::load_dataset("nothing");
    std::cout << "  load_dataset returns an empty optional: " << (d.has_value() ? "no" : "yes") << "\n";
  });
  report("an unknown kind string", [&] { auto f = studio("p6"); flash::jointplot(f, x, x, "bubbles"); });
  report("a regression with too few points", [&] {
    auto f = studio("p7");
    flash::regplot(f.pane2("a"), pluss::make_series({1.0}), pluss::make_series({2.0}));
  });
}
