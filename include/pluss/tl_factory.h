#pragma once

#include <pluss/Timeline.h>
#include <pluss/note_factory.h>
#include <string>

namespace pluss {

// Small friend factory that has access to Timeline's private constructors.
struct TimelineFactory {
    static Timeline make_default() { return Timeline(); }
    static Timeline from_string(const std::string& s) { return Timeline(s); }
    static Timeline from_date(int y, int m, int d) { return Timeline::createFromDateInts(y,m,d); }
    static Timeline from_time(int h, int m, int s) { return Timeline::createFromTimeInts(h,m,s); }
    static Timeline from_datetime(int y, int mo, int d, int h, int mi, int se) { return Timeline::createFromDateTimeInts(y,mo,d,h,mi,se); }
    static Timeline from_series_pair(const Series& ds, const Series& ts) { return Timeline(ds, ts); }
    static Timeline from_series(const Series& s) { return Timeline::fromSeries(s); }
};

// Public factories 1 preferred API for users.
inline Timeline make_timeline() {
    return TimelineFactory::make_default();
}

inline Timeline make_timeline(const std::string& s) {
    return TimelineFactory::from_string(s);
}

inline Timeline make_timeline(const char* s) {
    return TimelineFactory::from_string(std::string(s));
}

inline Timeline make_timeline(int year, int month, int day) {
    return TimelineFactory::from_date(year, month, day);
}

inline Timeline make_timeline_time(int hour, int minute, int second) {
    return TimelineFactory::from_time(hour, minute, second);
}

inline Timeline make_timeline(const Series& date_series, const Series& time_series) {
    return TimelineFactory::from_series_pair(date_series, time_series);
}

inline Timeline make_timeline(const Series& series) {
    return TimelineFactory::from_series(series);
}

// moment - parse/construct from a textual/rvalue-like representation.
inline Timeline moment(const std::string& s) {
    return TimelineFactory::from_string(s);
}

} // namespace pluss
