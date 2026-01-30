/*
 * End-user usage examples for `pluss::Timeline`.
 * This file demonstrates the preferred public API: `make_timeline(...)` and `moment(...)`.
 */

#include <iostream>
#include <pluss/pluss.h>

using namespace pluss;

int main() {
	std::cout << "pluss::Timeline — usage examples\n\n";

	// 1) Parse a full date-time string (recommended for literals)
	auto t = moment("2020-02-29 12:30:45");
	std::cout << "Parsed: " << t.to_string() << "\n";
	std::cout << "ISO:    " << t.toISOString() << "\n\n";

	// 2) Create date-only values
	auto d = make_timeline(2021, 3, 31);
	std::cout << "Date-only: " << d.to_string() << " (has_time=" << d.has_time() << ")\n\n";

	// 3) Create time-only values
	auto tm = make_timeline_time(23, 59, 59);
	std::cout << "Time-only: " << tm.to_string() << " (has_date=" << tm.has_date() << ")\n\n";

	// 4) Arithmetic
	auto next_day = t.add_days(1);
	std::cout << "Add 1 day: " << next_day.to_string() << "\n";
	std::cout << "Add 2 months: " << t.add_months(2).to_string() << "\n\n";

	// 5) Differences and comparisons
	auto a = moment("2020-01-01 00:00:00");
	auto b = moment("2020-01-02 12:00:00");
	std::cout << "Days between: " << a.days_between(b) << "\n";
	std::cout << "Hours between: " << a.hours_between(b) << "\n";
	std::cout << "a < b? " << (a < b) << "\n\n";

	// 6) Conversions to `Series` (for interoperability)
	Series s = t.to_Series();
	std::cout << "to_Series() produced a Series (dtype/sizes may vary), dims=" << s.dim() << "\n";

	// --- Pandas-like examples ---
	std::cout << "\nPandas-like examples:\n";
	auto pf = moment("2021-03-15 13:45:30").floor("D");
	std::cout << "floor(D): " << pf.to_string() << "\n";
	auto pc = moment("2021-03-15 13:45:30").ceil("D");
	std::cout << "ceil(D):  " << pc.to_string() << "\n";
	auto pr = moment("2021-03-15 13:31:00").round("H");
	std::cout << "round(H): " << pr.to_string() << "\n";
	std::cout << "shift +2 days: " << moment("2021-03-15 00:00:00").shift(2, "D").to_string() << "\n";
	auto rng = Timeline::date_range(make_timeline(2021,1,1), make_timeline(2021,1,4), "D");
	std::cout << "date_range(2021-01-01,2021-01-04): count=" << rng.size() << " first=" << rng.front().to_string() << "\n";
	std::cout << "is_month_start(2021-04-01): " << make_timeline(2021,4,1).is_month_start() << "\n";
	std::cout << "quarter(2021-04-15): " << make_timeline(2021,4,15).quarter() << "\n";


	std::cout << "\nNotes:\n";
	std::cout << " - Prefer `moment(...)` for parsing textual date/time literals.\n";
	std::cout << " - Use `make_timeline(year,month,day)` or `make_timeline_time(h,m,s)` for programmatic construction.\n";
	std::cout << " - Timeline objects are immutable-like; arithmetic returns new Timeline instances.\n";

	return 0;
}

