#include <pluss/pluss.h>

#include <iostream>
#include <string_view>

namespace {
void show(std::string_view label, const pluss::Timeline& value) {
  auto options = pluss::timeline::FormatOptions{};
  options.pattern = "%FT%T.%f%z";
  std::cout << "\n=== " << label << " ===\n"
            << pluss::note::print(pluss::timeline::format(value, options)) << '\n';
}

template <class Fn>
void show_rejection(std::string_view label, std::string_view message, Fn&& fn) {
  try {
    fn();
  } catch (const pluss::Error&) {
    std::cout << "\n=== " << label << " ===\n" << message << '\n';
  }
}
} // namespace

int main() {
  const auto base = pluss::timeline::make({
      {"2024-01-15T23:58:59.123456789Z", "2024-06-20T04:05:06.000000004Z"},
      {"2024-09-28T12:30:45.900000000Z", "2024-12-01T00:00:00.000000001Z"},
  });
  show("base", base);
  show("set_year", base.set_year(2030));
  show("set_month", base.set_month(2));
  show("set_day", base.set_day(1));
  show("set_hour", base.set_hour(6));
  show("set_minute", base.set_minute(15));
  show("set_second", base.set_second(0));

  show("set_month_is_replacement",
       pluss::timeline::make("2024-11-10T09:30:00Z").set_month(2));
  show("composed_clock", pluss::timeline::make("2024-08-19T21:47:36.250000001Z")
                             .set_hour(9).set_minute(30).set_second(0));

  const auto second_source = pluss::timeline::make("2024-08-19T09:30:42.750000003Z");
  show("set_second_preserves_fraction", second_source.set_second(0));
  show("floor_minute_clears_fraction", second_source.floor("min"));

  const auto ny = pluss::timeline::make("2024-07-01T16:45:00Z")
                      .tz_convert("America/New_York");
  show("set_local_hour", ny.set_hour(9));

  const auto with_absent = pluss::timeline::from_epoch({
      1'704'067'200'123'456'789LL, pluss::Timeline::kNaT});
  show("absent_propagates", with_absent.set_minute(30));

  show_rejection("invalid_leap_replacement", "invalid calendar date rejected", [] {
    (void)pluss::timeline::make("2024-02-29T12:00:00Z").set_year(2023);
  });
  show_rejection("invalid_day_for_month", "day is not valid for every month", [] {
    (void)pluss::timeline::make({"2024-01-10T00:00:00Z",
                                "2024-02-10T00:00:00Z"}).set_day(30);
  });
  show_rejection("invalid_month_day", "day does not exist in requested month", [] {
    (void)pluss::timeline::make("2024-05-31T08:00:00Z").set_month(2);
  });
  show_rejection("nonexistent_local_hour", "nonexistent local time rejected", [] {
    const auto local = pluss::timeline::make("2024-03-10T06:30:00Z")
                           .tz_convert("America/New_York");
    (void)local.set_hour(2);
  });
}
