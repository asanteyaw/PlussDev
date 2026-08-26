#include <cstdint>
#include <iostream>
#include <string_view>
#include <vector>

#include <pluss/pluss.h>

namespace {

void show_timeline(std::string_view label, const pluss::Timeline& t) {
  std::cout << "\n== " << label << " ==\n";
  std::cout << pluss::timeline::print(t) << "\n";
}

void show_series(std::string_view label, const pluss::Series& s) {
  std::cout << "\n== " << label << " ==\n";
  std::cout << pluss::series::print(s) << "\n";
}

void show_note(std::string_view label, const pluss::Note& n) {
  std::cout << "\n== " << label << " ==\n";
  std::cout << pluss::print_note(n) << "\n";
}

void show_i64s(std::string_view label, const std::vector<std::int64_t>& xs) {
  std::cout << "\n== " << label << " ==\n";
  std::cout << "[";
  for (std::size_t i = 0; i < xs.size(); ++i) {
    if (i != 0) std::cout << ", ";
    std::cout << xs[i];
  }
  std::cout << "]\n";
}

void show_scalar_string(std::string_view label, const std::string& value) {
  std::cout << "\n== " << label << " ==\n";
  std::cout << value << "\n";
}

void show_scalar_bool(std::string_view label, bool value) {
  std::cout << "\n== " << label << " ==\n";
  std::cout << (value ? "true" : "false") << "\n";
}

} // namespace

int main() {
  const auto base = pluss::timeline::make({
      "2024-11-01T09:00:00Z",
      "2024-11-01T10:15:30Z",
      "2024-12-31T23:59:59Z",
      "2025-01-01T00:00:00Z",
  });

  const auto other = pluss::timeline::make({
      "2024-11-01T09:00:00Z",
      "2024-11-01T10:30:00Z",
      "2024-12-31T20:00:00Z",
      "2025-01-02T00:00:00Z",
  });

  const auto with_nat = pluss::timeline::make({
      "2024-11-01T09:00:00Z",
      "",
      "2024-12-31T23:59:59Z",
      "",
  });

  const auto duplicated = pluss::timeline::make({
      "2024-11-01T10:15:30Z",
      "2024-11-01T09:00:00Z",
      "2024-11-01T10:15:30Z",
      "",
      "2025-01-01T00:00:00Z",
      "",
  });

  const auto sorted = pluss::timeline::make({
      "2024-11-01T09:00:00Z",
      "2024-11-01T10:00:00Z",
      "2024-11-01T11:00:00Z",
      "2024-11-01T11:00:00Z",
  });

  const auto queries = pluss::timeline::make({
      "2024-11-01T08:30:00Z",
      "2024-11-01T11:00:00Z",
      "2024-11-01T12:00:00Z",
  });

  const auto scalar = pluss::timeline::make("2024-11-01T09:00:00Z");
  const auto scalar_nat = pluss::timeline::make("");
  const auto scalar_later = pluss::timeline::make("2024-11-01T10:30:00Z");

  show_timeline("base", base);
  show_timeline("other", other);
  show_timeline("with_nat", with_nat);
  show_timeline("duplicated", duplicated);

  show_timeline("add_duration_scalar_90m",
                scalar.add_duration(pluss::duration::minutes(90)));
  show_timeline("add_duration_base_1h",
                base.add_duration(pluss::duration::hours(1)));
  show_timeline("sub_duration_scalar_1d",
                scalar.sub_duration(pluss::duration::days(1)));
  show_timeline("sub_duration_base_15m",
                base.sub_duration(pluss::duration::minutes(15)));
  show_note("diff_scalar_note_iso",
            pluss::codeh::_compat::invoke<pluss::Note>("to_note_iso",
                                                       scalar_later.diff(scalar)));
  show_series("diff_scalar_total_seconds",
              pluss::codeh::_compat::invoke<pluss::Series>("total",
                                                           scalar_later.diff(scalar),
                                                           {"s"}));
  show_note("diff_vector_note_iso",
            pluss::codeh::_compat::invoke<pluss::Note>("to_note_iso",
                                                       base.diff(other)));
  show_series("diff_vector_total_seconds",
              pluss::codeh::_compat::invoke<pluss::Series>("total",
                                                           base.diff(other),
                                                           {"s"}));

  show_series("eq", base.eq(other));
  show_series("ne", base.ne(other));
  show_series("lt", base.lt(other));
  show_series("le", base.le(other));
  show_series("gt", base.gt(other));
  show_series("ge", base.ge(other));

  show_series("isnat", with_nat.isnat());
  show_series("isvalid", with_nat.isvalid());
  show_series("isvalid_base", base.isvalid());
  show_series("argsort", duplicated.argsort());
  show_series("argsort_base", base.argsort());
  show_scalar_bool("isnat_scalar_false", scalar.isnat_scalar());
  show_scalar_bool("isnat_scalar_true", scalar_nat.isnat_scalar());
  show_scalar_string("isoformat", scalar.isoformat());

  show_note("strftime_ymd_hm", base.strftime("%Y-%m-%d %H:%M"));
  show_note("strftime_weekday_month", base.strftime("%A, %b %d"));
  show_note("to_note_iso", base.to_note_iso());
  show_note("to_note_iso_with_nat", with_nat.to_note_iso());
  show_note("weekday_name", pluss::codeh::_compat::invoke<pluss::Note>("weekday_name", base));
  show_note("month_name", pluss::codeh::_compat::invoke<pluss::Note>("month_name", base));

  show_timeline("floor_D", base.floor("D"));
  show_timeline("ceil_H", base.ceil("H"));
  show_timeline("round_H", base.round("H"));
  show_timeline("normalize", base.normalize());
  show_timeline("date", base.date());
  show_timeline("time", base.time());

  const auto local_wall = pluss::timeline::make({
      "2024-11-01T09:00:00Z",
      "2024-11-01T10:15:30Z",
  });
  const auto ny = local_wall.tz_localize("America/New_York");
  show_timeline("tz_localize_new_york", ny);
  show_timeline("tz_convert_utc", ny.tz_convert("UTC"));
  show_timeline("to_utc", ny.to_utc());

  show_timeline("min", duplicated.min());
  show_timeline("max", duplicated.max());
  show_timeline("sort", duplicated.sort());
  show_timeline("unique", duplicated.unique());
  show_timeline("drop_duplicates", duplicated.drop_duplicates());
  show_timeline("fill_nat_scalar", with_nat.fill_nat(pluss::timeline::make("2024-11-15T00:00:00Z")));
  show_timeline("where_isnat_floorD", with_nat.where(with_nat.isnat(), with_nat.floor("D")));
  show_timeline("add_nanos_1s", base.add_nanos(1'000'000'000LL));

  show_series("searchsorted_left", sorted.searchsorted_left(queries));
  show_series("searchsorted_right", sorted.searchsorted_right(queries));
  show_series("searchsorted_left_existing",
              sorted.searchsorted_left(pluss::timeline::make({
                  "2024-11-01T09:00:00Z",
                  "2024-11-01T11:00:00Z",
              })));
  show_series("searchsorted_right_existing",
              sorted.searchsorted_right(pluss::timeline::make({
                  "2024-11-01T09:00:00Z",
                  "2024-11-01T11:00:00Z",
              })));
  show_i64s("to_epoch_nanos_flat", base.to_epoch_nanos_flat());

  show_series("year", base.year());
  show_series("month", base.month());
  show_series("day", base.day());
  show_series("hour", base.hour());
  show_series("minute", base.minute());
  show_series("second", base.second());
  show_series("nanosecond", base.nanosecond());
  show_series("microsecond", base.microsecond());
  show_series("millisecond", base.millisecond());
  show_series("weekday", base.weekday());
  show_series("dayofyear", base.dayofyear());
  show_series("quarter", base.quarter());
  show_series("week", base.week());

  const auto calendar = pluss::timeline::make({
      "2024-02-29T12:00:00Z",
      "2024-03-01T12:00:00Z",
      "2024-03-31T12:00:00Z",
      "2024-04-01T12:00:00Z",
      "2024-12-31T12:00:00Z",
      "2025-01-01T12:00:00Z",
      "2024-11-02T12:00:00Z",
      "2024-11-04T12:00:00Z",
  });

  show_timeline("calendar", calendar);
  show_series("is_leap_year", calendar.is_leap_year());
  show_series("is_month_start", calendar.is_month_start());
  show_series("is_month_end", calendar.is_month_end());
  show_series("is_quarter_start", calendar.is_quarter_start());
  show_series("is_quarter_end", calendar.is_quarter_end());
  show_series("is_year_start", calendar.is_year_start());
  show_series("is_year_end", calendar.is_year_end());
  show_series("is_weekend", calendar.is_weekend());
  show_series("is_business_day", calendar.is_business_day());
  show_series("is_monday", calendar.is_monday());
  show_series("is_tuesday", calendar.is_tuesday());
  show_series("is_wednesday", calendar.is_wednesday());
  show_series("is_thursday", calendar.is_thursday());
  show_series("is_friday", calendar.is_friday());
  show_series("is_saturday", calendar.is_saturday());
  show_series("is_sunday", calendar.is_sunday());
  show_note("weekday_name_calendar",
            pluss::codeh::_compat::invoke<pluss::Note>("weekday_name",
                                                       calendar));
  show_note("month_name_calendar",
            pluss::codeh::_compat::invoke<pluss::Note>("month_name",
                                                       calendar));

  show_note("to_note_date", base.to_note_date());
  show_note("to_note_time", base.to_note_time());
  show_note("to_note_datetime", base.to_note_datetime());
  show_note("to_note_rfc3339", base.to_note_rfc3339());

  show_timeline("date_range_daily",
                pluss::timeline::date_range(pluss::timeline::make("2026-03-14T00:00:00Z"),
                                  pluss::timeline::make("2026-03-17T00:00:00Z"),
                                  "D"));
  show_timeline("date_range_15min",
                pluss::timeline::date_range(pluss::timeline::make("2026-03-14T09:00:00Z"),
                                  pluss::timeline::make("2026-03-14T10:00:00Z"),
                                  "min",
                                  15));
  show_timeline("date_range_2h",
                pluss::timeline::date_range(pluss::timeline::make("2026-03-14T00:00:00Z"),
                                  pluss::timeline::make("2026-03-14T06:00:00Z"),
                                  "H",
                                  2));

  return 0;
}
