#pragma once

#include <internals/core/include/b10/deets/operations_fwd.h>

#include <pluss/Timeline.h>

#include <string>

// Timeline privileged construction lives here (separate from operations.h) to avoid
// circular includes: Timeline's header needs series_gen for its inline helpers,
// but tl_gen needs the full Timeline definition.

namespace b10::detail {

struct TimelineSpec final {
	enum class Kind {
		Now,
		FromString,
		Date,
		Time,
		DateTime,
		FromSeriesPair,
		FromSeries,
	};

	Kind kind{Kind::Now};
	std::string text;
	int year{0};
	int month{0};
	int day{0};
	int hour{0};
	int minute{0};
	int second{0};
	const ::pluss::Series* date_series{nullptr};
	const ::pluss::Series* time_series{nullptr};
	const ::pluss::Series* series{nullptr};

	static TimelineSpec now() { return TimelineSpec{}; }
	static TimelineSpec from_string(std::string s) {
		TimelineSpec spec;
		spec.kind = Kind::FromString;
		spec.text = std::move(s);
		return spec;
	}
	static TimelineSpec date(int y, int m, int d) {
		TimelineSpec spec;
		spec.kind = Kind::Date;
		spec.year = y;
		spec.month = m;
		spec.day = d;
		return spec;
	}
	static TimelineSpec time(int h, int m, int s) {
		TimelineSpec spec;
		spec.kind = Kind::Time;
		spec.hour = h;
		spec.minute = m;
		spec.second = s;
		return spec;
	}
	static TimelineSpec date_time(int y, int mo, int d, int h, int mi, int s) {
		TimelineSpec spec;
		spec.kind = Kind::DateTime;
		spec.year = y;
		spec.month = mo;
		spec.day = d;
		spec.hour = h;
		spec.minute = mi;
		spec.second = s;
		return spec;
	}
	static TimelineSpec from_series_pair(const ::pluss::Series& ds, const ::pluss::Series& ts) {
		TimelineSpec spec;
		spec.kind = Kind::FromSeriesPair;
		spec.date_series = &ds;
		spec.time_series = &ts;
		return spec;
	}
	static TimelineSpec from_series(const ::pluss::Series& s) {
		TimelineSpec spec;
		spec.kind = Kind::FromSeries;
		spec.series = &s;
		return spec;
	}
};

// Single friend-gated Timeline construction entry point.
inline ::pluss::Timeline tl_gen(const TimelineSpec& spec) {
	switch (spec.kind) {
		case TimelineSpec::Kind::Now:
			return ::pluss::Timeline();
		case TimelineSpec::Kind::FromString:
			return ::pluss::Timeline(spec.text);
		case TimelineSpec::Kind::Date:
			return ::pluss::Timeline::createFromDateInts(spec.year, spec.month, spec.day);
		case TimelineSpec::Kind::Time:
			return ::pluss::Timeline::createFromTimeInts(spec.hour, spec.minute, spec.second);
		case TimelineSpec::Kind::DateTime:
			return ::pluss::Timeline::createFromDateTimeInts(
				spec.year, spec.month, spec.day, spec.hour, spec.minute, spec.second);
		case TimelineSpec::Kind::FromSeriesPair:
			return ::pluss::Timeline(*spec.date_series, *spec.time_series);
		case TimelineSpec::Kind::FromSeries:
			return ::pluss::Timeline::fromSeries(*spec.series);
	}
	return ::pluss::Timeline();
}

// Convenience wrappers (not friends)
inline ::pluss::Timeline tl_gen() { return tl_gen(TimelineSpec::now()); }
inline ::pluss::Timeline tl_gen(const std::string& s) { return tl_gen(TimelineSpec::from_string(s)); }
inline ::pluss::Timeline tl_gen(std::string&& s) { return tl_gen(TimelineSpec::from_string(std::move(s))); }
inline ::pluss::Timeline tl_gen(int year, int month, int day) { return tl_gen(TimelineSpec::date(year, month, day)); }
inline ::pluss::Timeline tl_time_gen(int hour, int minute, int second) {
	return tl_gen(TimelineSpec::time(hour, minute, second));
}
inline ::pluss::Timeline tl_gen(int year, int month, int day, int hour, int minute, int second) {
	return tl_gen(TimelineSpec::date_time(year, month, day, hour, minute, second));
}
inline ::pluss::Timeline tl_gen(const ::pluss::Series& date_series, const ::pluss::Series& time_series) {
	return tl_gen(TimelineSpec::from_series_pair(date_series, time_series));
}
inline ::pluss::Timeline tl_from_series_gen(const ::pluss::Series& series) {
	return tl_gen(TimelineSpec::from_series(series));
}

} // namespace b10::detail
