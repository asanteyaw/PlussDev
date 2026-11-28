#include <pluss/pluss.h>

#include <iostream>

int main() {
  const auto x = pluss::make_series({1.0, 2.0, 3.0});
  if (x.mean().item<double>() != 2.0) return 1;

  using pluss::table::DataFrame;
  using pluss::table::Shelf;
  auto observations = DataFrame({
      {"day", Shelf(pluss::make_note({"Mon", "Tue", "Wed"}))},
      {"value", Shelf(pluss::make_series({1.0, 2.0, 3.0}))},
  });
  const auto summary = observations.describe(false);
  std::cout << summary << '\n';

  auto chart = observations.plot().line("day", "value");
  chart.save_svg("plussii-code-smoke.svg");
  std::cout << "mean=2; chart saved\n";
}
