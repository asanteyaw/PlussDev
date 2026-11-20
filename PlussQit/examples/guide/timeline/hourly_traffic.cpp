// Turn raw event timestamps into a traffic report: total per hour, a rolling average on the clock, and "what was the latest reading at...?".
#include <pluss/pluss.h>

#include <iostream>

static void show(const char *label, const pluss::Note &n) { std::cout << label << ": " << pluss::note::print(n) << "\n"; }
static void show(const char *label, const pluss::Series &s) { std::cout << label << ": " << pluss::series::print(s) << "\n"; }

int main() {
  // Request counts logged at irregular moments, as seconds since the epoch, starting at 2026-11-13 09:00 UTC.
  const std::int64_t start = 1794560400;
  auto stamps = pluss::make_series({0, 600, 1500, 2400, 3500, 3700, 4300, 5400, 7000, 7300, 9000, 10100}).add(start);
  auto count = pluss::make_series({12.0, 18.0, 9.0, 22.0, 15.0, 30.0, 25.0, 11.0, 40.0, 35.0, 8.0, 14.0});
  auto when = pluss::Timeline::from_epoch_seconds(stamps);
  show("first events", when.strftime("%H:%M"));

  // 1. A Timeline used as an axis: index() checks it is one-dimensional, present and sorted and unlocks the ordered operations.
  auto axis = when.index();

  // 2. Total requests per clock hour: resample sums the counts that fall inside each hour-wide bin.
  auto hourly = axis.resample_sum(count, "1h");
  show("requests per hour", hourly);
  std::cout << "grand total " << count.sum().item<double>() << ", sum of the hours " << hourly.sum().item<double>() << "\n";

  // 3. A rolling mean over the previous 30 minutes, measured on the clock: it adapts to the irregular spacing.
  auto smooth = axis.rolling_mean(count, pluss::duration::minutes(30));
  show("30-minute rolling mean", smooth);

  // 4. As-of lookup: the last event at or before a given moment, for an event that happens later than the data starts.
  auto query = pluss::Timeline::from_epoch_seconds(pluss::make_series(start + 4000));
  const auto position = axis.asof(query);
  const auto label = when.strftime("%H:%M:%S").take({position}).host();
  std::cout << "latest event at or before 10:06:40 is number " << position << ", at " << label.vector()[0] << "\n";
}
