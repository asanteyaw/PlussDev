// A support-desk report from raw timestamps: how long each ticket took, which broke the 24-hour promise,
// the typical and worst case, and when in the week trouble arrives.
#include <pluss/pluss.h>

#include <iostream>
#include <vector>

static void show(const char *label, const pluss::Series &s) { std::cout << label << ": " << pluss::series::print(s) << "\n"; }

int main() {
  using pluss::ScalarType;
  // Eight tickets: when each was opened and when it was closed (UTC).
  auto opened = pluss::timeline::from_ymdhms(pluss::make_series({2026, 2026, 2026, 2026, 2026, 2026, 2026, 2026}), pluss::make_series({11, 11, 11, 11, 11, 11, 11, 11}),
                                             pluss::make_series({2, 2, 3, 4, 5, 6, 9, 9}), pluss::make_series({9, 14, 10, 16, 9, 22, 8, 11}),
                                             pluss::make_series({15, 40, 5, 30, 0, 10, 45, 20}), pluss::make_series({0, 0, 0, 0, 0, 0, 0, 0}));
  auto closed = pluss::timeline::from_ymdhms(pluss::make_series({2026, 2026, 2026, 2026, 2026, 2026, 2026, 2026}), pluss::make_series({11, 11, 11, 11, 11, 11, 11, 11}),
                                             pluss::make_series({2, 3, 3, 6, 5, 9, 9, 10}), pluss::make_series({11, 9, 15, 11, 12, 9, 10, 15}),
                                             pluss::make_series({0, 30, 20, 0, 30, 0, 15, 0}), pluss::make_series({0, 0, 0, 0, 0, 0, 0, 0}));

  // 1. Resolution time is closed minus opened: a Duration per ticket. Convert it to hours for reporting.
  auto took = closed - opened;
  const auto nanos = took.host();
  std::vector<double> hours;
  for (const auto ns : nanos) hours.push_back(static_cast<double>(ns) / 3600e9);
  auto resolution = pluss::make_series(hours);
  show("hours to resolve", resolution);

  // 2. The promise is 24 hours: compare the Duration with the limit.
  auto breached = took.gt(pluss::duration::hours(24));
  show("breached the 24 h promise", breached);
  std::cout << "breaches: " << breached.sum().item<double>() << " of " << breached.numel() << "\n";

  // 3. Typical and worst case. The median resists the one very slow ticket; the 90th percentile shows what a bad day looks like.
  std::cout << "mean " << resolution.mean().item<double>() << " h, median " << resolution.median().item<double>() << " h, 90th percentile "
            << resolution.quantile(0.9).item<double>() << " h, worst " << resolution.max().item<double>() << " h\n";

  // 4. When does trouble arrive? Calendar fields of the open time: the weekday (Monday = 0) and the hour of day.
  show("opened on weekday", opened.weekday());
  show("opened at hour", opened.hour());
  auto weekday = opened.weekday().to(ScalarType::Int64);
  show("mean hours to resolve by weekday (Mon..Sun)", resolution.grouped_mean(weekday, 7));
  show("tickets opened outside 09:00-17:00", opened.hour().lt(9).logical_or(opened.hour().ge(17)));
}
