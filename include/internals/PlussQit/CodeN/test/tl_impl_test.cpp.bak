#include <iostream>
#include <string>

#include <pluss/pluss.h>

using namespace pluss;

int main() {
    int exit_code = 0;

    auto check = [&](bool cond, const std::string& msg){
        if (!cond) {
            std::cerr << "FAIL: " << msg << "\n";
            exit_code = 1;
        } else {
            std::cerr << "OK: " << msg << "\n";
        }
    };

    // Construct full timeline via public factories
    Timeline t1 = moment("2020-02-29 12:30:45");
    check(t1.year() == 2020, "year() == 2020");
    check(t1.month() == 2, "month() == 2");
    check(t1.day() == 29, "day() == 29");
    check(t1.hour() == 12, "hour() == 12");
    check(t1.minute() == 30, "minute() == 30");
    check(t1.second() == 45, "second() == 45");
    check(t1.is_leap_year(), "2020 is leap year");

    // to_string and ISO
    std::string s1 = t1.to_string();
    check(s1.find("2020-02-29") != std::string::npos, "to_string contains date");
    check(t1.toISOString().find("2020-02-29T12:30:45") != std::string::npos, "toISOString matches");

    // Parse from string (date + time)
    Timeline t2 = make_timeline(std::string("2020-02-29 12:30:45"));
    check(t2.to_string() == t1.to_string(), "Parsed timeline equals constructed timeline (string factory)");

    // Date-only constructor via factory
    Timeline d = make_timeline(2021, 3, 31);
    check(d.has_date(), "date-only has_date true");
    check(!d.has_time(), "date-only has_time false");
    check(d.to_string().find("2021-03-31") != std::string::npos, "date-only to_string ok");

    // Time-only via factory
    Timeline tm = make_timeline_time(23, 59, 59);
    check(tm.has_time(), "time-only has_time true");
    check(!tm.has_date(), "time-only has_date false");

    // Add days rollover
    Timeline feb29 = make_timeline(2020, 2, 29);
    Timeline mar1 = feb29.add_days(1);
    check(mar1.year() == 2020 && mar1.month() == 3 && mar1.day() == 1, "Feb 29 +1 day -> Mar 1 2020");

    // Months arithmetic
    Timeline jan31 = make_timeline(2021, 1, 31);
    Timeline feb28 = jan31.add_months(1);
    check(feb28.month() == 2, "Jan 31 +1 month -> Feb");

    // Years arithmetic with leap handling
    Timeline leap = make_timeline(2016, 2, 29);
    Timeline nonleap = leap.add_years(1);
    check(nonleap.month() == 2 && nonleap.day() == 28, "Leap day +1 year -> Feb 28 on non-leap year");

    // Difference calculations
    Timeline a = moment("2020-01-01 00:00:00");
    Timeline b = moment("2020-01-02 00:00:00");
    check(a.days_between(b) == 1, "days_between 1 day");
    check(a.hours_between(b) == 24, "hours_between 24 hours");

    // Comparison operators
    check(a < b, "a < b");
    check(b > a, "b > a");
    check(a != b, "a != b");

    // Parsing various formats
    Timeline p1 = moment("March 5, 2022 4:05 pm");
    if (!(p1.year() == 2022 && p1.month() == 3 && p1.day() == 5)) {
        std::cerr << "DIAG p1: to_string='" << p1.to_string() << "' has_date=" << p1.has_date() << " has_time=" << p1.has_time() << "\n";
    }
    check(p1.year() == 2022 && p1.month() == 3 && p1.day() == 5, "Parse 'March 5, 2022'");

    Timeline p2 = make_timeline(std::string("2022-03-05"));
    check(p2.year() == 2022 && p2.month() == 3 && p2.day() == 5, "Parse '2022-03-05'");

    // to_Series conversions (basic smoke test: must not throw)
    try {
        Series s = t1.to_Series();
        (void)s;
        std::cerr << "OK: to_Series() executed\n";
    } catch (...) {
        std::cerr << "FAIL: to_Series() threw exception\n";
        exit_code = 1;
    }

    // --- New pandas-like API tests ---
    // floor/ceil/round
    Timeline p = moment("2021-03-15 13:45:30");
    Timeline pf = p.floor("D");
    check(pf.hour() == 0 && pf.minute() == 0 && pf.second() == 0, "floor to day sets time to midnight");
    Timeline pc = p.ceil("D");
    check(pc.hour() == 0 && pc.minute() == 0 && pc.second() == 0 && pc.day() == 16, "ceil to day moves to next day midnight");
    Timeline pr = moment("2021-03-15 13:29:00").round("H");
    if (pr.hour() != 13) {
        std::cerr << "DIAG pr: to_string='" << pr.to_string() << "' hour=" << pr.hour() << "\n";
    }
    check(pr.hour() == 13, "round to hour stays at 13 for :29 minutes");
    Timeline pr2 = moment("2021-03-15 13:31:00").round("H");
    if (pr2.hour() != 14) {
        std::cerr << "DIAG pr2: to_string='" << pr2.to_string() << "' hour=" << pr2.hour() << "\n";
    }
    check(pr2.hour() == 14, "round to hour moves to 14 for :31 minutes");

    // shift compared to add_* helpers
    Timeline shifted = p.shift(2, "D");
    Timeline added = p.add_days(2);
    check(shifted.year() == added.year() && shifted.month() == added.month() && shifted.day() == added.day(), "shift by days == add_days");

    // date_range generator
    Timeline dr_start = make_timeline(2021,1,1);
    Timeline dr_end = make_timeline(2021,1,5);
    auto rng = Timeline::date_range(dr_start, dr_end, "D", 1);
    if (static_cast<int>(rng.size()) != 5) {
        std::cerr << "DIAG date_range size=" << rng.size();
        if (!rng.empty()) std::cerr << " first='" << rng.front().to_string() << "' last='" << rng.back().to_string() << "'";
        std::cerr << "\n";
    }
    check(static_cast<int>(rng.size()) == 5, "date_range produces expected count");
    check(rng.front().to_string().find("2021-01-01") != std::string::npos, "date_range start matches");

    // month/quarter/week utilities
    Timeline ms = make_timeline(2021,4,1);
    check(ms.is_month_start(), "is_month_start true on 1st");
    Timeline me = make_timeline(2021,4,30);
    check(me.is_month_end(), "is_month_end true on month end");
    Timeline q = make_timeline(2021,4,15);
    check(q.quarter() == 2 && q.is_quarter_start() == false, "quarter utilities");
    Timeline qs = make_timeline(2021,4,1);
    check(qs.is_quarter_start(), "is_quarter_start true on quarter start");
    Timeline qe = make_timeline(2021,6,30);
    check(qe.is_quarter_end(), "is_quarter_end true on quarter end");

    Timeline wk = make_timeline(2021,1,2); // Saturday
    (void)wk; // ensure presence
    check(make_timeline(2021,1,2).is_weekend(), "is_weekend true on Saturday");
    int woy = make_timeline(2021,1,1).week_of_year();
    check(woy >= 0, "week_of_year returns non-negative value");

    // to_period (alias for floor)
    Timeline per = moment("2021-05-20 13:14:15").to_period("M");
    check(per.day() == 1, "to_period month returns first day of month");

    if (exit_code == 0) std::cerr << "All Timeline implementation tests passed.\n";
    else std::cerr << "Some Timeline implementation tests failed.\n";

    return exit_code;
}