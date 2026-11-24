// Subscription billing from dates alone: renewal dates that survive short months, the days left in a paid period,
// and a prorated refund for a cancellation part-way through.
#include <pluss/pluss.h>

#include <iostream>

static void show(const char *label, const pluss::Note &n) { std::cout << label << ": " << pluss::note::print(n) << "\n"; }
static void show(const char *label, const pluss::Series &s) { std::cout << label << ": " << pluss::series::print(s) << "\n"; }

int main() {
  // Three customers who started on the 31st, the 30th and the 15th of different months.
  auto started = pluss::timeline::from_ymd(pluss::make_series({2026, 2026, 2026}), pluss::make_series({1, 3, 8}), pluss::make_series({31, 30, 15}));
  show("started", started.strftime("%a %d %b %Y"));

  // 1. A monthly plan renews on the same day of the month. add_months is a calendar rule: when the month is too short the date
  //    clamps to the month's last day, instead of spilling into the next month as "add 30 days" would.
  show("first renewal ", started.add_months(1).strftime("%a %d %b %Y"));
  show("second renewal", started.add_months(2).strftime("%a %d %b %Y"));
  show("30 days later  ", (started + pluss::duration::days(30)).strftime("%a %d %b %Y"));

  // 2. A second group of customers who started in September on the 30th, the 26th and the 15th. As of 2026-12-10 each is in the
  //    period that began at the renewal two months after the start and ends at the renewal three months after it.
  auto subscribed = pluss::timeline::from_ymd(pluss::make_series({2026, 2026, 2026}), pluss::make_series({9, 9, 9}), pluss::make_series({30, 26, 15}));
  auto today = pluss::timeline::from_ymd(pluss::make_series({2026, 2026, 2026}), pluss::make_series({12, 12, 12}), pluss::make_series({10, 10, 10}));
  auto last_renewal = subscribed.add_months(2);
  auto next_renewal = subscribed.add_months(3);
  show("period began", last_renewal.strftime("%a %d %b %Y"));
  show("next renewal", next_renewal.strftime("%a %d %b %Y"));
  auto left = next_renewal - today;                    // a Duration per customer
  const auto nanos = left.host();
  std::cout << "days left in the period:";
  for (const auto ns : nanos) std::cout << " " << static_cast<double>(ns) / 86400e9;
  std::cout << "\n";

  // 3. Prorated refund for cancelling today: the unused share of the period price. The period runs from the last renewal to the next.
  auto period = next_renewal - last_renewal;
  const auto period_nanos = period.host();
  const double price = 30.0;
  std::cout << "refund for cancelling today:";
  for (std::size_t i = 0; i < nanos.size(); ++i) std::cout << " " << price * static_cast<double>(nanos[i]) / static_cast<double>(period_nanos[i]);
  std::cout << "\n";

  // 4. Calendar flags for operations: which renewals land on a month end, and which charges fall on a weekend.
  show("second renewal of the first group on a month end?", started.add_months(2).is_month_end());
  show("next renewal of the second group on a weekend?", next_renewal.is_weekend());
}
