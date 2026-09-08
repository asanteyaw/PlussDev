#include <pluss/pluss.h>

#include <iostream>

int main() {
  const auto leap_day = pluss::timeline::make("2024-02-29T23:45:00Z");
  std::cout << "=== leap_day ===\n"
            << "year=" << leap_day.year_scalar()
            << ", month=" << leap_day.month_scalar()
            << ", day=" << leap_day.day_scalar()
            << ", weekday=" << leap_day.weekday_scalar()
            << ", yearday=" << leap_day.yearday_scalar() << '\n';

  const auto friday = pluss::timeline::make("2021-01-01T12:00:00Z");
  const auto sunday = pluss::timeline::make("2021-01-03T12:00:00Z");
  std::cout << "\n=== weekday_convention ===\n"
            << "Friday=" << friday.weekday_scalar()
            << ", Sunday=" << sunday.weekday_scalar() << '\n';

  const auto utc = pluss::timeline::make("2025-01-01T01:30:00Z");
  const auto edmonton = utc.tz_convert("America/Edmonton");
  std::cout << "\n=== timezone_boundary ===\n"
            << "UTC=" << utc.year_scalar() << '-' << utc.month_scalar() << '-'
            << utc.day_scalar() << '\n'
            << "Edmonton=" << edmonton.year_scalar() << '-'
            << edmonton.month_scalar() << '-' << edmonton.day_scalar() << '\n';

  const auto year_start = pluss::timeline::make("2024-01-01T00:00:00Z");
  const auto year_end = pluss::timeline::make("2024-12-31T23:59:59Z");
  std::cout << "\n=== ordinal_boundaries ===\n"
            << "start=" << year_start.yearday_scalar()
            << ", end=" << year_end.yearday_scalar() << '\n';

  const auto nat = pluss::timeline::make("");
  std::cout << "\n=== absent_instant ===\n"
            << "year=" << nat.year_scalar()
            << ", month=" << nat.month_scalar()
            << ", day=" << nat.day_scalar()
            << ", weekday=" << nat.weekday_scalar()
            << ", yearday=" << nat.yearday_scalar() << '\n';

  const auto one_vector = pluss::timeline::make({"2024-02-29T23:45:00Z"});
  std::cout << "\n=== rank_check ===\n";
  try {
    static_cast<void>(one_vector.year_scalar());
    std::cout << "accepted\n";
  } catch (const std::exception&) {
    std::cout << "one-element vector rejected\n";
  }

  const auto clock = pluss::timeline::make("2025-01-01T01:30:45Z");
  const auto local_clock = clock.tz_convert("America/Edmonton");
  std::cout << "\n=== clock_timezone ===\n"
            << "UTC=" << clock.hour_scalar() << ':' << clock.minute_scalar()
            << ':' << clock.second_scalar() << '\n'
            << "Edmonton=" << local_clock.hour_scalar() << ':'
            << local_clock.minute_scalar() << ':' << local_clock.second_scalar()
            << '\n';

  const auto march_end = pluss::timeline::make("2024-03-31T12:00:00Z");
  const auto april_start = pluss::timeline::make("2024-04-01T12:00:00Z");
  std::cout << "\n=== quarter_boundary ===\n"
            << "March=" << march_end.quarter_scalar()
            << ", April=" << april_start.quarter_scalar() << '\n';

  const auto fraction =
      pluss::timeline::make("2025-01-01T01:30:45.999999999Z");
  std::cout << "\n=== fractional_second ===\n"
            << "second=" << fraction.second_scalar()
            << ", nanos=" << fraction.nanosecond().item<std::int32_t>() << '\n';

  const auto quarter_utc = pluss::timeline::make("2024-04-01T01:00:00Z");
  const auto quarter_local = quarter_utc.tz_convert("America/Edmonton");
  std::cout << "\n=== quarter_timezone ===\n"
            << "UTC=" << quarter_utc.quarter_scalar()
            << ", Edmonton=" << quarter_local.quarter_scalar() << '\n';

  const auto iso_tail = pluss::timeline::make("2021-01-01T12:00:00Z");
  const auto iso_start = pluss::timeline::make("2021-01-04T12:00:00Z");
  const auto next_iso_year = pluss::timeline::make("2024-12-31T12:00:00Z");
  std::cout << "\n=== iso_week_boundaries ===\n"
            << "2021-01-01: year=" << iso_tail.year_scalar()
            << ", week=" << iso_tail.week_scalar() << '\n'
            << "2021-01-04: year=" << iso_start.year_scalar()
            << ", week=" << iso_start.week_scalar() << '\n'
            << "2024-12-31: year=" << next_iso_year.year_scalar()
            << ", week=" << next_iso_year.week_scalar() << '\n';

  std::cout << "\n=== remaining_absent_fields ===\n"
            << "hour=" << nat.hour_scalar()
            << ", minute=" << nat.minute_scalar()
            << ", second=" << nat.second_scalar()
            << ", quarter=" << nat.quarter_scalar()
            << ", week=" << nat.week_scalar() << '\n';

  std::cout << "\n=== remaining_rank_check ===\n";
  try {
    static_cast<void>(one_vector.week_scalar());
    std::cout << "accepted\n";
  } catch (const std::exception&) {
    std::cout << "one-element vector rejected\n";
  }

  const auto common_day = pluss::timeline::make("2023-02-28T12:00:00Z");
  std::cout << "\n=== leap_predicate ===\n" << std::boolalpha
            << "2024=" << leap_day.leap_scalar()
            << ", 2023=" << common_day.leap_scalar() << '\n';

  const auto feb_start = pluss::timeline::make("2024-02-01T12:00:00Z");
  const auto feb_end = pluss::timeline::make("2024-02-29T12:00:00Z");
  std::cout << "\n=== month_boundaries ===\n"
            << "Feb 1: start=" << feb_start.month_start_scalar()
            << ", end=" << feb_start.month_end_scalar() << '\n'
            << "Feb 29: start=" << feb_end.month_start_scalar()
            << ", end=" << feb_end.month_end_scalar() << '\n';

  std::cout << "\n=== quarter_boundaries ===\n"
            << "Mar 31: start=" << march_end.quarter_start_scalar()
            << ", end=" << march_end.quarter_end_scalar() << '\n'
            << "Apr 1: start=" << april_start.quarter_start_scalar()
            << ", end=" << april_start.quarter_end_scalar() << '\n';

  const auto friday_weekend =
      pluss::timeline::make("2024-03-08T12:00:00Z");
  const auto saturday_weekend =
      pluss::timeline::make("2024-03-09T12:00:00Z");
  const auto sunday_weekend =
      pluss::timeline::make("2024-03-10T12:00:00Z");
  std::cout << "\n=== weekend_predicate ===\n"
            << "Friday=" << friday_weekend.is_weekend_scalar()
            << ", Saturday=" << saturday_weekend.is_weekend_scalar()
            << ", Sunday=" << sunday_weekend.is_weekend_scalar() << '\n';

  std::cout << "\n=== boundary_timezone ===\n"
            << "UTC: month_start=" << quarter_utc.month_start_scalar()
            << ", month_end=" << quarter_utc.month_end_scalar()
            << ", quarter_start=" << quarter_utc.quarter_start_scalar()
            << ", quarter_end=" << quarter_utc.quarter_end_scalar() << '\n'
            << "Edmonton: month_start=" << quarter_local.month_start_scalar()
            << ", month_end=" << quarter_local.month_end_scalar()
            << ", quarter_start=" << quarter_local.quarter_start_scalar()
            << ", quarter_end=" << quarter_local.quarter_end_scalar() << '\n';

  const auto leap_utc = pluss::timeline::make("2024-01-01T01:00:00Z");
  const auto common_local = leap_utc.tz_convert("America/Edmonton");
  std::cout << "\n=== leap_timezone ===\n"
            << "UTC=" << leap_utc.leap_scalar()
            << ", Edmonton=" << common_local.leap_scalar() << '\n';

  std::cout << "\n=== absent_predicates ===\n"
            << "leap=" << nat.leap_scalar()
            << ", month_start=" << nat.month_start_scalar()
            << ", month_end=" << nat.month_end_scalar()
            << ", quarter_start=" << nat.quarter_start_scalar()
            << ", quarter_end=" << nat.quarter_end_scalar()
            << ", weekend=" << nat.is_weekend_scalar() << '\n';

  std::cout << "\n=== predicate_rank_check ===\n";
  try {
    static_cast<void>(one_vector.is_weekend_scalar());
    std::cout << "accepted\n";
  } catch (const std::exception&) {
    std::cout << "one-element vector rejected\n";
  }
}
