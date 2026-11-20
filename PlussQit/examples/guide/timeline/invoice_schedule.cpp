// Plan and audit a payment schedule: due dates from payment terms (a Duration), weekend adjustment, lateness and ageing.
#include <pluss/pluss.h>

#include <algorithm>
#include <iostream>

static void show(const char *label, const pluss::Timeline &t) { std::cout << label << ": " << pluss::timeline::print(t) << "\n"; }
static void show(const char *label, const pluss::Note &n) { std::cout << label << ": " << pluss::note::print(n) << "\n"; }
static void show(const char *label, const pluss::Series &s) { std::cout << label << ": " << pluss::series::print(s) << "\n"; }

int main() {
  using pluss::ScalarType;
  auto issued = pluss::Timeline::from_ymd(pluss::make_series({2026, 2026, 2026, 2026}), pluss::make_series({9, 10, 10, 11}), pluss::make_series({15, 22, 23, 2}));
  show("issued", issued.strftime("%a %Y-%m-%d"));

  // 1. Payment terms are an elapsed length, a Duration: thirty days.
  auto terms = pluss::duration::days(30);
  auto due = issued + terms;
  show("due", due.strftime("%a %Y-%m-%d"));

  // 2. A due date on a Saturday or Sunday moves to the following Monday. weekday() numbers Monday as 0.
  auto weekday = due.weekday().to(ScalarType::Int64);
  auto push = weekday.eq(5).to(ScalarType::Int64).mul(2).add(weekday.eq(6).to(ScalarType::Int64));
  show("days to push", push);
  auto adjusted = due + pluss::duration::days(push.items<std::int64_t>());
  show("adjusted due", adjusted.strftime("%a %Y-%m-%d"));
  show("on a business day?", adjusted.is_business_day());

  // 3. Payments arrive on these dates. Subtracting two Timelines gives a Duration; comparing it with zero says who was late.
  auto paid = pluss::Timeline::from_ymd(pluss::make_series({2026, 2026, 2026, 2026}), pluss::make_series({10, 11, 11, 11}), pluss::make_series({14, 24, 23, 20}));
  auto delay = paid - adjusted;
  auto late = delay.gt(pluss::duration::days(0));
  show("late?", late);
  const auto nanos = delay.host();   // a Duration crosses to the host as signed nanoseconds
  std::cout << "days late (negative = early):";
  for (const auto ns : nanos) std::cout << " " << static_cast<double>(ns) / 86400e9;
  std::cout << "\n";

  // 4. Ageing: an invoice unpaid at the reporting date is overdue if the reporting date is later than its due date.
  auto reporting = pluss::Timeline::from_ymd(pluss::make_series(2026), pluss::make_series(11), pluss::make_series(25));
  auto age = reporting - adjusted;
  show("overdue at 2026-11-25?", age.gt(pluss::duration::days(0)));
  const auto waits = age.host();
  std::cout << "longest wait since the due date: " << static_cast<double>(*std::max_element(waits.begin(), waits.end())) / 86400e9 << " days\n";
}
