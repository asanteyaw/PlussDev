// Plan shifts across time zones: the same instants seen from several cities, shift lengths as Durations, and the day the clocks change.
#include <pluss/pluss.h>

#include <iostream>

static void show(const char *label, const pluss::Note &n) { std::cout << label << ": " << pluss::note::print(n) << "\n"; }

int main() {
  // 1. Wall-clock times are meaningless until you say where they were read. Attach a zone with tz_localize; the result is a real instant.
  auto london_start = pluss::Timeline::from_ymdhms(pluss::make_series({2026, 2026}), pluss::make_series({11, 11}), pluss::make_series({13, 14}),
                                                  pluss::make_series({22, 6}), pluss::make_series({0, 0}), pluss::make_series({0, 0}))
                          .tz_localize("Europe/London");
  show("start, London", london_start.strftime("%a %d %b %H:%M %Z"));

  // 2. The same instants in two other cities (the zones this build knows are UTC, Europe/London, America/New_York and America/Edmonton): tz_convert changes the view, never the moment.
  show("start, New York", london_start.tz_convert("America/New_York").strftime("%a %d %b %H:%M %Z"));
  show("start, Edmonton", london_start.tz_convert("America/Edmonton").strftime("%a %d %b %H:%M %Z"));

  // 3. A shift is eight hours: add a Duration, and measure what is left with a subtraction that yields a Duration.
  auto shift_length = pluss::duration::hours(8);
  auto end = london_start + shift_length;
  show("end, London", end.strftime("%a %d %b %H:%M %Z"));
  const auto worked = (end - london_start).host();
  std::cout << "shift length in hours: " << static_cast<double>(worked[0]) / 3600e9 << "\n";

  // 4. The night the clocks go back (London, 2026-10-25). A civil day and an elapsed day differ: add_days keeps the wall clock,
  //    adding 24 hours keeps the elapsed time.
  auto saturday = pluss::Timeline::from_ymdhms(pluss::make_series(2026), pluss::make_series(10), pluss::make_series(24), pluss::make_series(12),
                                              pluss::make_series(0), pluss::make_series(0))
                      .tz_localize("Europe/London");
  show("Saturday noon", saturday.strftime("%a %d %b %H:%M %Z"));
  show("add_days(1)", saturday.add_days(1).strftime("%a %d %b %H:%M %Z"));
  show("+ 24 hours ", (saturday + pluss::duration::hours(24)).strftime("%a %d %b %H:%M %Z"));
  const auto elapsed = (saturday.add_days(1) - saturday).host();
  std::cout << "hours between the two noons: " << static_cast<double>(elapsed[0]) / 3600e9 << "\n";
}
