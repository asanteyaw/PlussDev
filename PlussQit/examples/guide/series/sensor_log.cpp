// Turn a raw sensor log into a trustworthy summary: find the readings that cannot be right, replace them, and report.
#include <pluss/pluss.h>

#include <cmath>
#include <iostream>
#include <limits>

static void show(const char *label, const pluss::Series &s) { std::cout << label << ": " << pluss::series::print(s) << "\n"; }

int main() {
  const double missing = std::numeric_limits<double>::quiet_NaN();
  // One reading a minute from a room thermometer: two dropouts (NaN), a spike and a sensor fault.
  auto log = pluss::make_series({21.4, 21.6, missing, 22.0, 85.0, 22.3, 22.1, missing, 21.9, -40.0, 22.2, 22.4});
  show("raw log", log);

  // 1. A reading is unusable if it is missing or outside what the room can physically be.
  auto missing_mask = log.isnan();
  auto implausible = log.lt(0.0).logical_or(log.gt(50.0));
  auto unusable = missing_mask.logical_or(implausible);
  show("unusable", unusable);
  std::cout << "unusable readings: " << unusable.sum().item<double>() << " of " << log.numel() << "\n";

  // 2. Replace them with the median of the usable readings, so one fault cannot drag the summary.
  auto usable = log.masked_select(unusable.logical_not());
  show("usable", usable);
  const double centre = usable.median().item<double>();
  // pluss::series::where(condition, a, b) takes `a` where the condition is true and `b` elsewhere.
  auto repaired = pluss::series::where(unusable, centre, log);
  show("repaired", repaired);

  // 3. Report.
  std::cout << "centre (median of usable): " << centre << "\n";
  std::cout << "mean " << repaired.mean().item<double>() << "  std " << repaired.std().item<double>() << "\n";
  std::cout << "min " << repaired.min().item<double>() << "  max " << repaired.max().item<double>() << "\n";
  std::cout << "quartiles " << repaired.quantile(0.25).item<double>() << " " << repaired.quantile(0.5).item<double>() << " " << repaired.quantile(0.75).item<double>() << "\n";
}
