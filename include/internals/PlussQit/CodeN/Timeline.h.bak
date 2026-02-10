#pragma once

#include <chrono>
#include <string>
#include <vector>
#include <regex>
#include <iomanip>
#include <sstream>
#include <torch/torch.h>
#include "SeriesBody.h"

#include <internals/core/include/b10/deets/operations.h>

namespace pluss {

class Timeline {
private:
    Series date_series_; // Stores date as YYYYMMDD
    Series time_series_; // Stores time as HHMMSS
    bool has_date_ = false;
    bool has_time_ = false;
    std::string timezone_; // optional TZ label (naive handling)

    // Helper methods for parsing
    static std::pair<bool, int64_t> parseDate(const std::string& date_str);
    static std::pair<bool, int64_t> parseTime(const std::string& time_str);
    static std::pair<bool, std::chrono::system_clock::time_point> parseTimeline(const std::string& Timeline_str);
    
    // Convert chrono time_point to our internal format
    static std::pair<int64_t, int64_t> timePointToInternalFormat(const std::chrono::system_clock::time_point& tp);
    
    // Convert our internal format to chrono time_point
    std::chrono::system_clock::time_point toTimePoint() const;
    // Static factory methods
    static Timeline today();
    static Timeline yesterday();
    static Timeline tomorrow();
    static Timeline now();
    static Timeline fromTimestamp(int64_t timestamp);
    static Timeline fromSeries(const Series& series);
    // Controlled creation helpers (used by TimelineFactory)
    static Timeline createFromDateInts(int year, int month, int day);
    static Timeline createFromTimeInts(int hour, int minute, int second);
    static Timeline createFromDateTimeInts(int year, int month, int day, int hour, int minute, int second);

    // Frequency utilities for pandas-like operations
    enum class Frequency { YEAR, MONTH, DAY, HOUR, MINUTE, SECOND, UNKNOWN };
    static Frequency freqFromString(const std::string& freq_str);
    static std::chrono::system_clock::duration durationForFreq(Frequency f, int step = 1);

    static Series int_series(int64_t v) {
        return b10::detail::series_gen(torch::series(v, torch::dtype(torch::kInt64)));
    }

private:
    // Constructors (private — use pluss::make_timeline() or pluss::moment())
    friend ::pluss::Timeline b10::detail::tl_gen(const b10::detail::TimelineSpec& spec);

    Timeline() {
        auto now = std::chrono::system_clock::now();
        auto [date_int, time_int] = timePointToInternalFormat(now);
        
        date_series_ = int_series(static_cast<int64_t>(date_int));
        time_series_ = int_series(static_cast<int64_t>(time_int));
        has_date_ = true;
        has_time_ = true;
    }

    Timeline(const std::string& Timeline_str) {
        // Parse the full string into a time_point, but determine presence of date/time
        // by searching within the string (date/time may appear alongside other tokens).
        auto [success, tp] = parseTimeline(Timeline_str);

        auto [date_success, date_int] = parseDate(Timeline_str);
        auto [time_success, time_int] = parseTime(Timeline_str);
        has_date_ = date_success;
        has_time_ = time_success;

        if (success) {
            // If parseTimeline succeeded, prefer its normalized fields.
            auto [norm_date_int, norm_time_int] = timePointToInternalFormat(tp);
            if (has_date_) {
                date_series_ = int_series(static_cast<int64_t>(norm_date_int));
            }
            if (has_time_) {
                time_series_ = int_series(static_cast<int64_t>(norm_time_int));
            }
        } else {
            // Default to current date and time
            auto now = std::chrono::system_clock::now();
            auto [date_int, time_int] = timePointToInternalFormat(now);
            
            date_series_ = int_series(static_cast<int64_t>(date_int));
            time_series_ = int_series(static_cast<int64_t>(time_int));
            has_date_ = true;
            has_time_ = true;
        }
    }

    Timeline(int year, int month, int day) {
        int64_t date_int = year * 10000 + month * 100 + day;
        date_series_ = int_series(static_cast<int64_t>(date_int));
        has_date_ = true;
    }

    Timeline(const int& hour, const int& minute, const int& second) {
        int64_t time_int = hour * 10000 + minute * 100 + second;
        time_series_ = int_series(static_cast<int64_t>(time_int));
        has_time_ = true;
    }

    Timeline(int year, int month, int day, int hour, int minute, int second) {
        int64_t date_int = year * 10000 + month * 100 + day;
        int64_t time_int = hour * 10000 + minute * 100 + second;
        
        date_series_ = int_series(static_cast<int64_t>(date_int));
        time_series_ = int_series(static_cast<int64_t>(time_int));
        has_date_ = true;
        has_time_ = true;
    }

    Timeline(const Series& date_series, const Series& time_series) {
        if (date_series.defined()) {
            date_series_ = date_series;
            has_date_ = true;
        }
        
        if (time_series.defined()) {
            time_series_ = time_series;
            has_time_ = true;
        }
    }

public:

    // Getters
    int year() const;
    int month() const;
    int day() const;
    int hour() const;
    int minute() const;
    int second() const;
    int day_of_week() const;  // 0 = Sunday, 1 = Monday, etc.
    int day_of_year() const;
    bool is_leap_year() const;
    std::string weekday_name(bool abbreviated = false) const;
    std::string month_name(bool abbreviated = false) const;
    
    // Check if date/time components are present
    bool has_date() const { return has_date_; }
    bool has_time() const { return has_time_; }
    
    // Setters
    void set_year(int year);
    void set_month(int month);
    void set_day(int day);
    void set_hour(int hour);
    void set_minute(int minute);
    void set_second(int second);
    
    // Formatting
    std::string format(const std::string& format_str) const;
    std::string toISOString() const;
    std::string to_string() const;
    
    // Conversion to Series
    Series to_Series() const;
    Series date_to_Series() const { return date_series_; }
    Series time_to_Series() const { return time_series_; }
    
    // Arithmetic operations
    Timeline add_years(int years) const;
    Timeline add_months(int months) const;
    Timeline add_days(int days) const;
    Timeline add_hours(int hours) const;
    Timeline add_minutes(int minutes) const;
    Timeline add_seconds(int seconds) const;
    
    // Difference calculations
    int years_between(const Timeline& other) const;
    int months_between(const Timeline& other) const;
    int days_between(const Timeline& other) const;
    int hours_between(const Timeline& other) const;
    int minutes_between(const Timeline& other) const;
    int seconds_between(const Timeline& other) const;
    
    // Construction and combination helpers (operations on existing Timeline)
    Timeline concat(const Timeline& other) const;
    Timeline concat(const std::vector<Timeline>& others) const;
    Timeline repeat(int64_t times) const;
    Timeline empty_like() const;
    Timeline fill_like(int year, int month, int day) const;
    Timeline fill_like(const std::string& date_str) const;
    
    // Comparison operators
    bool operator==(const Timeline& other) const;
    bool operator!=(const Timeline& other) const;
    bool operator<(const Timeline& other) const;
    bool operator<=(const Timeline& other) const;
    bool operator>(const Timeline& other) const;
    bool operator>=(const Timeline& other) const;

    // Pandas-like utilities
    Timeline floor(const std::string& freq) const;
    Timeline ceil(const std::string& freq) const;
    Timeline round(const std::string& freq) const;
    Timeline shift(int periods, const std::string& freq) const;

    // Timezone handling (naive; stores tz label but doesn't perform conversions)
    Timeline tz_localize(const std::string& tz) const;
    Timeline tz_convert(const std::string& tz) const;

    // Range generator
    static std::vector<Timeline> date_range(const Timeline& start, const Timeline& end, const std::string& freq = "D", int step = 1);

    // Convenience checks
    bool is_month_start() const;
    bool is_month_end() const;
    int quarter() const;
    bool is_quarter_start() const;
    bool is_quarter_end() const;
    bool is_weekend() const;
    int week_of_year() const;
    Timeline to_period(const std::string& freq) const; // alias for floor to represent a period
};

} // namespace pluss

namespace pluss {

namespace timeline_detail {

// Helper function to check if a string is a number
[[maybe_unused]] inline bool isNumber(const std::string& s) {
    return !s.empty() && std::all_of(s.begin(), s.end(), ::isdigit);
}

// Helper function to convert string to lowercase
[[maybe_unused]] inline std::string toLower(const std::string& s) {
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(), 
                    [](unsigned char c){ return std::tolower(c); });
    return result;
}

// Helper function to trim whitespace from start and end of string
[[maybe_unused]] inline std::string trim(const std::string& s) {
    auto start = s.begin();
    while (start != s.end() && std::isspace(*start)) {
        start++;
    }
    
    auto end = s.end();
    do {
        end--;
    } while (std::distance(start, end) > 0 && std::isspace(*end));
    
    return std::string(start, end + 1);
}

// Helper function to extract numbers from a string
[[maybe_unused]] inline std::vector<int> extractNumbers(const std::string& s) {
    std::vector<int> numbers;
    std::string current_number;
    
    for (char c : s) {
        if (std::isdigit(c)) {
            current_number += c;
        } else if (!current_number.empty()) {
            numbers.push_back(std::stoi(current_number));
            current_number.clear();
        }
    }
    
    if (!current_number.empty()) {
        numbers.push_back(std::stoi(current_number));
    }
    
    return numbers;
}

// Month name to number mapping
inline const std::map<std::string, int> month_map = {
    {"january", 1}, {"jan", 1},
    {"february", 2}, {"feb", 2},
    {"march", 3}, {"mar", 3},
    {"april", 4}, {"apr", 4},
    {"may", 5},
    {"june", 6}, {"jun", 6},
    {"july", 7}, {"jul", 7},
    {"august", 8}, {"aug", 8},
    {"september", 9}, {"sep", 9}, {"sept", 9},
    {"october", 10}, {"oct", 10},
    {"november", 11}, {"nov", 11},
    {"december", 12}, {"dec", 12}
};

// Day suffix to number mapping
inline const std::map<std::string, int> day_suffix_map = {
    {"st", 1}, {"nd", 2}, {"rd", 3}, {"th", 4}
};

} // namespace timeline_detail

// Parse date from string
inline std::pair<bool, int64_t> Timeline::parseDate(const std::string& date_str) {
    std::string date = timeline_detail::toLower(timeline_detail::trim(date_str));
    
    // Try to match various date formats
    
    // Format: MM/DD/YY or MM/DD/YYYY or MM-DD-YY or MM-DD-YYYY
    std::regex date_regex1("(\\d{1,2})[-/](\\d{1,2})[-/](\\d{2,4})");
    // Format: DD/MM/YY or DD/MM/YYYY or DD-MM-YY or DD-MM-YYYY
    std::regex date_regex2("(\\d{1,2})[-/](\\d{1,2})[-/](\\d{2,4})");
    // Format: YYYY/MM/DD or YYYY-MM-DD
    std::regex date_regex3("(\\d{4})[-/](\\d{1,2})[-/](\\d{1,2})");
    // Format: Month DD, YYYY or Month DDth, YYYY
    std::regex date_regex4("([a-z]+)\\s+(\\d{1,2})(?:st|nd|rd|th)?[,\\s]+(\\d{4})");
    // Format: DD Month YYYY or DDth Month YYYY
    std::regex date_regex5("(\\d{1,2})(?:st|nd|rd|th)?\\s+([a-z]+)[,\\s]+(\\d{4})");
    // Format: YYYY, Month DD or YYYY Month DD
    std::regex date_regex6("(\\d{4})[,\\s]+([a-z]+)\\s+(\\d{1,2})(?:st|nd|rd|th)?");
    
    std::smatch match;
    int year = 0, month = 0, day = 0;
    
    if (std::regex_search(date, match, date_regex3)) {
        // YYYY/MM/DD or YYYY-MM-DD
        year = std::stoi(match[1]);
        month = std::stoi(match[2]);
        day = std::stoi(match[3]);
    } else if (std::regex_search(date, match, date_regex4)) {
        // Month DD, YYYY
        std::string month_str = match[1];
        auto it = timeline_detail::month_map.find(month_str);
        if (it == timeline_detail::month_map.end()) return {false, 0};
        month = it->second;
        day = std::stoi(match[2]);
        year = std::stoi(match[3]);
    } else if (std::regex_search(date, match, date_regex5)) {
        // DD Month YYYY
        day = std::stoi(match[1]);
        std::string month_str = match[2];
        auto it = timeline_detail::month_map.find(month_str);
        if (it == timeline_detail::month_map.end()) return {false, 0};
        month = it->second;
        year = std::stoi(match[3]);
    } else if (std::regex_search(date, match, date_regex6)) {
        // YYYY Month DD
        year = std::stoi(match[1]);
        std::string month_str = match[2];
        auto it = timeline_detail::month_map.find(month_str);
        if (it == timeline_detail::month_map.end()) return {false, 0};
        month = it->second;
        day = std::stoi(match[3]);
    } else if (std::regex_search(date, match, date_regex1)) {
        // MM/DD/YY or MM/DD/YYYY
        month = std::stoi(match[1]);
        day = std::stoi(match[2]);
        year = std::stoi(match[3]);
        
        // Handle 2-digit years
        if (year < 100) {
            year += (year < 50) ? 2000 : 1900;
        }
    } else {
        // Try to extract numbers and guess the format
        std::vector<int> numbers = timeline_detail::extractNumbers(date);
        
        if (numbers.size() >= 3) {
            // Try to determine the format based on the values
            if (numbers.size() >= 3) {
                // First number is likely a year
                year = numbers[0];
                month = numbers[1];
                day = numbers[2];
            } else if (numbers[2] > 31) {
                // Last number is likely a year
                if (numbers[0] > 12) {
                    // First number is likely a day
                    day = numbers[0];
                    month = numbers[1];
                } else {
                    // First number could be month or day, assume month for US format
                    month = numbers[0];
                    day = numbers[1];
                }
                year = numbers[2];
                
                // Handle 2-digit years
                if (year < 100) {
                    year += (year < 50) ? 2000 : 1900;
                }
            } else {
                // All numbers are <= 31, try to guess based on common formats
                // Assume MM/DD/YYYY for simplicity
                month = numbers[0];
                day = numbers[1];
                year = numbers[2];
                
                // Handle 2-digit years
                if (year < 100) {
                    year += (year < 50) ? 2000 : 1900;
                }
            }
        } else {
            return {false, 0};
        }
    }
    
    // Validate date
    if (month < 1 || month > 12 || day < 1 || day > 31 || year < 1) {
        return {false, 0};
    }
    
    // Check days in month
    const std::array<int, 12> days_in_month = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int max_days = days_in_month[month - 1];
    
    // Adjust for leap year
    if (month == 2 && ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0))) {
        max_days = 29;
    }
    
    if (day > max_days) {
        return {false, 0};
    }
    
    // Convert to YYYYMMDD format
    int64_t date_int = year * 10000 + month * 100 + day;
    return {true, date_int};
}

// Parse time from string
inline std::pair<bool, int64_t> Timeline::parseTime(const std::string& time_str) {
    std::string time = timeline_detail::toLower(timeline_detail::trim(time_str));
    
    // Try to match various time formats
    
    // Format: HH:MM:SS or HH:MM
    std::regex time_regex1("(\\d{1,2}):(\\d{1,2})(?::(\\d{1,2}))?");
    // Format: HH.MM.SS or HH.MM
    std::regex time_regex2("(\\d{1,2})\\.(\\d{1,2})(?:\\.(\\d{1,2}))?");
    // Format: HHMMSS or HHMM
    std::regex time_regex3("(\\d{2})(\\d{2})(\\d{2})?");
    // Format: HH AM/PM or HH:MM AM/PM or HH:MM:SS AM/PM
    std::regex time_regex4("(\\d{1,2})(?::(\\d{1,2}))?(?::(\\d{1,2}))?\\s*(am|pm)");
    
    std::smatch match;
    int hour = 0, minute = 0, second = 0;
    bool is_pm = false;
    
    if (std::regex_search(time, match, time_regex4)) {
        // HH AM/PM or HH:MM AM/PM or HH:MM:SS AM/PM
        hour = std::stoi(match[1]);
        if (match[2].matched) {
            minute = std::stoi(match[2]);
        }
        if (match[3].matched) {
            second = std::stoi(match[3]);
        }
        is_pm = (match[4] == "pm");

        // Convert 12-hour format to 24-hour format
        if (is_pm && hour < 12) {
            hour += 12;
        } else if (!is_pm && hour == 12) {
            hour = 0;
        }
    } else if (std::regex_search(time, match, time_regex1)) {
        // HH:MM:SS or HH:MM
        hour = std::stoi(match[1]);
        minute = std::stoi(match[2]);
        if (match[3].matched) {
            second = std::stoi(match[3]);
        }
    } else if (std::regex_search(time, match, time_regex2)) {
        // HH.MM.SS or HH.MM
        hour = std::stoi(match[1]);
        minute = std::stoi(match[2]);
        if (match[3].matched) {
            second = std::stoi(match[3]);
        }
    } else if (std::regex_search(time, match, time_regex3)) {
        // HHMMSS or HHMM
        hour = std::stoi(match[1]);
        minute = std::stoi(match[2]);
        if (match[3].matched) {
            second = std::stoi(match[3]);
        }
    } else {
        // Try to extract numbers and guess the format
        std::vector<int> numbers = timeline_detail::extractNumbers(time);
        
        if (numbers.size() >= 2) {
            hour = numbers[0];
            minute = numbers[1];
            if (numbers.size() >= 3) {
                second = numbers[2];
            }
            
            // Check for AM/PM
            if (time.find("pm") != std::string::npos) {
                is_pm = true;
                if (hour < 12) {
                    hour += 12;
                }
            } else if (time.find("am") != std::string::npos) {
                if (hour == 12) {
                    hour = 0;
                }
            }
        } else {
            return {false, 0};
        }
    }
    
    // Validate time
    if (hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 59) {
        return {false, 0};
    }
    
    // Convert to HHMMSS format
    int64_t time_int = hour * 10000 + minute * 100 + second;
    return {true, time_int};
}

// Parse Timeline from string
inline std::pair<bool, std::chrono::system_clock::time_point> Timeline::parseTimeline(const std::string& Timeline_str) {
    std::string timeline = timeline_detail::trim(Timeline_str);

    // Prefer parsing against the full string. parseDate/parseTime use regex_search,
    // so they can match embedded date/time segments (e.g. "March 5, 2022 4:05 pm").
    {
        auto [date_success, date_int] = parseDate(timeline);
        auto [time_success, time_int] = parseTime(timeline);
        if (date_success && time_success) {
            std::tm tm = {};
            tm.tm_year = date_int / 10000 - 1900;
            tm.tm_mon = (date_int % 10000) / 100 - 1;
            tm.tm_mday = date_int % 100;
            tm.tm_hour = time_int / 10000;
            tm.tm_min = (time_int % 10000) / 100;
            tm.tm_sec = time_int % 100;
            tm.tm_isdst = -1;

            std::time_t tt = std::mktime(&tm);
            return {true, std::chrono::system_clock::from_time_t(tt)};
        }
    }
    
    // Try parsing as date only
    auto [date_success, date_int] = parseDate(timeline);
    if (date_success) {
        std::tm tm = {};
        tm.tm_year = date_int / 10000 - 1900;
        tm.tm_mon = (date_int % 10000) / 100 - 1;
        tm.tm_mday = date_int % 100;
        tm.tm_isdst = -1;
        
        std::time_t tt = std::mktime(&tm);
        return {true, std::chrono::system_clock::from_time_t(tt)};
    }
    
    // Try parsing as time only
    auto [time_success, time_int] = parseTime(timeline);
    if (time_success) {
        auto now = std::chrono::system_clock::now();
        std::time_t now_tt = std::chrono::system_clock::to_time_t(now);
        std::tm* now_tm = std::localtime(&now_tt);
        
        now_tm->tm_hour = time_int / 10000;
        now_tm->tm_min = (time_int % 10000) / 100;
        now_tm->tm_sec = time_int % 100;
        
        std::time_t tt = std::mktime(now_tm);
        return {true, std::chrono::system_clock::from_time_t(tt)};
    }
    
    return {false, std::chrono::system_clock::time_point{}};
}

// Convert chrono time_point to internal format
inline std::pair<int64_t, int64_t> Timeline::timePointToInternalFormat(const std::chrono::system_clock::time_point& tp) {
    std::time_t tt = std::chrono::system_clock::to_time_t(tp);
    std::tm* tm = std::localtime(&tt);
    
    int64_t date_int = (tm->tm_year + 1900) * 10000 + (tm->tm_mon + 1) * 100 + tm->tm_mday;
    int64_t time_int = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
    
    return {date_int, time_int};
}

// Convert internal format to chrono time_point
inline std::chrono::system_clock::time_point Timeline::toTimePoint() const {
    if (!has_date_) {
        // If no date, use today's date
        auto now = std::chrono::system_clock::now();
        std::time_t now_tt = std::chrono::system_clock::to_time_t(now);
        std::tm* now_tm = std::localtime(&now_tt);
        
        int64_t time_val = has_time_ ? time_series_.item<int64_t>() : 0;
        
        now_tm->tm_hour = time_val / 10000;
        now_tm->tm_min = (time_val % 10000) / 100;
        now_tm->tm_sec = time_val % 100;
        
        std::time_t tt = std::mktime(now_tm);
        return std::chrono::system_clock::from_time_t(tt);
    } else if (!has_time_) {
        // If no time, use midnight
        int64_t date_val = date_series_.item<int64_t>();
        
        std::tm tm = {};
        tm.tm_year = date_val / 10000 - 1900;
        tm.tm_mon = (date_val % 10000) / 100 - 1;
        tm.tm_mday = date_val % 100;
        tm.tm_isdst = -1;
        
        std::time_t tt = std::mktime(&tm);
        return std::chrono::system_clock::from_time_t(tt);
    } else {
        // Both date and time
        int64_t date_val = date_series_.item<int64_t>();
        int64_t time_val = time_series_.item<int64_t>();
        
        std::tm tm = {};
        tm.tm_year = date_val / 10000 - 1900;
        tm.tm_mon = (date_val % 10000) / 100 - 1;
        tm.tm_mday = date_val % 100;
        tm.tm_hour = time_val / 10000;
        tm.tm_min = (time_val % 10000) / 100;
        tm.tm_sec = time_val % 100;
        tm.tm_isdst = -1;
        
        std::time_t tt = std::mktime(&tm);
        return std::chrono::system_clock::from_time_t(tt);
    }
}

inline Timeline Timeline::today() {
    auto now = std::chrono::system_clock::now();
    auto [date_int, _] = timePointToInternalFormat(now);
    
    Timeline dt;
    dt.date_series_ = int_series(static_cast<int64_t>(date_int));
    dt.has_date_ = true;
    dt.has_time_ = false;
    
    return dt;
}

inline Timeline Timeline::yesterday() {
    auto now = std::chrono::system_clock::now();
    auto yesterday = now - std::chrono::hours(24);
    auto [date_int, _] = timePointToInternalFormat(yesterday);
    
    Timeline dt;
    dt.date_series_ = int_series(static_cast<int64_t>(date_int));
    dt.has_date_ = true;
    dt.has_time_ = false;
    
    return dt;
}

inline Timeline Timeline::tomorrow() {
    auto now = std::chrono::system_clock::now();
    auto tomorrow = now + std::chrono::hours(24);
    auto [date_int, _] = timePointToInternalFormat(tomorrow);
    
    Timeline dt;
    dt.date_series_ = int_series(static_cast<int64_t>(date_int));
    dt.has_date_ = true;
    dt.has_time_ = false;
    
    return dt;
}

inline Timeline Timeline::now() {
    return Timeline();
}

inline Timeline Timeline::createFromDateInts(int year, int month, int day) {
    Timeline t;
    int64_t date_int = static_cast<int64_t>(year) * 10000 + month * 100 + day;
    t.date_series_ = int_series(static_cast<int64_t>(date_int));
    t.has_date_ = true;
    t.has_time_ = false;
    return t;
}

inline Timeline Timeline::createFromTimeInts(int hour, int minute, int second) {
    Timeline t;
    int64_t time_int = static_cast<int64_t>(hour) * 10000 + minute * 100 + second;
    t.time_series_ = int_series(static_cast<int64_t>(time_int));
    t.has_time_ = true;
    t.has_date_ = false;
    return t;
}

inline Timeline Timeline::createFromDateTimeInts(int year, int month, int day, int hour, int minute, int second) {
    Timeline t;
    int64_t date_int = static_cast<int64_t>(year) * 10000 + month * 100 + day;
    int64_t time_int = static_cast<int64_t>(hour) * 10000 + minute * 100 + second;
    t.date_series_ = int_series(static_cast<int64_t>(date_int));
    t.time_series_ = int_series(static_cast<int64_t>(time_int));
    t.has_date_ = true;
    t.has_time_ = true;
    return t;
}

inline Timeline Timeline::fromTimestamp(int64_t timestamp) {
    auto tp = std::chrono::system_clock::from_time_t(timestamp);
    auto [date_int, time_int] = timePointToInternalFormat(tp);
    
    Timeline dt;
    dt.date_series_ = int_series(static_cast<int64_t>(date_int));
    dt.time_series_ = int_series(static_cast<int64_t>(time_int));
    dt.has_date_ = true;
    dt.has_time_ = true;
    
    return dt;
}

inline Timeline Timeline::fromSeries(const Series& series) {
    // Handle scalar
    if (series.dim() == 0) {
        return Timeline(series, Series());
    }

    // If 1D with length==2 treat as [date, time] pair
    if (series.dim() == 1 && series.shape(0) == 2) {
        return Timeline(series[0], series[1]);
    }

    // If the last dimension is 2, treat it as a pair dimension and split into date/time series
    // Works for 1D (already handled), 2D, 3D, etc.
    // Use Series API to split last-dimension pairs (avoids needing series cast)
    try {
        int64_t last_dim = series.dim() - 1;
        if (series.dim() >= 1 && series.shape(last_dim) == 2) {
            Series ds = series.select(last_dim, 0);
            Series ts = series.select(last_dim, 1);
            return Timeline(ds, ts);
        }
    } catch (...) {
        // Fallthrough to default
    }

    // Default: cannot interpret -> current date/time
    return Timeline();
}

// Getters
inline int Timeline::year() const {
    if (!has_date_) return 0;
    int64_t date_val = date_series_.item<int64_t>();
    return date_val / 10000;
}

inline int Timeline::month() const {
    if (!has_date_) return 0;
    int64_t date_val = date_series_.item<int64_t>();
    return (date_val % 10000) / 100;
}

inline int Timeline::day() const {
    if (!has_date_) return 0;
    int64_t date_val = date_series_.item<int64_t>();
    return date_val % 100;
}

inline int Timeline::hour() const {
    if (!has_time_) return 0;
    int64_t time_val = time_series_.item<int64_t>();
    return time_val / 10000;
}

inline int Timeline::minute() const {
    if (!has_time_) return 0;
    int64_t time_val = time_series_.item<int64_t>();
    return (time_val % 10000) / 100;
}

inline int Timeline::second() const {
    if (!has_time_) return 0;
    int64_t time_val = time_series_.item<int64_t>();
    return time_val % 100;
}

inline int Timeline::day_of_week() const {
    if (!has_date_) return 0;
    
    auto tp = toTimePoint();
    std::time_t tt = std::chrono::system_clock::to_time_t(tp);
    std::tm* tm = std::localtime(&tt);
    
    return tm->tm_wday;
}

inline int Timeline::day_of_year() const {
    if (!has_date_) return 0;
    
    auto tp = toTimePoint();
    std::time_t tt = std::chrono::system_clock::to_time_t(tp);
    std::tm* tm = std::localtime(&tt);
    
    return tm->tm_yday + 1;
}

inline bool Timeline::is_leap_year() const {
    if (!has_date_) return false;
    
    int y = year();
    return (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
}

inline std::string Timeline::weekday_name(bool abbreviated) const {
    if (!has_date_) return "";
    
    static const std::array<std::string, 7> full_names = {
        "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"
    };
    
    static const std::array<std::string, 7> abbr_names = {
        "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
    };
    
    int dow = day_of_week();
    return abbreviated ? abbr_names[dow] : full_names[dow];
}

inline std::string Timeline::month_name(bool abbreviated) const {
    if (!has_date_) return "";
    
    static const std::array<std::string, 12> full_names = {
        "January", "February", "March", "April", "May", "June",
        "July", "August", "September", "October", "November", "December"
    };
    
    static const std::array<std::string, 12> abbr_names = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };
    
    int m = month();
    if (m < 1 || m > 12) return "";
    
    return abbreviated ? abbr_names[m - 1] : full_names[m - 1];
}

// Setters
inline void Timeline::set_year(int year) {
    if (!has_date_) {
        // If no date set, use current date
        auto now = std::chrono::system_clock::now();
        auto [date_int, _] = timePointToInternalFormat(now);
        date_series_ = int_series(static_cast<int64_t>(date_int));
        has_date_ = true;
    }
    
    int64_t date_val = date_series_.item<int64_t>();
    int month = (date_val % 10000) / 100;
    int day = date_val % 100;
    
    int64_t new_date = year * 10000 + month * 100 + day;
    date_series_ = int_series(static_cast<int64_t>(new_date));
}

inline void Timeline::set_month(int month) {
    if (!has_date_) {
        // If no date set, use current date
        auto now = std::chrono::system_clock::now();
        auto [date_int, _] = timePointToInternalFormat(now);
        date_series_ = int_series(static_cast<int64_t>(date_int));
        has_date_ = true;
    }
    
    int64_t date_val = date_series_.item<int64_t>();
    int year = date_val / 10000;
    int day = date_val % 100;
    
    // Validate month
    if (month < 1 || month > 12) {
        month = 1;  // Default to January if invalid
    }
    
    // Validate day for the new month
    const std::array<int, 12> days_in_month = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int max_days = days_in_month[month - 1];
    
    // Adjust for leap year
    if (month == 2 && ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0))) {
        max_days = 29;
    }
    
    if (day > max_days) {
        day = max_days;  // Adjust day if it exceeds the maximum for the month
    }
    
    int64_t new_date = year * 10000 + month * 100 + day;
    date_series_ = int_series(static_cast<int64_t>(new_date));
}

inline void Timeline::set_day(int day) {
    if (!has_date_) {
        // If no date set, use current date
        auto now = std::chrono::system_clock::now();
        auto [date_int, _] = timePointToInternalFormat(now);
        date_series_ = int_series(static_cast<int64_t>(date_int));
        has_date_ = true;
    }
    
    int64_t date_val = date_series_.item<int64_t>();
    int year = date_val / 10000;
    int month = (date_val % 10000) / 100;
    
    // Validate day for the month
    const std::array<int, 12> days_in_month = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int max_days = days_in_month[month - 1];
    
    // Adjust for leap year
    if (month == 2 && ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0))) {
        max_days = 29;
    }
    
    // Validate day
    if (day < 1 || day > max_days) {
        day = 1;  // Default to first day if invalid
    }
    
    int64_t new_date = year * 10000 + month * 100 + day;
    date_series_ = int_series(static_cast<int64_t>(new_date));
}

inline void Timeline::set_hour(int hour) {
    if (!has_time_) {
        // If no time set, use midnight
        time_series_ = int_series(static_cast<int64_t>(0));
        has_time_ = true;
    }
    
    int64_t time_val = time_series_.item<int64_t>();
    int minute = (time_val % 10000) / 100;
    int second = time_val % 100;
    
    // Validate hour
    if (hour < 0 || hour > 23) {
        hour = 0;  // Default to midnight if invalid
    }
    
    int64_t new_time = hour * 10000 + minute * 100 + second;
    time_series_ = int_series(static_cast<int64_t>(new_time));
}

inline void Timeline::set_minute(int minute) {
    if (!has_time_) {
        // If no time set, use midnight
        time_series_ = int_series(static_cast<int64_t>(0));
        has_time_ = true;
    }
    
    int64_t time_val = time_series_.item<int64_t>();
    int hour = time_val / 10000;
    int second = time_val % 100;
    
    // Validate minute
    if (minute < 0 || minute > 59) {
        minute = 0;  // Default to 0 if invalid
    }
    
    int64_t new_time = hour * 10000 + minute * 100 + second;
    time_series_ = int_series(static_cast<int64_t>(new_time));
}

inline void Timeline::set_second(int second) {
    if (!has_time_) {
        // If no time set, use midnight
        time_series_ = int_series(static_cast<int64_t>(0));
        has_time_ = true;
    }
    
    int64_t time_val = time_series_.item<int64_t>();
    int hour = time_val / 10000;
    int minute = (time_val % 10000) / 100;
    
    // Validate second
    if (second < 0 || second > 59) {
        second = 0;  // Default to 0 if invalid
    }
    
    int64_t new_time = hour * 10000 + minute * 100 + second;
    time_series_ = int_series(static_cast<int64_t>(new_time));
}

// Formatting
inline std::string Timeline::format(const std::string& format_str) const {
    auto tp = toTimePoint();
    std::time_t tt = std::chrono::system_clock::to_time_t(tp);
    std::tm* tm = std::localtime(&tt);
    
    std::stringstream ss;
    ss << std::put_time(tm, format_str.c_str());
    return ss.str();
}

inline std::string Timeline::toISOString() const {
    if (has_date_ && has_time_) {
        return format("%Y-%m-%dT%H:%M:%S");
    } else if (has_date_) {
        return format("%Y-%m-%d");
    } else if (has_time_) {
        return format("%H:%M:%S");
    } else {
        return "";
    }
}

inline std::string Timeline::to_string() const {
    if (has_date_ && has_time_) {
        return format("%Y-%m-%d %H:%M:%S");
    } else if (has_date_) {
        return format("%Y-%m-%d");
    } else if (has_time_) {
        return format("%H:%M:%S");
    } else {
        return "";
    }
}

// Conversion to Series
inline Series Timeline::to_Series() const {
    if (has_date_ && has_time_) {
        // Return a 1D Series with [date, time]
		auto date_tensor = date_series_.to_cpu().series();
		auto time_tensor = time_series_.to_cpu().series();
		auto stacked = pluss::stack({date_tensor, time_tensor});
		return b10::detail::series_gen(std::move(stacked));
    } else if (has_date_) {
        return date_series_;
    } else if (has_time_) {
        return time_series_;
    } else {
        // Return an empty Series
        return Series();
    }
}

// Arithmetic operations
inline Timeline Timeline::add_years(int years) const {
    if (!has_date_) {
        return *this;
    }
    
    int y = year() + years;
    int m = month();
    int d = day();
    
    // Handle leap year edge case (February 29)
    if (m == 2 && d == 29) {
        bool is_leap = (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
        if (!is_leap) {
            d = 28;
        }
    }
    
    Timeline result = *this;
    result.set_year(y);
    result.set_month(m);
    result.set_day(d);
    
    return result;
}

inline Timeline Timeline::add_months(int months) const {
    if (!has_date_) {
        return *this;
    }
    
    int y = year();
    int m = month();
    int d = day();
    
    // Calculate new year and month
    m += months;
    y += (m - 1) / 12;
    m = ((m - 1) % 12) + 1;
    if (m < 1) {
        m += 12;
        y--;
    }
    
    // Handle month length differences
    const std::array<int, 12> days_in_month = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int max_days = days_in_month[m - 1];
    
    // Adjust for leap year
    if (m == 2 && ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0))) {
        max_days = 29;
    }
    
    if (d > max_days) {
        d = max_days;
    }
    
    Timeline result(*this);
    result.set_year(y);
    result.set_month(m);
    result.set_day(d);
    
    return result;
}

inline Timeline Timeline::add_days(int days) const {
    auto tp = toTimePoint();
    tp += std::chrono::hours(24 * days);
    
    auto [date_int, time_int] = timePointToInternalFormat(tp);
    
    Timeline result(*this);
    if (has_date_) {
        result.date_series_ = int_series(static_cast<int64_t>(date_int));
    }
    
    return result;
}

inline Timeline Timeline::add_hours(int hours) const {
    auto tp = toTimePoint();
    tp += std::chrono::hours(hours);
    
    auto [date_int, time_int] = timePointToInternalFormat(tp);
    
    Timeline result(*this);
    if (has_date_) {
        result.date_series_ = int_series(static_cast<int64_t>(date_int));
    }
    if (has_time_) {
        result.time_series_ = int_series(static_cast<int64_t>(time_int));
    }
    
    return result;
}

inline Timeline Timeline::add_minutes(int minutes) const {
    auto tp = toTimePoint();
    tp += std::chrono::minutes(minutes);
    
    auto [date_int, time_int] = timePointToInternalFormat(tp);
    
    Timeline result(*this);
    if (has_date_) {
        result.date_series_ = int_series(static_cast<int64_t>(date_int));
    }
    if (has_time_) {
        result.time_series_ = int_series(static_cast<int64_t>(time_int));
    }
    
    return result;
}

inline Timeline Timeline::add_seconds(int seconds) const {
    auto tp = toTimePoint();
    tp += std::chrono::seconds(seconds);
    
    auto [date_int, time_int] = timePointToInternalFormat(tp);
    
    Timeline result(*this);
    if (has_date_) {
        result.date_series_ = int_series(static_cast<int64_t>(date_int));
    }
    if (has_time_) {
        result.time_series_ = int_series(static_cast<int64_t>(time_int));
    }
    
    return result;
}

// Difference calculations
inline int Timeline::years_between(const Timeline& other) const {
    if (!has_date_ || !other.has_date_) {
        return 0;
    }
    
    int y1 = year();
    int m1 = month();
    int d1 = day();
    
    int y2 = other.year();
    int m2 = other.month();
    int d2 = other.day();
    
    int years = y2 - y1;
    
    // Adjust for month and day
    if (m2 < m1 || (m2 == m1 && d2 < d1)) {
        years--;
    }
    
    return years;
}

inline int Timeline::months_between(const Timeline& other) const {
    if (!has_date_ || !other.has_date_) {
        return 0;
    }
    
    int y1 = year();
    int m1 = month();
    int d1 = day();
    
    int y2 = other.year();
    int m2 = other.month();
    int d2 = other.day();
    
    int months = (y2 - y1) * 12 + (m2 - m1);
    
    // Adjust for day
    if (d2 < d1) {
        months--;
    }
    
    return months;
}

inline int Timeline::days_between(const Timeline& other) const {
    if (!has_date_ || !other.has_date_) {
        return 0;
    }
    
    auto tp1 = toTimePoint();
    auto tp2 = other.toTimePoint();
    
    auto duration = tp2 - tp1;
    auto days = std::chrono::duration_cast<std::chrono::hours>(duration).count() / 24;
    
    return static_cast<int>(days);
}

inline int Timeline::hours_between(const Timeline& other) const {
    if ((!has_date_ && !has_time_) || (!other.has_date_ && !other.has_time_)) {
        return 0;
    }
    
    auto tp1 = toTimePoint();
    auto tp2 = other.toTimePoint();
    
    auto duration = tp2 - tp1;
    auto hours = std::chrono::duration_cast<std::chrono::hours>(duration).count();
    
    return static_cast<int>(hours);
}

inline int Timeline::minutes_between(const Timeline& other) const {
    if ((!has_date_ && !has_time_) || (!other.has_date_ && !other.has_time_)) {
        return 0;
    }
    
    auto tp1 = toTimePoint();
    auto tp2 = other.toTimePoint();
    
    auto duration = tp2 - tp1;
    auto minutes = std::chrono::duration_cast<std::chrono::minutes>(duration).count();
    
    return static_cast<int>(minutes);
}

inline int Timeline::seconds_between(const Timeline& other) const {
    if ((!has_date_ && !has_time_) || (!other.has_date_ && !other.has_time_)) {
        return 0;
    }
    
    auto tp1 = toTimePoint();
    auto tp2 = other.toTimePoint();
    
    auto duration = tp2 - tp1;
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
    
    return static_cast<int>(seconds);
}

// Comparison operators
inline bool Timeline::operator==(const Timeline& other) const {
    if (has_date_ != other.has_date_ || has_time_ != other.has_time_) {
        return false;
    }
    
    if (has_date_ && date_series_.item<int64_t>() != other.date_series_.item<int64_t>()) {
        return false;
    }
    
    if (has_time_ && time_series_.item<int64_t>() != other.time_series_.item<int64_t>()) {
        return false;
    }
    
    return true;
}

inline bool Timeline::operator!=(const Timeline& other) const {
    return !(*this == other);
}

inline bool Timeline::operator<(const Timeline& other) const {
    if (!has_date_ && !other.has_date_) {
        // Compare time only
        if (!has_time_ || !other.has_time_) {
            return false;
        }
        return time_series_.item<int64_t>() < other.time_series_.item<int64_t>();
    } else if (has_date_ && other.has_date_) {
        // Compare date first
        int64_t date1 = date_series_.item<int64_t>();
        int64_t date2 = other.date_series_.item<int64_t>();
        
        if (date1 != date2) {
            return date1 < date2;
        }
        
        // If dates are equal, compare time
        if (!has_time_ || !other.has_time_) {
            return !has_time_ && other.has_time_;
        }
        
        return time_series_.item<int64_t>() < other.time_series_.item<int64_t>();
    } else {
        // One has date, one doesn't
        return !has_date_ && other.has_date_;
    }
}

inline bool Timeline::operator<=(const Timeline& other) const {
    return *this < other || *this == other;
}

inline bool Timeline::operator>(const Timeline& other) const {
    return !(*this <= other);
}

inline bool Timeline::operator>=(const Timeline& other) const {
    return !(*this < other);
}

// Frequency helpers
inline Timeline::Frequency Timeline::freqFromString(const std::string& freq_str) {
    std::string s = timeline_detail::toLower(timeline_detail::trim(freq_str));
    if (s.empty()) return Frequency::UNKNOWN;
    if (s == "y" || s == "year" || s == "years" || s == "annual") return Frequency::YEAR;
    if (s == "m" || s == "month" || s == "months") return Frequency::MONTH;
    if (s == "d" || s == "day" || s == "days") return Frequency::DAY;
    if (s == "h" || s == "hour" || s == "hours") return Frequency::HOUR;
    if (s == "t" || s == "min" || s == "minute" || s == "minutes") return Frequency::MINUTE;
    if (s == "s" || s == "sec" || s == "second" || s == "seconds") return Frequency::SECOND;
    return Frequency::UNKNOWN;
}

inline std::chrono::system_clock::duration Timeline::durationForFreq(Frequency f, int step) {
    switch (f) {
        case Frequency::DAY: return std::chrono::hours(24LL * step);
        case Frequency::HOUR: return std::chrono::hours(1LL * step);
        case Frequency::MINUTE: return std::chrono::minutes(1LL * step);
        case Frequency::SECOND: return std::chrono::seconds(1LL * step);
        default: return std::chrono::seconds(0);
    }
}

// Pandas-like utilities
inline Timeline Timeline::floor(const std::string& freq) const {
    Frequency f = freqFromString(freq);
    Timeline r = *this;
    switch (f) {
        case Frequency::YEAR:
            r.set_month(1);
            r.set_day(1);
            r.set_hour(0);
            r.set_minute(0);
            r.set_second(0);
            return r;
        case Frequency::MONTH:
            r.set_day(1);
            r.set_hour(0);
            r.set_minute(0);
            r.set_second(0);
            return r;
        case Frequency::DAY:
            r.set_hour(0);
            r.set_minute(0);
            r.set_second(0);
            return r;
        case Frequency::HOUR:
            r.set_minute(0);
            r.set_second(0);
            return r;
        case Frequency::MINUTE:
            r.set_second(0);
            return r;
        case Frequency::SECOND:
        default:
            return *this;
    }
}

inline Timeline Timeline::ceil(const std::string& freq) const {
    Timeline f = floor(freq);
    if (*this == f) return f;
    return f.shift(1, freq);
}

inline Timeline Timeline::round(const std::string& freq) const {
    Frequency fr = freqFromString(freq);
    if (fr == Frequency::YEAR || fr == Frequency::MONTH) {
        return floor(freq);
    }

    Timeline f = floor(freq);
    auto tp = toTimePoint();
    auto f_tp = f.toTimePoint();
    auto d = tp - f_tp;
    auto step = durationForFreq(fr, 1);
    if (step == std::chrono::seconds(0)) return f;
    if (d < (step / 2)) return f;
    return f.shift(1, freq);
}

inline Timeline Timeline::shift(int periods, const std::string& freq) const {
    Frequency f = freqFromString(freq);
    switch (f) {
        case Frequency::YEAR: return add_years(periods);
        case Frequency::MONTH: return add_months(periods);
        case Frequency::DAY: return add_days(periods);
        case Frequency::HOUR: return add_hours(periods);
        case Frequency::MINUTE: return add_minutes(periods);
        case Frequency::SECOND: return add_seconds(periods);
        default: return *this;
    }
}

inline Timeline Timeline::tz_localize(const std::string& tz) const {
    Timeline r = *this;
    r.timezone_ = tz;
    return r;
}

inline Timeline Timeline::tz_convert(const std::string& tz) const {
    // Naive: only changes label; no offset math implemented
    Timeline r = *this;
    r.timezone_ = tz;
    return r;
}

inline std::vector<Timeline> Timeline::date_range(const Timeline& start, const Timeline& end, const std::string& freq, int step) {
    std::vector<Timeline> out;
    if (end < start) return out;
    Timeline cur = start.to_period(freq);
    Timeline end_period = end.to_period(freq);
    while (cur <= end_period) {
        out.push_back(cur);
        cur = cur.shift(step, freq);
    }
    return out;
}

inline bool Timeline::is_month_start() const {
    if (!has_date_) return false;
    return day() == 1;
}

inline bool Timeline::is_month_end() const {
    if (!has_date_) return false;
    int y = year();
    int m = month();
    const std::array<int, 12> days_in_month = {31,28,31,30,31,30,31,31,30,31,30,31};
    int max_days = days_in_month[m - 1];
    if (m == 2 && ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0))) max_days = 29;
    return day() == max_days;
}

inline int Timeline::quarter() const {
    if (!has_date_) return 0;
    return ((month() - 1) / 3) + 1;
}

inline bool Timeline::is_quarter_start() const {
    if (!has_date_) return false;
    int m = month();
    return (m == 1 || m == 4 || m == 7 || m == 10) && day() == 1;
}

inline bool Timeline::is_quarter_end() const {
    if (!has_date_) return false;
    int m = month();
    if (m == 3 || m == 6 || m == 9 || m == 12) {
        return is_month_end();
    }
    return false;
}

inline bool Timeline::is_weekend() const {
    if (!has_date_) return false;
    int dow = day_of_week();
    return dow == 0 || dow == 6;
}

inline int Timeline::week_of_year() const {
    if (!has_date_) return 0;
    auto tp = toTimePoint();
    std::time_t tt = std::chrono::system_clock::to_time_t(tp);
    std::tm* tm = std::localtime(&tt);
    char buf[8] = {0};
    if (std::strftime(buf, sizeof(buf), "%U", tm) == 0) return 0;
    return std::stoi(std::string(buf));
}

inline Timeline Timeline::to_period(const std::string& freq) const {
    return floor(freq);
}

// Construction and combination helpers (operations on existing Timeline)
inline Timeline Timeline::concat(const Timeline& other) const {
    // Concatenate two Timelines by combining their date and time Series
    if (!has_date_ && !other.has_date_) {
        // Both are time-only, concatenate times
        if (has_time_ && other.has_time_) {
            pluss::Series concat_time = torch::cat({time_series_.series(), other.time_series_.series()}, 0);
            return Timeline(Series(), b10::detail::series_gen(concat_time));
        }
        return *this;
    }
    
    if (!has_time_ && !other.has_time_) {
        // Both are date-only, concatenate dates
        if (has_date_ && other.has_date_) {
            pluss::Series concat_date = torch::cat({date_series_.series(), other.date_series_.series()}, 0);
            return Timeline(b10::detail::series_gen(concat_date), Series());
        }
        return *this;
    }
    
    // Both have date and time, concatenate both
    if (has_date_ && other.has_date_ && has_time_ && other.has_time_) {
        pluss::Series concat_date = torch::cat({date_series_.series(), other.date_series_.series()}, 0);
        pluss::Series concat_time = torch::cat({time_series_.series(), other.time_series_.series()}, 0);
        return Timeline(b10::detail::series_gen(concat_date), b10::detail::series_gen(concat_time));
    }
    
    // Mismatched types, return first one
    return *this;
}

inline Timeline Timeline::concat(const std::vector<Timeline>& others) const {
    Timeline result = *this;
    for (const auto& other : others) {
        result = result.concat(other);
    }
    return result;
}

inline Timeline Timeline::repeat(int64_t times) const {
    if (times <= 0) return Timeline();
    if (times == 1) return *this;
    
    std::vector<pluss::Series> date_tensors;
    std::vector<pluss::Series> time_tensors;
    
    for (int64_t i = 0; i < times; ++i) {
        if (has_date_) date_tensors.push_back(date_series_.series());
        if (has_time_) time_tensors.push_back(time_series_.series());
    }
    
    Series result_date = has_date_ ? b10::detail::series_gen(torch::cat(date_tensors, 0)) : Series();
    Series result_time = has_time_ ? b10::detail::series_gen(torch::cat(time_tensors, 0)) : Series();
    
    return Timeline(result_date, result_time);
}

inline Timeline Timeline::empty_like() const {
    // Create an empty Timeline with same structure (date/time presence) but no data
    if (has_date_ && has_time_) {
        return Timeline(
            b10::detail::series_gen(torch::empty({0}, torch::dtype(torch::kInt64))),
            b10::detail::series_gen(torch::empty({0}, torch::dtype(torch::kInt64)))
        );
    } else if (has_date_) {
        return Timeline(
            b10::detail::series_gen(torch::empty({0}, torch::dtype(torch::kInt64))),
            Series()
        );
    } else if (has_time_) {
        return Timeline(
            Series(),
            b10::detail::series_gen(torch::empty({0}, torch::dtype(torch::kInt64)))
        );
    }
    return Timeline();
}

inline Timeline Timeline::fill_like(int year, int month, int day) const {
    // Create a Timeline with same shape as this, filled with specified date
    if (!has_date_) return *this;
    
    int64_t fill_date = year * 10000 + month * 100 + day;
    int64_t size = date_series_.numel();
    
    Series fill_date_series = b10::detail::series_gen(
        torch::full({size}, fill_date, torch::dtype(torch::kInt64))
    );
    
    Series fill_time_series = has_time_ ? 
        b10::detail::series_gen(torch::zeros({size}, torch::dtype(torch::kInt64))) : 
        Series();
    
    return Timeline(fill_date_series, fill_time_series);
}

inline Timeline Timeline::fill_like(const std::string& date_str) const {
    Timeline ref(date_str);
    return fill_like(ref.year(), ref.month(), ref.day());
}
    
} // namespace pluss
