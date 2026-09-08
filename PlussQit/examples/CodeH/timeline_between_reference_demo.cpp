#include <pluss/pluss.h>

#include <iostream>
#include <string_view>

template <class Fn>
void show_rejection(std::string_view label, Fn&& fn) {
  try {
    fn();
  } catch (const pluss::Error&) {
    std::cout << label << "=rejected\n";
  }
}

int main() {
  const auto month_start = pluss::timeline::make("2023-01-31T10:00:00Z");
  const auto month_anniversary = pluss::timeline::make("2023-02-28T10:00:00Z");
  const auto before_month = pluss::timeline::make("2023-02-28T09:59:59Z");
  std::cout << "months_complete="
            << month_start.months_between(month_anniversary) << '\n'
            << "months_incomplete=" << month_start.months_between(before_month)
            << '\n'
            << "months_reverse="
            << month_anniversary.months_between(month_start) << '\n';

  const auto leap = pluss::timeline::make("2024-02-29T12:00:00Z");
  const auto year_anniversary = pluss::timeline::make("2025-02-28T12:00:00Z");
  const auto before_year = pluss::timeline::make("2025-02-28T11:59:59Z");
  std::cout << "years_complete=" << leap.years_between(year_anniversary) << '\n'
            << "years_incomplete=" << leap.years_between(before_year) << '\n'
            << "years_reverse=" << year_anniversary.years_between(leap) << '\n';

  const auto start = pluss::timeline::make("2024-01-01T00:00:00Z");
  const auto end = pluss::timeline::make("2024-01-02T02:03:04.500000000Z");
  std::cout << "days=" << start.days_between(end) << '\n'
            << "hours=" << start.hours_between(end) << '\n'
            << "minutes=" << start.minutes_between(end) << '\n'
            << "seconds=" << start.seconds_between(end) << '\n'
            << "reverse_days=" << end.days_between(start) << '\n'
            << "reverse_seconds=" << end.seconds_between(start) << '\n';

  const auto before_dst = pluss::timeline::make("2024-03-09T10:30:00Z")
                              .tz_convert("America/Edmonton");
  const auto next_civil_day = before_dst.add_days(1);
  std::cout << "civil_day_elapsed_hours="
            << before_dst.hours_between(next_civil_day) << '\n'
            << "civil_day_complete_elapsed_days="
            << before_dst.days_between(next_civil_day) << '\n'
            << "twenty_four_hours_complete_days="
            << before_dst.days_between(before_dst.add_hours(24)) << '\n';

  const auto early = pluss::timeline::make("1700-01-01T00:00:00Z");
  const auto late = pluss::timeline::make("2200-01-01T00:00:00Z");
  std::cout << "wide_seconds=" << early.seconds_between(late) << '\n'
            << "wide_seconds_reverse=" << late.seconds_between(early) << '\n';

  const auto same_in_zone = start.tz_convert("America/Edmonton");
  std::cout << "same_instant_seconds="
            << start.seconds_between(same_in_zone) << '\n';
  show_rejection("calendar_timezone_mismatch", [&] {
    (void)start.months_between(same_in_zone);
  });
  show_rejection("between_nat", [&] {
    (void)start.seconds_between(pluss::timeline::make(""));
  });
  show_rejection("between_vector", [&] {
    (void)start.days_between(
        pluss::timeline::make({"2024-01-01", "2024-01-02"}));
  });
}
