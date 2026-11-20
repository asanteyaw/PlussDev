// From daily prices to a monthly view: daily returns, monthly averages, and the best and worst month.
#include <pluss/pluss.h>

#include <iostream>

using pluss::table::DataFrame;

int main() {
  // Eight trading days spread over three months.
  auto prices = DataFrame({
      {"date", pluss::make_timeline({"2026-01-05T00:00:00Z", "2026-01-20T00:00:00Z", "2026-01-28T00:00:00Z", "2026-02-03T00:00:00Z",
                                       "2026-02-17T00:00:00Z", "2026-03-02T00:00:00Z", "2026-03-16T00:00:00Z", "2026-03-30T00:00:00Z"})},
      {"close", pluss::make_series({100.0, 104.0, 102.0, 108.0, 111.0, 107.0, 112.0, 118.0})},
      {"volume", pluss::make_series({1200.0, 900.0, 1500.0, 1100.0, 1300.0, 800.0, 1000.0, 1400.0})},
  });
  std::cout << prices.to_string() << "\n";

  // 1. Day-to-day returns of the closing price (numeric streams use .ts()).
  auto returns = prices.features("close").ts().returns(1);
  std::cout << "returns: " << pluss::series::print(returns.as_series()) << "\n";

  // 2. Put the dates on the case axis and resample to months: mean close and total volume per month.
  auto indexed = prices.labels().set("date", false).features({"close", "volume"});
  auto monthly = indexed.ts().resample("MS").mean();
  std::cout << monthly.to_string() << "\n";
  auto monthly_volume = indexed.ts().resample("MS").sum();
  std::cout << monthly_volume.to_string() << "\n";
}
