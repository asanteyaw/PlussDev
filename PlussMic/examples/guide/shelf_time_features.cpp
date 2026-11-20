// Turn a column of timestamps and a numeric stream into model-ready features: calendar fields, a lag, a rolling mean and a daily return.
#include <pluss/pluss.h>

#include <iostream>

using pluss::table::Shelf;

static void show(const char *label, const Shelf &s) { std::cout << label << "\n" << s.to_string() << "\n"; }

int main() {
  Shelf when(pluss::make_timeline({"2026-11-02T09:00:00Z", "2026-11-03T09:00:00Z", "2026-11-04T09:00:00Z", "2026-11-05T09:00:00Z",
                                    "2026-11-06T09:00:00Z", "2026-11-07T09:00:00Z", "2026-11-09T09:00:00Z"}));
  Shelf close(pluss::make_series({100.0, 102.0, 101.0, 105.0, 107.0, 106.0, 110.0}));

  // 1. Calendar features from the timestamps: weekday name, whether it is a weekend, the month.
  show("weekday", when.tl().weekday_name());
  show("weekend?", when.tl().weekend());
  show("month", when.tl().month());

  // 2. Features of the numeric stream: yesterday's value, the change, and the one-day return.
  show("lag 1", close.ts().lag(1));
  show("return", close.ts().returns(1));

  // 3. A smoothed value: the mean of the last three observations (the first positions use the observations available so far).
  show("rolling mean (3)", close.ts().rolling(3));
}
