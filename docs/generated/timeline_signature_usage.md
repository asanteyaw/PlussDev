# Complete Timeline Method Reference: Signatures + Real Calls

```cpp
auto t = pluss::moment({"2024-01-01T00:00:00Z", "2024-01-01T01:00:00Z"});
auto other_t = pluss::moment({"2024-01-02T00:00:00Z", "2024-01-02T01:00:00Z"});
auto dur = pluss::duration_minutes(5);
auto mask = pluss::Series::ones({2}, pluss::ScalarType::Bool);
auto series_vals = pluss::value(std::vector<double>{1.0, 2.0}, {2});
auto year_col = pluss::value(std::vector<std::int64_t>{2024,2024}, {2});
auto month_col = pluss::value(std::vector<std::int64_t>{1,1}, {2});
auto day_col = pluss::value(std::vector<std::int64_t>{10,11}, {2});
auto hour_col = pluss::value(std::vector<std::int64_t>{8,9}, {2});
auto minute_col = pluss::value(std::vector<std::int64_t>{30,45}, {2});
auto second_col = pluss::value(std::vector<std::int64_t>{0,0}, {2});
std::vector<pluss::codeh::Timeline> others{other_t};
```

- Signature: `int year_scalar() const`
  Example: `auto out = t.year_scalar();`
- Signature: `int month_scalar() const`
  Example: `auto out = t.month_scalar();`
- Signature: `int day_scalar() const`
  Example: `auto out = t.day_scalar();`
- Signature: `int hour_scalar() const`
  Example: `auto out = t.hour_scalar();`
- Signature: `int minute_scalar() const`
  Example: `auto out = t.minute_scalar();`
- Signature: `int second_scalar() const`
  Example: `auto out = t.second_scalar();`
- Signature: `int day_of_year_scalar() const`
  Example: `auto out = t.day_of_year_scalar();`
- Signature: `int quarter_scalar() const`
  Example: `auto out = t.quarter_scalar();`
- Signature: `int week_of_year_scalar() const`
  Example: `auto out = t.week_of_year_scalar();`
- Signature: `bool is_leap_year_scalar() const`
  Example: `auto out = t.is_leap_year_scalar();`
- Signature: `bool is_month_start_scalar() const`
  Example: `auto out = t.is_month_start_scalar();`
- Signature: `bool is_month_end_scalar() const`
  Example: `auto out = t.is_month_end_scalar();`
- Signature: `bool is_quarter_start_scalar() const`
  Example: `auto out = t.is_quarter_start_scalar();`
- Signature: `bool is_quarter_end_scalar() const`
  Example: `auto out = t.is_quarter_end_scalar();`
- Signature: `bool is_weekend_scalar() const`
  Example: `auto out = t.is_weekend_scalar();`
- Signature: `std::string weekday_name(bool abbreviated) const`
  Example: `auto out = t.weekday_name(true);`
- Signature: `std::string month_name(bool abbreviated) const`
  Example: `auto out = t.month_name(true);`
- Signature: `Timeline set_year(int year) const`
  Example: `auto out = t.set_year(1);`
- Signature: `Timeline set_month(int month) const`
  Example: `auto out = t.set_month(1);`
- Signature: `Timeline set_day(int day) const`
  Example: `auto out = t.set_day(1);`
- Signature: `Timeline set_hour(int hour) const`
  Example: `auto out = t.set_hour(1);`
- Signature: `Timeline set_minute(int minute) const`
  Example: `auto out = t.set_minute(1);`
- Signature: `Timeline set_second(int second) const`
  Example: `auto out = t.set_second(1);`
- Signature: `std::string format(const std::string& format_str) const`
  Example: `auto out = t.format("%Y-%m-%d");`
- Signature: `std::string toISOString() const`
  Example: `auto out = t.toISOString();`
- Signature: `std::string to_string() const`
  Example: `auto out = t.to_string();`
- Signature: `pluss::codem::Series to_Series() const`
  Example: `auto out = t.to_Series();`
- Signature: `Timeline add_years(int years) const`
  Example: `auto out = t.add_years(1);`
- Signature: `Timeline add_months(int months) const`
  Example: `auto out = t.add_months(1);`
- Signature: `Timeline add_days(int days) const`
  Example: `auto out = t.add_days(1);`
- Signature: `Timeline add_hours(int hours) const`
  Example: `auto out = t.add_hours(1);`
- Signature: `Timeline add_minutes(int minutes) const`
  Example: `auto out = t.add_minutes(1);`
- Signature: `Timeline add_seconds(int seconds) const`
  Example: `auto out = t.add_seconds(1);`
- Signature: `int years_between(const Timeline& other) const`
  Example: `auto out = t.years_between(other_t);`
- Signature: `int months_between(const Timeline& other) const`
  Example: `auto out = t.months_between(other_t);`
- Signature: `int days_between(const Timeline& other) const`
  Example: `auto out = t.days_between(other_t);`
- Signature: `int hours_between(const Timeline& other) const`
  Example: `auto out = t.hours_between(other_t);`
- Signature: `int minutes_between(const Timeline& other) const`
  Example: `auto out = t.minutes_between(other_t);`
- Signature: `int seconds_between(const Timeline& other) const`
  Example: `auto out = t.seconds_between(other_t);`
- Signature: `Timeline concat(const Timeline& other) const`
  Example: `auto out = t.concat(other_t);`
- Signature: `Timeline concat(const std::vector<Timeline>& others) const`
  Example: `auto out = t.concat(others);`
- Signature: `Timeline repeat(std::int64_t times) const`
  Example: `auto out = t.repeat(1);`
- Signature: `Timeline empty_like() const`
  Example: `auto out = t.empty_like();`
- Signature: `Timeline fill_like(int year, int month, int day) const`
  Example: `auto out = t.fill_like(1, 1, 1);`
- Signature: `Timeline fill_like(const std::string& date_str) const`
  Example: `auto out = t.fill_like("demo");`
- Signature: `Timeline shift(int periods, std::string_view freq) const`
  Example: `auto out = t.shift(1, "D");`
- Signature: `Timeline to_period(std::string_view freq) const`
  Example: `auto out = t.to_period("D");`
- Signature: `TimeIndex to_timeindex(bool require_sorted) const`
  Example: `auto out = t.to_timeindex(true);`
- Signature: `pluss::codem::Series epoch_nanos() const`
  Example: `auto out = t.epoch_nanos();`
- Signature: `pluss::codem::Series epoch_micros() const`
  Example: `auto out = t.epoch_micros();`
- Signature: `pluss::codem::Series epoch_millis() const`
  Example: `auto out = t.epoch_millis();`
- Signature: `pluss::codem::Series epoch_seconds() const`
  Example: `auto out = t.epoch_seconds();`
- Signature: `Timeline now()`
  Example: `auto out = pluss::codeh::Timeline::now();`
- Signature: `Timeline today()`
  Example: `auto out = pluss::codeh::Timeline::today();`
- Signature: `Timeline yesterday()`
  Example: `auto out = pluss::codeh::Timeline::yesterday();`
- Signature: `Timeline tomorrow()`
  Example: `auto out = pluss::codeh::Timeline::tomorrow();`
- Signature: `Timeline fromTimestamp(std::int64_t timestamp_seconds)`
  Example: `auto out = pluss::codeh::Timeline::fromTimestamp(1);`
- Signature: `Timeline fromSeries(const pluss::codem::Series& series)`
  Example: `auto out = pluss::codeh::Timeline::fromSeries(series_vals);`
- Signature: `Timeline createFromDateInts(int year, int month, int day)`
  Example: `auto out = pluss::codeh::Timeline::createFromDateInts(1, 1, 1);`
- Signature: `Timeline createFromTimeInts(int hour, int minute, int second)`
  Example: `auto out = pluss::codeh::Timeline::createFromTimeInts(1, 1, 1);`
- Signature: `Timeline add_duration(const Duration& d) const`
  Example: `auto out = t.add_duration(dur);`
- Signature: `Timeline sub_duration(const Duration& d) const`
  Example: `auto out = t.sub_duration(dur);`
- Signature: `Duration diff(const Timeline& other) const`
  Example: `auto out = t.diff(other_t);`
- Signature: `pluss::codem::Series eq(const Timeline& other) const`
  Example: `auto out = t.eq(other_t);`
- Signature: `pluss::codem::Series ne(const Timeline& other) const`
  Example: `auto out = t.ne(other_t);`
- Signature: `pluss::codem::Series lt(const Timeline& other) const`
  Example: `auto out = t.lt(other_t);`
- Signature: `pluss::codem::Series le(const Timeline& other) const`
  Example: `auto out = t.le(other_t);`
- Signature: `pluss::codem::Series gt(const Timeline& other) const`
  Example: `auto out = t.gt(other_t);`
- Signature: `pluss::codem::Series ge(const Timeline& other) const`
  Example: `auto out = t.ge(other_t);`
- Signature: `pluss::codem::Series isnat() const`
  Example: `auto out = t.isnat();`
- Signature: `pluss::codem::Series isvalid() const`
  Example: `auto out = t.isvalid();`
- Signature: `pluss::codem::Series argsort() const`
  Example: `auto out = t.argsort();`
- Signature: `bool isnat_scalar() const`
  Example: `auto out = t.isnat_scalar();`
- Signature: `std::string isoformat() const`
  Example: `auto out = t.isoformat();`
- Signature: `pluss::coden::Note strftime(std::string_view fmt) const`
  Example: `auto out = t.strftime("%Y-%m-%d");`
- Signature: `pluss::coden::Note to_note_iso() const`
  Example: `auto out = t.to_note_iso();`
- Signature: `pluss::coden::Note weekday_name() const`
  Example: `auto out = t.weekday_name();`
- Signature: `pluss::coden::Note month_name() const`
  Example: `auto out = t.month_name();`
- Signature: `Timeline floor(std::string_view unit) const`
  Example: `auto out = t.floor("D");`
- Signature: `Timeline ceil(std::string_view unit) const`
  Example: `auto out = t.ceil("D");`
- Signature: `Timeline round(std::string_view unit) const`
  Example: `auto out = t.round("D");`
- Signature: `Timeline normalize() const`
  Example: `auto out = t.normalize();`
- Signature: `Timeline date() const`
  Example: `auto out = t.date();`
- Signature: `Timeline time() const`
  Example: `auto out = t.time();`
- Signature: `Timeline tz_convert(std::string_view tz_name) const`
  Example: `auto out = t.tz_convert("UTC");`
- Signature: `Timeline tz_localize(std::string_view tz_name, int fold, std::string_view nonexistent) const`
  Example: `auto out = t.tz_localize("UTC", 1, "raise");`
- Signature: `Timeline to_utc() const`
  Example: `auto out = t.to_utc();`
- Signature: `Timeline min() const`
  Example: `auto out = t.min();`
- Signature: `Timeline max() const`
  Example: `auto out = t.max();`
- Signature: `Timeline sort() const`
  Example: `auto out = t.sort();`
- Signature: `Timeline unique() const`
  Example: `auto out = t.unique();`
- Signature: `Timeline drop_duplicates() const`
  Example: `auto out = t.drop_duplicates();`
- Signature: `Timeline fill_nat(const Timeline& value) const`
  Example: `auto out = t.fill_nat(other_t);`
- Signature: `Timeline where(const pluss::codem::Series& mask, const Timeline& other) const`
  Example: `auto out = t.where(mask, other_t);`
- Signature: `Timeline add_nanos(std::int64_t nanos) const`
  Example: `auto out = t.add_nanos(1);`
- Signature: `pluss::codem::Series searchsorted_left(const Timeline& values) const`
  Example: `auto out = t.searchsorted_left(other_t);`
- Signature: `pluss::codem::Series searchsorted_right(const Timeline& values) const`
  Example: `auto out = t.searchsorted_right(other_t);`
- Signature: `std::vector<std::int64_t> to_epoch_nanos_flat() const`
  Example: `auto out = t.to_epoch_nanos_flat();`
- Signature: `pluss::codem::Series year() const`
  Example: `auto out = t.year();`
- Signature: `pluss::codem::Series y() const`
  Example: `auto out = t.y();`
- Signature: `pluss::codem::Series yr() const`
  Example: `auto out = t.yr();`
- Signature: `pluss::codem::Series years() const`
  Example: `auto out = t.years();`
- Signature: `pluss::codem::Series year_() const`
  Example: `auto out = t.year_();`
- Signature: `pluss::codem::Series month() const`
  Example: `auto out = t.month();`
- Signature: `pluss::codem::Series mon() const`
  Example: `auto out = t.mon();`
- Signature: `pluss::codem::Series mo() const`
  Example: `auto out = t.mo();`
- Signature: `pluss::codem::Series months() const`
  Example: `auto out = t.months();`
- Signature: `pluss::codem::Series month_() const`
  Example: `auto out = t.month_();`
- Signature: `pluss::codem::Series day() const`
  Example: `auto out = t.day();`
- Signature: `pluss::codem::Series d() const`
  Example: `auto out = t.d();`
- Signature: `pluss::codem::Series dom() const`
  Example: `auto out = t.dom();`
- Signature: `pluss::codem::Series day_() const`
  Example: `auto out = t.day_();`
- Signature: `pluss::codem::Series days() const`
  Example: `auto out = t.days();`
- Signature: `pluss::codem::Series hour() const`
  Example: `auto out = t.hour();`
- Signature: `pluss::codem::Series h() const`
  Example: `auto out = t.h();`
- Signature: `pluss::codem::Series hr() const`
  Example: `auto out = t.hr();`
- Signature: `pluss::codem::Series hours() const`
  Example: `auto out = t.hours();`
- Signature: `pluss::codem::Series hour_() const`
  Example: `auto out = t.hour_();`
- Signature: `pluss::codem::Series minute() const`
  Example: `auto out = t.minute();`
- Signature: `pluss::codem::Series m() const`
  Example: `auto out = t.m();`
- Signature: `pluss::codem::Series minutes() const`
  Example: `auto out = t.minutes();`
- Signature: `pluss::codem::Series minute_() const`
  Example: `auto out = t.minute_();`
- Signature: `pluss::codem::Series second() const`
  Example: `auto out = t.second();`
- Signature: `pluss::codem::Series sec() const`
  Example: `auto out = t.sec();`
- Signature: `pluss::codem::Series s() const`
  Example: `auto out = t.s();`
- Signature: `pluss::codem::Series seconds() const`
  Example: `auto out = t.seconds();`
- Signature: `pluss::codem::Series second_() const`
  Example: `auto out = t.second_();`
- Signature: `pluss::codem::Series nanosecond() const`
  Example: `auto out = t.nanosecond();`
- Signature: `pluss::codem::Series ns() const`
  Example: `auto out = t.ns();`
- Signature: `pluss::codem::Series nsec() const`
  Example: `auto out = t.nsec();`
- Signature: `pluss::codem::Series nano() const`
  Example: `auto out = t.nano();`
- Signature: `pluss::codem::Series nanoseconds() const`
  Example: `auto out = t.nanoseconds();`
- Signature: `pluss::codem::Series microsecond() const`
  Example: `auto out = t.microsecond();`
- Signature: `pluss::codem::Series us() const`
  Example: `auto out = t.us();`
- Signature: `pluss::codem::Series usec() const`
  Example: `auto out = t.usec();`
- Signature: `pluss::codem::Series micro() const`
  Example: `auto out = t.micro();`
- Signature: `pluss::codem::Series microseconds() const`
  Example: `auto out = t.microseconds();`
- Signature: `pluss::codem::Series millisecond() const`
  Example: `auto out = t.millisecond();`
- Signature: `pluss::codem::Series ms() const`
  Example: `auto out = t.ms();`
- Signature: `pluss::codem::Series msec() const`
  Example: `auto out = t.msec();`
- Signature: `pluss::codem::Series milli() const`
  Example: `auto out = t.milli();`
- Signature: `pluss::codem::Series milliseconds() const`
  Example: `auto out = t.milliseconds();`
- Signature: `pluss::codem::Series weekday() const`
  Example: `auto out = t.weekday();`
- Signature: `pluss::codem::Series dayofweek() const`
  Example: `auto out = t.dayofweek();`
- Signature: `pluss::codem::Series dow() const`
  Example: `auto out = t.dow();`
- Signature: `pluss::codem::Series weekday_() const`
  Example: `auto out = t.weekday_();`
- Signature: `pluss::codem::Series dayofyear() const`
  Example: `auto out = t.dayofyear();`
- Signature: `pluss::codem::Series doy() const`
  Example: `auto out = t.doy();`
- Signature: `pluss::codem::Series day_of_year() const`
  Example: `auto out = t.day_of_year();`
- Signature: `pluss::codem::Series dayofyear_() const`
  Example: `auto out = t.dayofyear_();`
- Signature: `pluss::codem::Series quarter() const`
  Example: `auto out = t.quarter();`
- Signature: `pluss::codem::Series q() const`
  Example: `auto out = t.q();`
- Signature: `pluss::codem::Series qtr() const`
  Example: `auto out = t.qtr();`
- Signature: `pluss::codem::Series quarter_() const`
  Example: `auto out = t.quarter_();`
- Signature: `pluss::codem::Series week() const`
  Example: `auto out = t.week();`
- Signature: `pluss::codem::Series weekofyear() const`
  Example: `auto out = t.weekofyear();`
- Signature: `pluss::codem::Series woy() const`
  Example: `auto out = t.woy();`
- Signature: `pluss::codem::Series week_() const`
  Example: `auto out = t.week_();`
- Signature: `pluss::codem::Series is_leap_year() const`
  Example: `auto out = t.is_leap_year();`
- Signature: `pluss::codem::Series leap_year() const`
  Example: `auto out = t.leap_year();`
- Signature: `pluss::codem::Series isleapyear() const`
  Example: `auto out = t.isleapyear();`
- Signature: `pluss::codem::Series is_leap() const`
  Example: `auto out = t.is_leap();`
- Signature: `pluss::codem::Series is_month_start() const`
  Example: `auto out = t.is_month_start();`
- Signature: `pluss::codem::Series is_month_begin() const`
  Example: `auto out = t.is_month_begin();`
- Signature: `pluss::codem::Series is_monthstart() const`
  Example: `auto out = t.is_monthstart();`
- Signature: `pluss::codem::Series is_month_end() const`
  Example: `auto out = t.is_month_end();`
- Signature: `pluss::codem::Series is_month_finish() const`
  Example: `auto out = t.is_month_finish();`
- Signature: `pluss::codem::Series is_monthend() const`
  Example: `auto out = t.is_monthend();`
- Signature: `pluss::codem::Series is_quarter_start() const`
  Example: `auto out = t.is_quarter_start();`
- Signature: `pluss::codem::Series is_qs() const`
  Example: `auto out = t.is_qs();`
- Signature: `pluss::codem::Series is_quarter_begin() const`
  Example: `auto out = t.is_quarter_begin();`
- Signature: `pluss::codem::Series is_quarterstart() const`
  Example: `auto out = t.is_quarterstart();`
- Signature: `pluss::codem::Series is_quarter_end() const`
  Example: `auto out = t.is_quarter_end();`
- Signature: `pluss::codem::Series is_qe() const`
  Example: `auto out = t.is_qe();`
- Signature: `pluss::codem::Series is_quarter_finish() const`
  Example: `auto out = t.is_quarter_finish();`
- Signature: `pluss::codem::Series is_quarterend() const`
  Example: `auto out = t.is_quarterend();`
- Signature: `pluss::codem::Series is_year_start() const`
  Example: `auto out = t.is_year_start();`
- Signature: `pluss::codem::Series is_year_begin() const`
  Example: `auto out = t.is_year_begin();`
- Signature: `pluss::codem::Series is_yearstart() const`
  Example: `auto out = t.is_yearstart();`
- Signature: `pluss::codem::Series is_year_end() const`
  Example: `auto out = t.is_year_end();`
- Signature: `pluss::codem::Series is_year_finish() const`
  Example: `auto out = t.is_year_finish();`
- Signature: `pluss::codem::Series is_yearend() const`
  Example: `auto out = t.is_yearend();`
- Signature: `pluss::codem::Series is_weekend() const`
  Example: `auto out = t.is_weekend();`
- Signature: `pluss::codem::Series weekend() const`
  Example: `auto out = t.weekend();`
- Signature: `pluss::codem::Series is_sat_or_sun() const`
  Example: `auto out = t.is_sat_or_sun();`
- Signature: `pluss::codem::Series is_business_day() const`
  Example: `auto out = t.is_business_day();`
- Signature: `pluss::codem::Series is_bday() const`
  Example: `auto out = t.is_bday();`
- Signature: `pluss::codem::Series is_businessday() const`
  Example: `auto out = t.is_businessday();`
- Signature: `pluss::codem::Series business_day() const`
  Example: `auto out = t.business_day();`
- Signature: `pluss::codem::Series is_monday() const`
  Example: `auto out = t.is_monday();`
- Signature: `pluss::codem::Series mon_is() const`
  Example: `auto out = t.mon_is();`
- Signature: `pluss::codem::Series is_mon() const`
  Example: `auto out = t.is_mon();`
- Signature: `pluss::codem::Series is_tuesday() const`
  Example: `auto out = t.is_tuesday();`
- Signature: `pluss::codem::Series tue_is() const`
  Example: `auto out = t.tue_is();`
- Signature: `pluss::codem::Series is_tue() const`
  Example: `auto out = t.is_tue();`
- Signature: `pluss::codem::Series is_wednesday() const`
  Example: `auto out = t.is_wednesday();`
- Signature: `pluss::codem::Series wed_is() const`
  Example: `auto out = t.wed_is();`
- Signature: `pluss::codem::Series is_wed() const`
  Example: `auto out = t.is_wed();`
- Signature: `pluss::codem::Series is_thursday() const`
  Example: `auto out = t.is_thursday();`
- Signature: `pluss::codem::Series thu_is() const`
  Example: `auto out = t.thu_is();`
- Signature: `pluss::codem::Series is_thu() const`
  Example: `auto out = t.is_thu();`
- Signature: `pluss::codem::Series is_friday() const`
  Example: `auto out = t.is_friday();`
- Signature: `pluss::codem::Series fri_is() const`
  Example: `auto out = t.fri_is();`
- Signature: `pluss::codem::Series is_fri() const`
  Example: `auto out = t.is_fri();`
- Signature: `pluss::codem::Series is_saturday() const`
  Example: `auto out = t.is_saturday();`
- Signature: `pluss::codem::Series sat_is() const`
  Example: `auto out = t.sat_is();`
- Signature: `pluss::codem::Series is_sat() const`
  Example: `auto out = t.is_sat();`
- Signature: `pluss::codem::Series is_sunday() const`
  Example: `auto out = t.is_sunday();`
- Signature: `pluss::codem::Series sun_is() const`
  Example: `auto out = t.sun_is();`
- Signature: `pluss::codem::Series is_sun() const`
  Example: `auto out = t.is_sun();`
- Signature: `pluss::coden::Note to_note_date() const`
  Example: `auto out = t.to_note_date();`
- Signature: `pluss::coden::Note to_note_date_compact() const`
  Example: `auto out = t.to_note_date_compact();`
- Signature: `pluss::coden::Note to_note_time() const`
  Example: `auto out = t.to_note_time();`
- Signature: `pluss::coden::Note to_note_time_hm() const`
  Example: `auto out = t.to_note_time_hm();`
- Signature: `pluss::coden::Note to_note_datetime() const`
  Example: `auto out = t.to_note_datetime();`
- Signature: `pluss::coden::Note to_note_rfc3339() const`
  Example: `auto out = t.to_note_rfc3339();`
- Signature: `pluss::coden::Note to_note_rfc3339_nano() const`
  Example: `auto out = t.to_note_rfc3339_nano();`
- Signature: `pluss::coden::Note to_note_ym() const`
  Example: `auto out = t.to_note_ym();`
- Signature: `pluss::coden::Note to_note_my() const`
  Example: `auto out = t.to_note_my();`
- Signature: `pluss::coden::Note to_note_md() const`
  Example: `auto out = t.to_note_md();`
- Signature: `pluss::coden::Note to_note_dmy() const`
  Example: `auto out = t.to_note_dmy();`
- Signature: `pluss::coden::Note to_note_Ymd_hm_sp_dash() const`
  Example: `auto out = t.to_note_Ymd_hm_sp_dash();`
- Signature: `pluss::coden::Note to_note_Ymd_hm_t_dash() const`
  Example: `auto out = t.to_note_Ymd_hm_t_dash();`
- Signature: `pluss::coden::Note to_note_Ymd_hms_sp_dash() const`
  Example: `auto out = t.to_note_Ymd_hms_sp_dash();`
- Signature: `pluss::coden::Note to_note_Ymd_hms_t_dash() const`
  Example: `auto out = t.to_note_Ymd_hms_t_dash();`
- Signature: `pluss::coden::Note to_note_dmY_hm_sp_dash() const`
  Example: `auto out = t.to_note_dmY_hm_sp_dash();`
- Signature: `pluss::coden::Note to_note_dmY_hm_t_dash() const`
  Example: `auto out = t.to_note_dmY_hm_t_dash();`
- Signature: `pluss::coden::Note to_note_dmY_hms_sp_dash() const`
  Example: `auto out = t.to_note_dmY_hms_sp_dash();`
- Signature: `pluss::coden::Note to_note_dmY_hms_t_dash() const`
  Example: `auto out = t.to_note_dmY_hms_t_dash();`
- Signature: `pluss::coden::Note to_note_mdY_hm_sp_dash() const`
  Example: `auto out = t.to_note_mdY_hm_sp_dash();`
- Signature: `pluss::coden::Note to_note_mdY_hm_t_dash() const`
  Example: `auto out = t.to_note_mdY_hm_t_dash();`
- Signature: `pluss::coden::Note to_note_mdY_hms_sp_dash() const`
  Example: `auto out = t.to_note_mdY_hms_sp_dash();`
- Signature: `pluss::coden::Note to_note_mdY_hms_t_dash() const`
  Example: `auto out = t.to_note_mdY_hms_t_dash();`
- Signature: `pluss::coden::Note to_note_Ymd_hm_sp_slash() const`
  Example: `auto out = t.to_note_Ymd_hm_sp_slash();`
- Signature: `pluss::coden::Note to_note_Ymd_hm_t_slash() const`
  Example: `auto out = t.to_note_Ymd_hm_t_slash();`
- Signature: `pluss::coden::Note to_note_Ymd_hms_sp_slash() const`
  Example: `auto out = t.to_note_Ymd_hms_sp_slash();`
- Signature: `pluss::coden::Note to_note_Ymd_hms_t_slash() const`
  Example: `auto out = t.to_note_Ymd_hms_t_slash();`
- Signature: `pluss::coden::Note to_note_dmY_hm_sp_slash() const`
  Example: `auto out = t.to_note_dmY_hm_sp_slash();`
- Signature: `pluss::coden::Note to_note_dmY_hm_t_slash() const`
  Example: `auto out = t.to_note_dmY_hm_t_slash();`
- Signature: `pluss::coden::Note to_note_dmY_hms_sp_slash() const`
  Example: `auto out = t.to_note_dmY_hms_sp_slash();`
- Signature: `pluss::coden::Note to_note_dmY_hms_t_slash() const`
  Example: `auto out = t.to_note_dmY_hms_t_slash();`
- Signature: `pluss::coden::Note to_note_mdY_hm_sp_slash() const`
  Example: `auto out = t.to_note_mdY_hm_sp_slash();`
- Signature: `pluss::coden::Note to_note_mdY_hm_t_slash() const`
  Example: `auto out = t.to_note_mdY_hm_t_slash();`
- Signature: `pluss::coden::Note to_note_mdY_hms_sp_slash() const`
  Example: `auto out = t.to_note_mdY_hms_sp_slash();`
- Signature: `pluss::coden::Note to_note_mdY_hms_t_slash() const`
  Example: `auto out = t.to_note_mdY_hms_t_slash();`
- Signature: `pluss::coden::Note to_note_Ymd_hm_sp_dot() const`
  Example: `auto out = t.to_note_Ymd_hm_sp_dot();`
- Signature: `pluss::coden::Note to_note_Ymd_hm_t_dot() const`
  Example: `auto out = t.to_note_Ymd_hm_t_dot();`
- Signature: `pluss::coden::Note to_note_Ymd_hms_sp_dot() const`
  Example: `auto out = t.to_note_Ymd_hms_sp_dot();`
- Signature: `pluss::coden::Note to_note_Ymd_hms_t_dot() const`
  Example: `auto out = t.to_note_Ymd_hms_t_dot();`
- Signature: `pluss::coden::Note to_note_dmY_hm_sp_dot() const`
  Example: `auto out = t.to_note_dmY_hm_sp_dot();`
- Signature: `pluss::coden::Note to_note_dmY_hm_t_dot() const`
  Example: `auto out = t.to_note_dmY_hm_t_dot();`
- Signature: `pluss::coden::Note to_note_dmY_hms_sp_dot() const`
  Example: `auto out = t.to_note_dmY_hms_sp_dot();`
- Signature: `pluss::coden::Note to_note_dmY_hms_t_dot() const`
  Example: `auto out = t.to_note_dmY_hms_t_dot();`
- Signature: `pluss::coden::Note to_note_mdY_hm_sp_dot() const`
  Example: `auto out = t.to_note_mdY_hm_sp_dot();`
- Signature: `pluss::coden::Note to_note_mdY_hm_t_dot() const`
  Example: `auto out = t.to_note_mdY_hm_t_dot();`
- Signature: `pluss::coden::Note to_note_mdY_hms_sp_dot() const`
  Example: `auto out = t.to_note_mdY_hms_sp_dot();`
- Signature: `pluss::coden::Note to_note_mdY_hms_t_dot() const`
  Example: `auto out = t.to_note_mdY_hms_t_dot();`
- Signature: `pluss::coden::Note to_note_date_z() const`
  Example: `auto out = t.to_note_date_z();`
- Signature: `pluss::coden::Note to_note_date_nano() const`
  Example: `auto out = t.to_note_date_nano();`
- Signature: `pluss::coden::Note to_note_date_nano_z() const`
  Example: `auto out = t.to_note_date_nano_z();`
- Signature: `pluss::coden::Note to_note_date_compact_z() const`
  Example: `auto out = t.to_note_date_compact_z();`
- Signature: `pluss::coden::Note to_note_date_compact_nano() const`
  Example: `auto out = t.to_note_date_compact_nano();`
- Signature: `pluss::coden::Note to_note_date_compact_nano_z() const`
  Example: `auto out = t.to_note_date_compact_nano_z();`
- Signature: `pluss::coden::Note to_note_time_z() const`
  Example: `auto out = t.to_note_time_z();`
- Signature: `pluss::coden::Note to_note_time_nano() const`
  Example: `auto out = t.to_note_time_nano();`
- Signature: `pluss::coden::Note to_note_time_nano_z() const`
  Example: `auto out = t.to_note_time_nano_z();`
- Signature: `pluss::coden::Note to_note_time_hm_z() const`
  Example: `auto out = t.to_note_time_hm_z();`
- Signature: `pluss::coden::Note to_note_time_hm_nano() const`
  Example: `auto out = t.to_note_time_hm_nano();`
- Signature: `pluss::coden::Note to_note_time_hm_nano_z() const`
  Example: `auto out = t.to_note_time_hm_nano_z();`
- Signature: `pluss::coden::Note to_note_datetime_z() const`
  Example: `auto out = t.to_note_datetime_z();`
- Signature: `pluss::coden::Note to_note_datetime_nano() const`
  Example: `auto out = t.to_note_datetime_nano();`
- Signature: `pluss::coden::Note to_note_datetime_nano_z() const`
  Example: `auto out = t.to_note_datetime_nano_z();`
- Signature: `pluss::coden::Note to_note_rfc3339_z() const`
  Example: `auto out = t.to_note_rfc3339_z();`
- Signature: `pluss::coden::Note to_note_rfc3339_nano_z() const`
  Example: `auto out = t.to_note_rfc3339_nano_z();`
- Signature: `pluss::coden::Note to_note_rfc3339_nano_nano() const`
  Example: `auto out = t.to_note_rfc3339_nano_nano();`
- Signature: `pluss::coden::Note to_note_rfc3339_nano_nano_z() const`
  Example: `auto out = t.to_note_rfc3339_nano_nano_z();`
- Signature: `pluss::coden::Note to_note_ym_z() const`
  Example: `auto out = t.to_note_ym_z();`
- Signature: `pluss::coden::Note to_note_ym_nano() const`
  Example: `auto out = t.to_note_ym_nano();`
- Signature: `pluss::coden::Note to_note_ym_nano_z() const`
  Example: `auto out = t.to_note_ym_nano_z();`
- Signature: `pluss::coden::Note to_note_my_z() const`
  Example: `auto out = t.to_note_my_z();`
- Signature: `pluss::coden::Note to_note_my_nano() const`
  Example: `auto out = t.to_note_my_nano();`
- Signature: `pluss::coden::Note to_note_my_nano_z() const`
  Example: `auto out = t.to_note_my_nano_z();`
- Signature: `pluss::coden::Note to_note_md_z() const`
  Example: `auto out = t.to_note_md_z();`
- Signature: `pluss::coden::Note to_note_md_nano() const`
  Example: `auto out = t.to_note_md_nano();`
- Signature: `pluss::coden::Note to_note_md_nano_z() const`
  Example: `auto out = t.to_note_md_nano_z();`
- Signature: `pluss::coden::Note to_note_dmy_z() const`
  Example: `auto out = t.to_note_dmy_z();`
- Signature: `pluss::coden::Note to_note_dmy_nano() const`
  Example: `auto out = t.to_note_dmy_nano();`
- Signature: `pluss::coden::Note to_note_dmy_nano_z() const`
  Example: `auto out = t.to_note_dmy_nano_z();`
- Signature: `pluss::coden::Note to_note_Ymd_hm_sp_dash_z() const`
  Example: `auto out = t.to_note_Ymd_hm_sp_dash_z();`
- Signature: `pluss::coden::Note to_note_Ymd_hm_sp_dash_nano() const`
  Example: `auto out = t.to_note_Ymd_hm_sp_dash_nano();`
- Signature: `pluss::coden::Note to_note_Ymd_hm_sp_dash_nano_z() const`
  Example: `auto out = t.to_note_Ymd_hm_sp_dash_nano_z();`
- Signature: `pluss::coden::Note to_note_Ymd_hm_t_dash_z() const`
  Example: `auto out = t.to_note_Ymd_hm_t_dash_z();`
- Signature: `pluss::coden::Note to_note_Ymd_hm_t_dash_nano() const`
  Example: `auto out = t.to_note_Ymd_hm_t_dash_nano();`
- Signature: `pluss::coden::Note to_note_Ymd_hm_t_dash_nano_z() const`
  Example: `auto out = t.to_note_Ymd_hm_t_dash_nano_z();`
- Signature: `pluss::coden::Note to_note_Ymd_hms_sp_dash_z() const`
  Example: `auto out = t.to_note_Ymd_hms_sp_dash_z();`
- Signature: `pluss::coden::Note to_note_Ymd_hms_sp_dash_nano() const`
  Example: `auto out = t.to_note_Ymd_hms_sp_dash_nano();`
- Signature: `pluss::coden::Note to_note_Ymd_hms_sp_dash_nano_z() const`
  Example: `auto out = t.to_note_Ymd_hms_sp_dash_nano_z();`
- Signature: `pluss::coden::Note to_note_Ymd_hms_t_dash_z() const`
  Example: `auto out = t.to_note_Ymd_hms_t_dash_z();`
- Signature: `pluss::coden::Note to_note_Ymd_hms_t_dash_nano() const`
  Example: `auto out = t.to_note_Ymd_hms_t_dash_nano();`
- Signature: `pluss::coden::Note to_note_Ymd_hms_t_dash_nano_z() const`
  Example: `auto out = t.to_note_Ymd_hms_t_dash_nano_z();`
- Signature: `pluss::coden::Note to_note_dmY_hm_sp_dash_z() const`
  Example: `auto out = t.to_note_dmY_hm_sp_dash_z();`
- Signature: `pluss::coden::Note to_note_dmY_hm_sp_dash_nano() const`
  Example: `auto out = t.to_note_dmY_hm_sp_dash_nano();`
- Signature: `pluss::coden::Note to_note_dmY_hm_sp_dash_nano_z() const`
  Example: `auto out = t.to_note_dmY_hm_sp_dash_nano_z();`
- Signature: `pluss::coden::Note to_note_dmY_hm_t_dash_z() const`
  Example: `auto out = t.to_note_dmY_hm_t_dash_z();`
- Signature: `pluss::coden::Note to_note_dmY_hm_t_dash_nano() const`
  Example: `auto out = t.to_note_dmY_hm_t_dash_nano();`
- Signature: `pluss::coden::Note to_note_dmY_hm_t_dash_nano_z() const`
  Example: `auto out = t.to_note_dmY_hm_t_dash_nano_z();`
- Signature: `pluss::coden::Note to_note_dmY_hms_sp_dash_z() const`
  Example: `auto out = t.to_note_dmY_hms_sp_dash_z();`
- Signature: `pluss::coden::Note to_note_dmY_hms_sp_dash_nano() const`
  Example: `auto out = t.to_note_dmY_hms_sp_dash_nano();`
- Signature: `pluss::coden::Note to_note_dmY_hms_sp_dash_nano_z() const`
  Example: `auto out = t.to_note_dmY_hms_sp_dash_nano_z();`
- Signature: `pluss::coden::Note to_note_dmY_hms_t_dash_z() const`
  Example: `auto out = t.to_note_dmY_hms_t_dash_z();`
- Signature: `pluss::coden::Note to_note_dmY_hms_t_dash_nano() const`
  Example: `auto out = t.to_note_dmY_hms_t_dash_nano();`
- Signature: `pluss::coden::Note to_note_dmY_hms_t_dash_nano_z() const`
  Example: `auto out = t.to_note_dmY_hms_t_dash_nano_z();`
- Signature: `pluss::coden::Note to_note_mdY_hm_sp_dash_z() const`
  Example: `auto out = t.to_note_mdY_hm_sp_dash_z();`
- Signature: `pluss::coden::Note to_note_mdY_hm_sp_dash_nano() const`
  Example: `auto out = t.to_note_mdY_hm_sp_dash_nano();`
- Signature: `pluss::coden::Note to_note_mdY_hm_sp_dash_nano_z() const`
  Example: `auto out = t.to_note_mdY_hm_sp_dash_nano_z();`
- Signature: `pluss::coden::Note to_note_mdY_hm_t_dash_z() const`
  Example: `auto out = t.to_note_mdY_hm_t_dash_z();`
- Signature: `pluss::coden::Note to_note_mdY_hm_t_dash_nano() const`
  Example: `auto out = t.to_note_mdY_hm_t_dash_nano();`
- Signature: `pluss::coden::Note to_note_mdY_hm_t_dash_nano_z() const`
  Example: `auto out = t.to_note_mdY_hm_t_dash_nano_z();`
- Signature: `pluss::coden::Note to_note_mdY_hms_sp_dash_z() const`
  Example: `auto out = t.to_note_mdY_hms_sp_dash_z();`
- Signature: `pluss::coden::Note to_note_mdY_hms_sp_dash_nano() const`
  Example: `auto out = t.to_note_mdY_hms_sp_dash_nano();`
- Signature: `pluss::coden::Note to_note_mdY_hms_sp_dash_nano_z() const`
  Example: `auto out = t.to_note_mdY_hms_sp_dash_nano_z();`
- Signature: `pluss::coden::Note to_note_mdY_hms_t_dash_z() const`
  Example: `auto out = t.to_note_mdY_hms_t_dash_z();`
- Signature: `pluss::coden::Note to_note_mdY_hms_t_dash_nano() const`
  Example: `auto out = t.to_note_mdY_hms_t_dash_nano();`
- Signature: `pluss::coden::Note to_note_mdY_hms_t_dash_nano_z() const`
  Example: `auto out = t.to_note_mdY_hms_t_dash_nano_z();`
- Signature: `pluss::coden::Note to_note_Ymd_hm_sp_slash_z() const`
  Example: `auto out = t.to_note_Ymd_hm_sp_slash_z();`
- Signature: `pluss::coden::Note to_note_Ymd_hm_sp_slash_nano() const`
  Example: `auto out = t.to_note_Ymd_hm_sp_slash_nano();`
- Signature: `pluss::coden::Note to_note_Ymd_hm_sp_slash_nano_z() const`
  Example: `auto out = t.to_note_Ymd_hm_sp_slash_nano_z();`
- Signature: `pluss::coden::Note to_note_Ymd_hm_t_slash_z() const`
  Example: `auto out = t.to_note_Ymd_hm_t_slash_z();`
- Signature: `pluss::coden::Note to_note_Ymd_hm_t_slash_nano() const`
  Example: `auto out = t.to_note_Ymd_hm_t_slash_nano();`
- Signature: `pluss::coden::Note to_note_Ymd_hm_t_slash_nano_z() const`
  Example: `auto out = t.to_note_Ymd_hm_t_slash_nano_z();`
- Signature: `pluss::coden::Note to_note_Ymd_hms_sp_slash_z() const`
  Example: `auto out = t.to_note_Ymd_hms_sp_slash_z();`
- Signature: `pluss::coden::Note to_note_Ymd_hms_sp_slash_nano() const`
  Example: `auto out = t.to_note_Ymd_hms_sp_slash_nano();`
- Signature: `pluss::coden::Note to_note_Ymd_hms_sp_slash_nano_z() const`
  Example: `auto out = t.to_note_Ymd_hms_sp_slash_nano_z();`
- Signature: `pluss::coden::Note to_note_Ymd_hms_t_slash_z() const`
  Example: `auto out = t.to_note_Ymd_hms_t_slash_z();`
- Signature: `pluss::coden::Note to_note_Ymd_hms_t_slash_nano() const`
  Example: `auto out = t.to_note_Ymd_hms_t_slash_nano();`
- Signature: `pluss::coden::Note to_note_Ymd_hms_t_slash_nano_z() const`
  Example: `auto out = t.to_note_Ymd_hms_t_slash_nano_z();`
- Signature: `pluss::coden::Note to_note_dmY_hm_sp_slash_z() const`
  Example: `auto out = t.to_note_dmY_hm_sp_slash_z();`
- Signature: `pluss::coden::Note to_note_dmY_hm_sp_slash_nano() const`
  Example: `auto out = t.to_note_dmY_hm_sp_slash_nano();`
- Signature: `pluss::coden::Note to_note_dmY_hm_sp_slash_nano_z() const`
  Example: `auto out = t.to_note_dmY_hm_sp_slash_nano_z();`
- Signature: `pluss::coden::Note to_note_dmY_hm_t_slash_z() const`
  Example: `auto out = t.to_note_dmY_hm_t_slash_z();`
- Signature: `pluss::coden::Note to_note_dmY_hm_t_slash_nano() const`
  Example: `auto out = t.to_note_dmY_hm_t_slash_nano();`
- Signature: `pluss::coden::Note to_note_dmY_hm_t_slash_nano_z() const`
  Example: `auto out = t.to_note_dmY_hm_t_slash_nano_z();`
- Signature: `pluss::coden::Note to_note_dmY_hms_sp_slash_z() const`
  Example: `auto out = t.to_note_dmY_hms_sp_slash_z();`
- Signature: `pluss::coden::Note to_note_dmY_hms_sp_slash_nano() const`
  Example: `auto out = t.to_note_dmY_hms_sp_slash_nano();`
- Signature: `pluss::coden::Note to_note_dmY_hms_sp_slash_nano_z() const`
  Example: `auto out = t.to_note_dmY_hms_sp_slash_nano_z();`
- Signature: `pluss::coden::Note to_note_dmY_hms_t_slash_z() const`
  Example: `auto out = t.to_note_dmY_hms_t_slash_z();`
- Signature: `pluss::coden::Note to_note_dmY_hms_t_slash_nano() const`
  Example: `auto out = t.to_note_dmY_hms_t_slash_nano();`
- Signature: `pluss::coden::Note to_note_dmY_hms_t_slash_nano_z() const`
  Example: `auto out = t.to_note_dmY_hms_t_slash_nano_z();`
- Signature: `pluss::coden::Note to_note_mdY_hm_sp_slash_z() const`
  Example: `auto out = t.to_note_mdY_hm_sp_slash_z();`
- Signature: `pluss::coden::Note to_note_mdY_hm_sp_slash_nano() const`
  Example: `auto out = t.to_note_mdY_hm_sp_slash_nano();`
- Signature: `pluss::coden::Note to_note_mdY_hm_sp_slash_nano_z() const`
  Example: `auto out = t.to_note_mdY_hm_sp_slash_nano_z();`
- Signature: `pluss::coden::Note to_note_mdY_hm_t_slash_z() const`
  Example: `auto out = t.to_note_mdY_hm_t_slash_z();`
- Signature: `pluss::coden::Note to_note_mdY_hm_t_slash_nano() const`
  Example: `auto out = t.to_note_mdY_hm_t_slash_nano();`
- Signature: `pluss::coden::Note to_note_mdY_hm_t_slash_nano_z() const`
  Example: `auto out = t.to_note_mdY_hm_t_slash_nano_z();`
- Signature: `pluss::coden::Note to_note_mdY_hms_sp_slash_z() const`
  Example: `auto out = t.to_note_mdY_hms_sp_slash_z();`
- Signature: `pluss::coden::Note to_note_mdY_hms_sp_slash_nano() const`
  Example: `auto out = t.to_note_mdY_hms_sp_slash_nano();`
- Signature: `pluss::coden::Note to_note_mdY_hms_sp_slash_nano_z() const`
  Example: `auto out = t.to_note_mdY_hms_sp_slash_nano_z();`
- Signature: `pluss::coden::Note to_note_mdY_hms_t_slash_z() const`
  Example: `auto out = t.to_note_mdY_hms_t_slash_z();`
- Signature: `pluss::coden::Note to_note_mdY_hms_t_slash_nano() const`
  Example: `auto out = t.to_note_mdY_hms_t_slash_nano();`
- Signature: `pluss::coden::Note to_note_mdY_hms_t_slash_nano_z() const`
  Example: `auto out = t.to_note_mdY_hms_t_slash_nano_z();`
- Signature: `pluss::coden::Note to_note_Ymd_hm_sp_dot_z() const`
  Example: `auto out = t.to_note_Ymd_hm_sp_dot_z();`
- Signature: `pluss::coden::Note to_note_Ymd_hm_sp_dot_nano() const`
  Example: `auto out = t.to_note_Ymd_hm_sp_dot_nano();`
- Signature: `pluss::coden::Note to_note_Ymd_hm_sp_dot_nano_z() const`
  Example: `auto out = t.to_note_Ymd_hm_sp_dot_nano_z();`
- Signature: `pluss::coden::Note to_note_Ymd_hm_t_dot_z() const`
  Example: `auto out = t.to_note_Ymd_hm_t_dot_z();`
- Signature: `pluss::coden::Note to_note_Ymd_hm_t_dot_nano() const`
  Example: `auto out = t.to_note_Ymd_hm_t_dot_nano();`
- Signature: `pluss::coden::Note to_note_Ymd_hm_t_dot_nano_z() const`
  Example: `auto out = t.to_note_Ymd_hm_t_dot_nano_z();`
- Signature: `pluss::coden::Note to_note_Ymd_hms_sp_dot_z() const`
  Example: `auto out = t.to_note_Ymd_hms_sp_dot_z();`
- Signature: `pluss::coden::Note to_note_Ymd_hms_sp_dot_nano() const`
  Example: `auto out = t.to_note_Ymd_hms_sp_dot_nano();`
- Signature: `pluss::coden::Note to_note_Ymd_hms_sp_dot_nano_z() const`
  Example: `auto out = t.to_note_Ymd_hms_sp_dot_nano_z();`
- Signature: `pluss::coden::Note to_note_Ymd_hms_t_dot_z() const`
  Example: `auto out = t.to_note_Ymd_hms_t_dot_z();`
- Signature: `pluss::coden::Note to_note_Ymd_hms_t_dot_nano() const`
  Example: `auto out = t.to_note_Ymd_hms_t_dot_nano();`
- Signature: `pluss::coden::Note to_note_Ymd_hms_t_dot_nano_z() const`
  Example: `auto out = t.to_note_Ymd_hms_t_dot_nano_z();`
- Signature: `pluss::coden::Note to_note_dmY_hm_sp_dot_z() const`
  Example: `auto out = t.to_note_dmY_hm_sp_dot_z();`
- Signature: `pluss::coden::Note to_note_dmY_hm_sp_dot_nano() const`
  Example: `auto out = t.to_note_dmY_hm_sp_dot_nano();`
- Signature: `pluss::coden::Note to_note_dmY_hm_sp_dot_nano_z() const`
  Example: `auto out = t.to_note_dmY_hm_sp_dot_nano_z();`
- Signature: `pluss::coden::Note to_note_dmY_hm_t_dot_z() const`
  Example: `auto out = t.to_note_dmY_hm_t_dot_z();`
- Signature: `pluss::coden::Note to_note_dmY_hm_t_dot_nano() const`
  Example: `auto out = t.to_note_dmY_hm_t_dot_nano();`
- Signature: `pluss::coden::Note to_note_dmY_hm_t_dot_nano_z() const`
  Example: `auto out = t.to_note_dmY_hm_t_dot_nano_z();`
- Signature: `pluss::coden::Note to_note_dmY_hms_sp_dot_z() const`
  Example: `auto out = t.to_note_dmY_hms_sp_dot_z();`
- Signature: `pluss::coden::Note to_note_dmY_hms_sp_dot_nano() const`
  Example: `auto out = t.to_note_dmY_hms_sp_dot_nano();`
- Signature: `pluss::coden::Note to_note_dmY_hms_sp_dot_nano_z() const`
  Example: `auto out = t.to_note_dmY_hms_sp_dot_nano_z();`
- Signature: `pluss::coden::Note to_note_dmY_hms_t_dot_z() const`
  Example: `auto out = t.to_note_dmY_hms_t_dot_z();`
- Signature: `pluss::coden::Note to_note_dmY_hms_t_dot_nano() const`
  Example: `auto out = t.to_note_dmY_hms_t_dot_nano();`
- Signature: `pluss::coden::Note to_note_dmY_hms_t_dot_nano_z() const`
  Example: `auto out = t.to_note_dmY_hms_t_dot_nano_z();`
- Signature: `pluss::coden::Note to_note_mdY_hm_sp_dot_z() const`
  Example: `auto out = t.to_note_mdY_hm_sp_dot_z();`
- Signature: `pluss::coden::Note to_note_mdY_hm_sp_dot_nano() const`
  Example: `auto out = t.to_note_mdY_hm_sp_dot_nano();`
- Signature: `pluss::coden::Note to_note_mdY_hm_sp_dot_nano_z() const`
  Example: `auto out = t.to_note_mdY_hm_sp_dot_nano_z();`
- Signature: `pluss::coden::Note to_note_mdY_hm_t_dot_z() const`
  Example: `auto out = t.to_note_mdY_hm_t_dot_z();`
- Signature: `pluss::coden::Note to_note_mdY_hm_t_dot_nano() const`
  Example: `auto out = t.to_note_mdY_hm_t_dot_nano();`
- Signature: `pluss::coden::Note to_note_mdY_hm_t_dot_nano_z() const`
  Example: `auto out = t.to_note_mdY_hm_t_dot_nano_z();`
- Signature: `pluss::coden::Note to_note_mdY_hms_sp_dot_z() const`
  Example: `auto out = t.to_note_mdY_hms_sp_dot_z();`
- Signature: `pluss::coden::Note to_note_mdY_hms_sp_dot_nano() const`
  Example: `auto out = t.to_note_mdY_hms_sp_dot_nano();`
- Signature: `pluss::coden::Note to_note_mdY_hms_sp_dot_nano_z() const`
  Example: `auto out = t.to_note_mdY_hms_sp_dot_nano_z();`
- Signature: `pluss::coden::Note to_note_mdY_hms_t_dot_z() const`
  Example: `auto out = t.to_note_mdY_hms_t_dot_z();`
- Signature: `pluss::coden::Note to_note_mdY_hms_t_dot_nano() const`
  Example: `auto out = t.to_note_mdY_hms_t_dot_nano();`
- Signature: `pluss::coden::Note to_note_mdY_hms_t_dot_nano_z() const`
  Example: `auto out = t.to_note_mdY_hms_t_dot_nano_z();`
- Signature: `pluss::coden::Note to_note_weekday() const`
  Example: `auto out = t.to_note_weekday();`
- Signature: `pluss::coden::Note to_note_weekday_long() const`
  Example: `auto out = t.to_note_weekday_long();`
- Signature: `pluss::coden::Note to_note_month_name() const`
  Example: `auto out = t.to_note_month_name();`
- Signature: `pluss::coden::Note to_note_month_name_long() const`
  Example: `auto out = t.to_note_month_name_long();`
- Signature: `pluss::coden::Note to_note_month_year_name() const`
  Example: `auto out = t.to_note_month_year_name();`
- Signature: `pluss::coden::Note to_note_iso_week() const`
  Example: `auto out = t.to_note_iso_week();`
- Signature: `pluss::coden::Note to_note_iso_week_compact() const`
  Example: `auto out = t.to_note_iso_week_compact();`
- Signature: `Timeline createFromDateTimeInts(int year, int month, int day, int hour, int minute, int second)`
  Example: `auto out = pluss::codeh::Timeline::createFromDateTimeInts(2024, 1, 10, 8, 30, 0);`
- Signature: `std::vector<Timeline> date_range(const Timeline& start, const Timeline& end, std::string_view freq = "D", int step = 1)`
  Example: `auto out = pluss::codeh::Timeline::date_range(t, other_t, "D", 1);`
- Signature: `Timeline from_ymd(const pluss::codem::Series& year, const pluss::codem::Series& month, const pluss::codem::Series& day, std::string_view tz_name = "UTC")`
  Example: `auto out = pluss::codeh::Timeline::from_ymd(year_col, month_col, day_col, "UTC");`
- Signature: `Timeline from_ymdhms(const pluss::codem::Series& year, const pluss::codem::Series& month, const pluss::codem::Series& day, const pluss::codem::Series& hour, const pluss::codem::Series& minute, const pluss::codem::Series& second, std::string_view tz_name = "UTC")`
  Example: `auto out = pluss::codeh::Timeline::from_ymdhms(year_col, month_col, day_col, hour_col, minute_col, second_col, "UTC");`
- Signature: `Timeline from_epoch_seconds(const pluss::codem::Series& epoch_seconds, std::string_view tz_name = "UTC")`
  Example: `auto out = pluss::codeh::Timeline::from_epoch_seconds(series_vals, "UTC");`
- Signature: `Timeline from_epoch_millis(const pluss::codem::Series& epoch_millis, std::string_view tz_name = "UTC")`
  Example: `auto out = pluss::codeh::Timeline::from_epoch_millis(series_vals, "UTC");`
- Signature: `Timeline from_epoch_micros(const pluss::codem::Series& epoch_micros, std::string_view tz_name = "UTC")`
  Example: `auto out = pluss::codeh::Timeline::from_epoch_micros(series_vals, "UTC");`
