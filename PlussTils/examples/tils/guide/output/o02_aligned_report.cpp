// A monthly sales report whose columns line up. Each row is written with the format path, so the
// widths and alignments are part of the pattern, and the totals use grouping and fixed decimals.
#include <pluss/tils/kit/output.h>

#include <iostream>
#include <string>
#include <vector>

using namespace pluss::tils;

struct Sale {
  std::string region;
  long units;
  double revenue;
};

int main() {
  OutputEngine out{std::cout};
  const std::vector<Sale> sales{
      {"north", 1280, 15360.5},
      {"south", 96, 1152.0},
      {"east", 25400, 304800.25},
  };

  out.writelnf("{:<8}{:>10}{:>16}", "region", "units", "revenue");
  out.writelnf("{:-<34}", "");
  long total_units = 0;
  double total_revenue = 0.0;
  for (const auto& sale : sales) {
    // The trailing options apply to every value on the line, so units and revenue both group,
    // and revenue shows two decimals.
    out.writelnf("{:<8}{:>10}{:>16}", sale.region, sale.units, sale.revenue,
                 PrintOptions{}.fixed(2).grouped(','));
    total_units += sale.units;
    total_revenue += sale.revenue;
  }
  out.writelnf("{:-<34}", "");
  out.writelnf("{:<8}{:>10}{:>16}", "total", total_units, total_revenue,
               PrintOptions{}.fixed(2).grouped(','));
  return 0;
}
