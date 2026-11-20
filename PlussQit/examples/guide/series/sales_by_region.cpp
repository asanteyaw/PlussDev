// Summarise transactions by group: totals, averages and shares per region, and the best sale in each.
#include <pluss/pluss.h>

#include <iostream>

static void show(const char *label, const pluss::Series &s) { std::cout << label << ": " << pluss::series::print(s) << "\n"; }

int main() {
  // Ten sales. region[i] is 0 = north, 1 = south, 2 = east for sale i.
  auto region = pluss::make_series({0, 1, 0, 2, 1, 0, 2, 2, 1, 0});
  auto amount = pluss::make_series({120.0, 80.0, 45.0, 200.0, 95.0, 60.0, 150.0, 30.0, 110.0, 75.0});
  const std::int64_t regions = 3;

  // 1. One reduction per region: the sum, mean and largest sale of each group.
  auto total = amount.grouped_sum(region, regions);
  auto mean = amount.grouped_mean(region, regions);
  auto largest = amount.grouped_max(region, regions);
  auto count = amount.grouped_count(region, regions);
  show("sales per region", count);
  show("total per region", total);
  show("mean per region", mean);
  show("largest per region", largest);

  // 2. Each region's share of the whole.
  show("share of total", total / total.sum());

  // 3. Each sale as a fraction of its own region's total: gather the region total back onto every sale.
  auto own_total = total.index_select(0, region);
  show("sale / its region total", amount / own_total);

  // 4. scatter_add builds the same totals from first principles: add each amount into the slot named by its region.
  auto manual = pluss::series::zeros({regions}, pluss::ScalarType::Float64).scatter_add(0, region, amount);
  show("scatter_add totals", manual);
  std::cout << "grouped_sum and scatter_add agree: " << total.allclose(manual) << "\n";
}
