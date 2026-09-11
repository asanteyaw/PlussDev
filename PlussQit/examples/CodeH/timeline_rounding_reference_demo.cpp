#include <pluss/pluss.h>

#include <iostream>
#include <string_view>

namespace {
void show(std::string_view label, const pluss::Timeline& value) {
  std::cout << "\n=== " << label << " ===\n"
            << pluss::timeline::print(value) << '\n';
}
}

int main() {
  const auto values = pluss::timeline::make({
      "1969-12-31T23:29:30.123456789Z",
      "1969-12-31T23:30:00Z",
      "1970-01-01T00:30:00Z",
      "1970-01-01T01:30:00Z",
      "",
  });
  show("floor_hour", values.floor("H"));
  show("ceil_hour", values.ceil("H"));
  show("round_hour_half_even", values.round("H"));

  const auto units = pluss::timeline::make(
      "2024-06-15T12:34:56.789123456Z");
  show("floor_second", units.floor("S"));
  show("floor_minute", units.floor("min"));
  show("floor_day", units.floor("D"));

  const auto edmonton = pluss::timeline::make(
      "2024-03-10T18:00:00Z").tz_convert("America/Edmonton");
  show("dst_date", edmonton.date());

  const auto folds = pluss::timeline::make({
      "2024-11-03T07:45:00Z",
      "2024-11-03T08:45:00Z",
  }).tz_convert("America/Edmonton");
  show("fold_floor_hour", folds.floor("H"));

  const auto clocks = pluss::timeline::make({
      "2024-07-15T18:34:56.123456789Z", "",
  }).tz_convert("America/Edmonton");
  show("time_anchor", clocks.time());
  show("date_projection", clocks.date());
  show("date_factory_projection", pluss::timeline::date(clocks));
  show("time_factory_projection", pluss::timeline::time(clocks));
  const auto schedule = pluss::timeline::make({
      {"2024-01-01T00:00:00Z", "2024-04-01T00:00:00Z"},
      {"2024-07-01T00:00:00Z", "2024-10-01T00:00:00Z"},
  });
  show("schedule_date_factory", pluss::timeline::date(schedule));
  show("schedule_time_factory", pluss::timeline::time(schedule));
  const auto precise_clock = pluss::timeline::make(
      "2024-07-15T18:34:56.123456789Z").tz_convert("America/Edmonton");
  std::cout << "\n=== time_anchor_precise ===\n"
            << precise_clock.time().isoformat() << '\n';

  const auto matrix = pluss::timeline::make({
      {"2024-01-01T00:29:00Z", "2024-01-01T00:31:00Z"},
      {"2024-01-01T01:29:00Z", ""},
  });
  show("matrix_round", matrix.round("H"));

  try {
    (void)units.floor("week");
  } catch (const std::exception&) {
    std::cout << "\n=== invalid_unit ===\nrejected\n";
  }

  const auto spring = pluss::timeline::make(
      "2024-03-10T08:30:00Z").tz_convert("America/Edmonton");
  try {
    (void)spring.ceil("H");
  } catch (const std::exception&) {
    std::cout << "\n=== nonexistent_boundary ===\nrejected\n";
  }
}
