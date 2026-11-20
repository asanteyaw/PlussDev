// Compare what-if scenarios side by side: one table of products under four assumptions held in an Atlas, profit computed in every
// scenario at once, and the sensitivity of the total to each assumption.
#include <pluss/pluss.h>

#include <iostream>

using pluss::table::Atlas;
using pluss::table::DataFrame;
using pluss::table::Shelf;

static DataFrame scenario(double price_factor, double cost_factor) {
  const std::vector<double> price{20.0, 35.0, 50.0, 12.0}, cost{12.0, 25.0, 38.0, 9.0}, volume{500.0, 300.0, 120.0, 900.0};
  std::vector<double> p, c;
  for (std::size_t i = 0; i < price.size(); ++i) {
    p.push_back(price[i] * price_factor);
    c.push_back(cost[i] * cost_factor);
  }
  return DataFrame({{"product", pluss::make_note({"basic", "plus", "pro", "lite"})}, {"price", pluss::make_series(p)}, {"cost", pluss::make_series(c)}, {"volume", pluss::make_series(volume)}});
}

int main() {
  Atlas scenarios({scenario(1.0, 1.0), scenario(1.10, 1.0), scenario(1.0, 1.15), scenario(1.10, 1.15)},
                  Shelf(pluss::make_note({"baseline", "price +10%", "cost +15%", "both"})));
  std::cout << "scenarios " << scenarios.depth() << ", products " << scenarios.ncases() << "\n";

  // 1. The same formula in every scenario: map_frames applies a DataFrame transform to each frame.
  auto with_profit = scenarios.map_frames([](const DataFrame &f) {
    return f.assign({{"profit", [](const DataFrame &g) { return (g.features("price") - g.features("cost")) * g.features("volume"); }}});
  });
  std::cout << "profit by product in the baseline:\n" << with_profit.loc("baseline").features({"product", "profit"}).to_string() << "\n";

  // 2. One row per scenario: total profit. aggregate_depth sees all scenarios at once.
  auto totals = with_profit.aggregate_depth([](const std::vector<DataFrame> &frames) {
    std::vector<double> total;
    for (const auto &f : frames) total.push_back(f.features("profit").as_series().sum().item<double>());
    return DataFrame({{"total_profit", pluss::make_series(total)}});
  });
  std::cout << totals.to_string() << "\n";

  // 3. Sensitivity against the baseline, in money and in percent.
  auto profit = totals.features("total_profit").as_series();
  const double base = profit.select(0, 0).item<double>();
  auto delta = profit.sub(base);
  std::cout << "change from baseline: " << pluss::series::print(delta) << "\n";
  std::cout << "percent of baseline:  " << pluss::series::print(delta.div(base).mul(100.0)) << "\n";

  // 4. Which product suffers most when costs rise? Compare product profits between the baseline and the cost scenario.
  auto drop = with_profit.loc("baseline").features("profit").as_series().sub(with_profit.loc("cost +15%").features("profit").as_series());
  std::cout << "profit lost per product when costs rise 15%: " << pluss::series::print(drop) << "\n";
}
