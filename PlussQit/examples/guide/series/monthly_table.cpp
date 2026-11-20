// A year-by-month table: yearly totals, running totals, month-on-month change and the best month of each year.
#include <pluss/pluss.h>

#include <iostream>

static void show(const char *label, const pluss::Series &s) { std::cout << label << ": " << pluss::series::print(s) << "\n"; }

int main() {
  // Three years of monthly rainfall (mm) laid out flat, January of year one first.
  auto flat = pluss::make_series({78.0, 62.0, 55.0, 48.0, 51.0, 40.0, 33.0, 38.0, 52.0, 71.0, 85.0, 90.0,
                                  82.0, 59.0, 61.0, 45.0, 49.0, 36.0, 30.0, 41.0, 58.0, 69.0, 80.0, 95.0,
                                  74.0, 66.0, 52.0, 50.0, 47.0, 42.0, 29.0, 35.0, 55.0, 73.0, 88.0, 92.0});
  // 1. reshape turns the flat series into a [year, month] table without copying the values.
  auto table = flat.reshape({3, 12});
  std::cout << "table is " << table.size(0) << " years by " << table.size(1) << " months\n";

  // 2. Reductions along an axis: dimension 1 runs across the months of one year.
  show("total per year", table.sum(1));
  show("wettest month (0 = Jan) per year", table.argmax(1));
  show("average per calendar month", table.mean(0));

  // 3. Running totals within each year, and the change from one month to the next.
  show("running total, year 1", table.cumsum(1).select(0, 0));
  show("month-on-month change, year 1", table.diff(1, 1).select(0, 0));

  // 4. Compare each year with the average year: positive means wetter than usual.
  show("departure from the average year", (table - table.mean(0)).sum(1));

  // 5. permute swaps the axes, so each row is now a calendar month and each column a year.
  show("March across the years", table.permute({1, 0}).select(0, 2));
}
