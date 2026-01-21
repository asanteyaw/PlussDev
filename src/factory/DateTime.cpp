#include "pluss/DateTime.h"
#include <map>
#include <array>
#include <ctime>
#include <locale>
#include <algorithm>
#include <cctype>
#include <torch/torch.h>

namespace pluss {

// Helper function to check if a string is a number
static bool isNumber(const std::string& s) {
    return !s.empty() && std::all_of(s.begin(), s.end(), ::isdigit);
}

// Helper function to convert string to lowercase
static std::string toLower(const std::string& s) {
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(), 
                   [](unsigned char c){ return std::tolower(c); });
    return result;
}

// Helper function to trim whitespace from start and end of string
static std::string trim(const std::string& s) {
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
static std::vector<int> extractNumbers(const std::string& s) {
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
static const std::map<std::string, int> month_map = {
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
static const std::map<std::string, int> day_suffix_map = {
    {"st", 1}, {"nd", 2}, {"rd", 3}, {"th", 4}
};

// Parse date from string
std::pair<bool, int64_t> DateTime::parseDate(const std::string& date_str) {
    std::string date = toLower(trim(date_str));
    
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
    
    if (std::regex_match(date, match, date_regex1)) {
        // MM/DD/YY or MM/DD/YYYY
        month = std::stoi(match[1]);
        day = std::stoi(match[2]);
        year = std::stoi(match[3]);
        
        // Handle 2-digit years
        if (year < 100) {
            year += (year < 50) ? 2000 : 1900;
        }
    } else if (std::regex_match(date, match, date_regex2)) {
        // DD/MM/YY or DD/MM/YYYY (European format)
        day = std::stoi(match[1]);
        month = std::stoi(match[2]);
        year = std::stoi(match[3]);
        
        // Handle 2-digit years
        if (year < 100) {
            year += (year < 50) ? 2000 : 1900;
        }
    } else if (std::regex_match(date, match, date_regex3)) {
        // YYYY/MM/DD
        year = std::stoi(match[1]);
        month = std::stoi(match[2]);
        day = std::stoi(match[3]);
    } else if (std::regex_match(date, match, date_regex4)) {
        // Month DD, YYYY
        std::string month_str = match[1];
        day = std::stoi(match[2]);
        year = std::stoi(match[3]);
        
        auto it = month_map.find(month_str);
        if (it != month_map.end()) {
            month = it->second;
        } else {
            return {false, 0};
        }
    } else if (std::regex_match(date, match, date_regex5)) {
        // DD Month YYYY
        day = std::stoi(match[1]);
        std::string month_str = match[2];
        year = std::stoi(match[3]);
        
        auto it = month_map.find(month_str);
        if (it != month_map.end()) {
            month = it->second;
        } else {
            return {false, 0};
        }
    } else if (std::regex_match(date, match, date_regex6)) {
        // YYYY, Month DD
        year = std::stoi(match[1]);
        std::string month_str = match[2];
        day = std::stoi(match[3]);
        
        auto it = month_map.find(month_str);
        if (it != month_map.end()) {
            month = it->second;
        } else {
            return {false, 0};
        }
    } else {
        // Try to extract numbers and guess the format
        std::vector<int> numbers = extractNumbers(date);
        
        if (numbers.size() >= 3) {
            // Try to determine the format based on the values
            if (numbers[0] > 31) {
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
std::pair<bool, int64_t> DateTime::parseTime(const std::string& time_str) {
    std::string time = toLower(trim(time_str));
    
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
    
    if (std::regex_match(time, match, time_regex1)) {
        // HH:MM:SS or HH:MM
        hour = std::stoi(match[1]);
        minute = std::stoi(match[2]);
        if (match[3].matched) {
            second = std::stoi(match[3]);
        }
    } else if (std::regex_match(time, match, time_regex2)) {
        // HH.MM.SS or HH.MM
        hour = std::stoi(match[1]);
        minute = std::stoi(match[2]);
        if (match[3].matched) {
            second = std::stoi(match[3]);
        }
    } else if (std::regex_match(time, match, time_regex3)) {
        // HHMMSS or HHMM
        hour = std::stoi(match[1]);
        minute = std::stoi(match[2]);
        if (match[3].matched) {
            second = std::stoi(match[3]);
        }
    } else if (std::regex_match(time, match, time_regex4)) {
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
    } else {
        // Try to extract numbers and guess the format
        std::vector<int> numbers = extractNumbers(time);
        
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

// Parse datetime from string
std::pair<bool, std::chrono::system_clock::time_point> DateTime::parseDateTime(const std::string& datetime_str) {
    std::string datetime = trim(datetime_str);
    
    // Try to split into date and time parts
    size_t space_pos = datetime.find_first_of(" \t");
    
    if (space_pos != std::string::npos) {
        // String contains both date and time
        std::string date_part = datetime.substr(0, space_pos);
        std::string time_part = datetime.substr(space_pos + 1);
        
        auto [date_success, date_int] = parseDate(date_part);
        auto [time_success, time_int] = parseTime(time_part);
        
        if (date_success && time_success) {
            // Convert to time_point
            std::tm tm = {};
            tm.tm_year = date_int / 10000 - 1900;
            tm.tm_mon = (date_int % 10000) / 100 - 1;
            tm.tm_mday = date_int % 100;
            tm.tm_hour = time_int / 10000;
            tm.tm_min = (time_int % 10000) / 100;
            tm.tm_sec = time_int % 100;
            
            std::time_t tt = std::mktime(&tm);
            return {true, std::chrono::system_clock::from_time_t(tt)};
        } else if (date_success) {
            // Only date part was valid
            std::tm tm = {};
            tm.tm_year = date_int / 10000 - 1900;
            tm.tm_mon = (date_int % 10000) / 100 - 1;
            tm.tm_mday = date_int % 100;
            
            std::time_t tt = std::mktime(&tm);
            return {true, std::chrono::system_clock::from_time_t(tt)};
        } else if (time_success) {
            // Only time part was valid
            auto now = std::chrono::system_clock::now();
            std::time_t now_tt = std::chrono::system_clock::to_time_t(now);
            std::tm* now_tm = std::localtime(&now_tt);
            
            now_tm->tm_hour = time_int / 10000;
            now_tm->tm_min = (time_int % 10000) / 100;
            now_tm->tm_sec = time_int % 100;
            
            std::time_t tt = std::mktime(now_tm);
            return {true, std::chrono::system_clock::from_time_t(tt)};
        }
    }
    
    // Try parsing as date only
    auto [date_success, date_int] = parseDate(datetime);
    if (date_success) {
        std::tm tm = {};
        tm.tm_year = date_int / 10000 - 1900;
        tm.tm_mon = (date_int % 10000) / 100 - 1;
        tm.tm_mday = date_int % 100;
        
        std::time_t tt = std::mktime(&tm);
        return {true, std::chrono::system_clock::from_time_t(tt)};
    }
    
    // Try parsing as time only
    auto [time_success, time_int] = parseTime(datetime);
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
std::pair<int64_t, int64_t> DateTime::timePointToInternalFormat(const std::chrono::system_clock::time_point& tp) {
    std::time_t tt = std::chrono::system_clock::to_time_t(tp);
    std::tm* tm = std::localtime(&tt);
    
    int64_t date_int = (tm->tm_year + 1900) * 10000 + (tm->tm_mon + 1) * 100 + tm->tm_mday;
    int64_t time_int = tm->tm_hour * 10000 + tm->tm_min * 100 + tm->tm_sec;
    
    return {date_int, time_int};
}

// Convert internal format to chrono time_point
std::chrono::system_clock::time_point DateTime::toTimePoint() const {
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
        
        std::time_t tt = std::mktime(&tm);
        return std::chrono::system_clock::from_time_t(tt);
    }
}

// Constructors
DateTime::DateTime() {
    auto now = std::chrono::system_clock::now();
    auto [date_int, time_int] = timePointToInternalFormat(now);
    
    date_series_ = pluss::value(static_cast<int64_t>(date_int));
    time_series_ = pluss::value(static_cast<int64_t>(time_int));
    has_date_ = true;
    has_time_ = true;
}

DateTime::DateTime(const std::string& datetime_str) {
    auto [success, tp] = parseDateTime(datetime_str);
    
    if (success) {
        auto [date_int, time_int] = timePointToInternalFormat(tp);
        
        // Check if the string contained a date part
        auto [date_success, _] = parseDate(datetime_str);
        has_date_ = date_success;
        
        // Check if the string contained a time part
        auto [time_success, __] = parseTime(datetime_str);
        has_time_ = time_success;
        
        if (has_date_) {
            date_series_ = Series(torch::tensor(static_cast<int64_t>(date_int)));
        }
        
        if (has_time_) {
            time_series_ = Series(torch::tensor(static_cast<int64_t>(time_int)));
        }
    } else {
        // Default to current date and time
        auto now = std::chrono::system_clock::now();
        auto [date_int, time_int] = timePointToInternalFormat(now);
        
        date_series_ = Series(torch::tensor(static_cast<int64_t>(date_int)));
        time_series_ = Series(torch::tensor(static_cast<int64_t>(time_int)));
        has_date_ = true;
        has_time_ = true;
    }
}

DateTime::DateTime(int year, int month, int day) {
    int64_t date_int = year * 10000 + month * 100 + day;
    date_series_ = Series(torch::tensor(static_cast<int64_t>(date_int)));
    has_date_ = true;
}

DateTime::DateTime(int hour, int minute, int second) {
    int64_t time_int = hour * 10000 + minute * 100 + second;
    time_series_ = Series(torch::tensor(static_cast<int64_t>(time_int)));
    has_time_ = true;
}

DateTime::DateTime(int year, int month, int day, int hour, int minute, int second) {
    int64_t date_int = year * 10000 + month * 100 + day;
    int64_t time_int = hour * 10000 + minute * 100 + second;
    
    date_series_ = Series(torch::tensor(static_cast<int64_t>(date_int)));
    time_series_ = Series(torch::tensor(static_cast<int64_t>(time_int)));
    has_date_ = true;
    has_time_ = true;
}

DateTime::DateTime(const Series& date_series, const Series& time_series) {
    if (date_series.defined()) {
        date_series_ = date_series;
        has_date_ = true;
    }
    
    if (time_series.defined()) {
        time_series_ = time_series;
        has_time_ = true;
    }
}

// Static factory methods
DateTime DateTime::today() {
    auto now = std::chrono::system_clock::now();
    auto [date_int, _] = timePointToInternalFormat(now);
    
    DateTime dt;
    dt.date_series_ = Series(torch::tensor(static_cast<int64_t>(date_int)));
    dt.has_date_ = true;
    dt.has_time_ = false;
    
    return dt;
}

DateTime DateTime::yesterday() {
    auto now = std::chrono::system_clock::now();
    auto yesterday = now - std::chrono::hours(24);
    auto [date_int, _] = timePointToInternalFormat(yesterday);
    
    DateTime dt;
    dt.date_series_ = Series(torch::tensor(static_cast<int64_t>(date_int)));
    dt.has_date_ = true;
    dt.has_time_ = false;
    
    return dt;
}

DateTime DateTime::tomorrow() {
    auto now = std::chrono::system_clock::now();
    auto tomorrow = now + std::chrono::hours(24);
    auto [date_int, _] = timePointToInternalFormat(tomorrow);
    
    DateTime dt;
    dt.date_series_ = Series(torch::tensor(static_cast<int64_t>(date_int)));
    dt.has_date_ = true;
    dt.has_time_ = false;
    
    return dt;
}

DateTime DateTime::now() {
    return DateTime();
}

DateTime DateTime::fromTimestamp(int64_t timestamp) {
    auto tp = std::chrono::system_clock::from_time_t(timestamp);
    auto [date_int, time_int] = timePointToInternalFormat(tp);
    
    DateTime dt;
    dt.date_series_ = Series(torch::tensor(static_cast<int64_t>(date_int)));
    dt.time_series_ = Series(torch::tensor(static_cast<int64_t>(time_int)));
    dt.has_date_ = true;
    dt.has_time_ = true;
    
    return dt;
}

DateTime DateTime::fromSeries(const Series& series) {
    if (series.dim() == 0) {
        // Scalar Series, interpret as date
        return DateTime(series, Series());
    } else if (series.dim() == 1 && series.size(0) == 2) {
        // 1D Series with 2 elements, interpret as [date, time]
        return DateTime(series[0], series[1]);
    } else {
        // Default to current date and time
        return DateTime();
    }
}

// Getters
int DateTime::year() const {
    if (!has_date_) return 0;
    int64_t date_val = date_series_.item<int64_t>();
    return date_val / 10000;
}

int DateTime::month() const {
    if (!has_date_) return 0;
    int64_t date_val = date_series_.item<int64_t>();
    return (date_val % 10000) / 100;
}

int DateTime::day() const {
    if (!has_date_) return 0;
    int64_t date_val = date_series_.item<int64_t>();
    return date_val % 100;
}

int DateTime::hour() const {
    if (!has_time_) return 0;
    int64_t time_val = time_series_.item<int64_t>();
    return time_val / 10000;
}

int DateTime::minute() const {
    if (!has_time_) return 0;
    int64_t time_val = time_series_.item<int64_t>();
    return (time_val % 10000) / 100;
}

int DateTime::second() const {
    if (!has_time_) return 0;
    int64_t time_val = time_series_.item<int64_t>();
    return time_val % 100;
}

int DateTime::dayOfWeek() const {
    if (!has_date_) return 0;
    
    auto tp = toTimePoint();
    std::time_t tt = std::chrono::system_clock::to_time_t(tp);
    std::tm* tm = std::localtime(&tt);
    
    return tm->tm_wday;
}

int DateTime::dayOfYear() const {
    if (!has_date_) return 0;
    
    auto tp = toTimePoint();
    std::time_t tt = std::chrono::system_clock::to_time_t(tp);
    std::tm* tm = std::localtime(&tt);
    
    return tm->tm_yday + 1;
}

bool DateTime::isLeapYear() const {
    if (!has_date_) return false;
    
    int y = year();
    return (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
}

std::string DateTime::weekdayName(bool abbreviated) const {
    if (!has_date_) return "";
    
    static const std::array<std::string, 7> full_names = {
        "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"
    };
    
    static const std::array<std::string, 7> abbr_names = {
        "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
    };
    
    int dow = dayOfWeek();
    return abbreviated ? abbr_names[dow] : full_names[dow];
}

std::string DateTime::monthName(bool abbreviated) const {
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
void DateTime::setYear(int year) {
    if (!has_date_) {
        // If no date set, use current date
        auto now = std::chrono::system_clock::now();
        auto [date_int, _] = timePointToInternalFormat(now);
        date_series_ = Series(torch::tensor(static_cast<int64_t>(date_int)));
        has_date_ = true;
    }
    
    int64_t date_val = date_series_.item<int64_t>();
    int month = (date_val % 10000) / 100;
    int day = date_val % 100;
    
    int64_t new_date = year * 10000 + month * 100 + day;
    date_series_ = Series(torch::tensor(static_cast<int64_t>(new_date)));
}

void DateTime::setMonth(int month) {
    if (!has_date_) {
        // If no date set, use current date
        auto now = std::chrono::system_clock::now();
        auto [date_int, _] = timePointToInternalFormat(now);
        date_series_ = Series(torch::tensor(static_cast<int64_t>(date_int)));
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
    date_series_ = Series(torch::tensor(static_cast<int64_t>(new_date)));
}

void DateTime::setDay(int day) {
    if (!has_date_) {
        // If no date set, use current date
        auto now = std::chrono::system_clock::now();
        auto [date_int, _] = timePointToInternalFormat(now);
        date_series_ = Series(torch::tensor(static_cast<int64_t>(date_int)));
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
    date_series_ = Series(torch::tensor(static_cast<int64_t>(new_date)));
}

void DateTime::setHour(int hour) {
    if (!has_time_) {
        // If no time set, use midnight
        time_series_ = Series(torch::tensor(static_cast<int64_t>(0)));
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
    time_series_ = Series(torch::tensor(static_cast<int64_t>(new_time)));
}

void DateTime::setMinute(int minute) {
    if (!has_time_) {
        // If no time set, use midnight
        time_series_ = Series(torch::tensor(static_cast<int64_t>(0)));
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
    time_series_ = Series(torch::tensor(static_cast<int64_t>(new_time)));
}

void DateTime::setSecond(int second) {
    if (!has_time_) {
        // If no time set, use midnight
        time_series_ = Series(torch::tensor(static_cast<int64_t>(0)));
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
    time_series_ = Series(torch::tensor(static_cast<int64_t>(new_time)));
}

// Formatting
std::string DateTime::format(const std::string& format_str) const {
    auto tp = toTimePoint();
    std::time_t tt = std::chrono::system_clock::to_time_t(tp);
    std::tm* tm = std::localtime(&tt);
    
    std::stringstream ss;
    ss << std::put_time(tm, format_str.c_str());
    return ss.str();
}

std::string DateTime::toISOString() const {
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

std::string DateTime::toString() const {
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
Series DateTime::toSeries() const {
    if (has_date_ && has_time_) {
        // Return a 1D Series with [date, time]
        return Series(torch::stack({date_series_.cpu(), time_series_.cpu()}));
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
DateTime DateTime::addYears(int years) const {
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
    
    DateTime result = *this;
    result.setYear(y);
    
    return result;
}

DateTime DateTime::addMonths(int months) const {
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
    
    DateTime result(*this);
    result.setYear(y);
    result.setMonth(m);
    result.setDay(d);
    
    return result;
}

DateTime DateTime::addDays(int days) const {
    auto tp = toTimePoint();
    tp += std::chrono::hours(24 * days);
    
    auto [date_int, time_int] = timePointToInternalFormat(tp);
    
    DateTime result(*this);
    if (has_date_) {
        result.date_series_ = Series(torch::tensor(static_cast<int64_t>(date_int)));
    }
    
    return result;
}

DateTime DateTime::addHours(int hours) const {
    auto tp = toTimePoint();
    tp += std::chrono::hours(hours);
    
    auto [date_int, time_int] = timePointToInternalFormat(tp);
    
    DateTime result(*this);
    if (has_date_) {
        result.date_series_ = Series(torch::tensor(static_cast<int64_t>(date_int)));
    }
    if (has_time_) {
        result.time_series_ = Series(torch::tensor(static_cast<int64_t>(time_int)));
    }
    
    return result;
}

DateTime DateTime::addMinutes(int minutes) const {
    auto tp = toTimePoint();
    tp += std::chrono::minutes(minutes);
    
    auto [date_int, time_int] = timePointToInternalFormat(tp);
    
    DateTime result(*this);
    if (has_date_) {
        result.date_series_ = Series(torch::tensor(static_cast<int64_t>(date_int)));
    }
    if (has_time_) {
        result.time_series_ = Series(torch::tensor(static_cast<int64_t>(time_int)));
    }
    
    return result;
}

DateTime DateTime::addSeconds(int seconds) const {
    auto tp = toTimePoint();
    tp += std::chrono::seconds(seconds);
    
    auto [date_int, time_int] = timePointToInternalFormat(tp);
    
    DateTime result(*this);
    if (has_date_) {
        result.date_series_ = Series(torch::tensor(static_cast<int64_t>(date_int)));
    }
    if (has_time_) {
        result.time_series_ = Series(torch::tensor(static_cast<int64_t>(time_int)));
    }
    
    return result;
}

// Difference calculations
int DateTime::yearsBetween(const DateTime& other) const {
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

int DateTime::monthsBetween(const DateTime& other) const {
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

int DateTime::daysBetween(const DateTime& other) const {
    if (!has_date_ || !other.has_date_) {
        return 0;
    }
    
    auto tp1 = toTimePoint();
    auto tp2 = other.toTimePoint();
    
    auto duration = tp2 - tp1;
    auto days = std::chrono::duration_cast<std::chrono::hours>(duration).count() / 24;
    
    return static_cast<int>(days);
}

int DateTime::hoursBetween(const DateTime& other) const {
    if ((!has_date_ && !has_time_) || (!other.has_date_ && !other.has_time_)) {
        return 0;
    }
    
    auto tp1 = toTimePoint();
    auto tp2 = other.toTimePoint();
    
    auto duration = tp2 - tp1;
    auto hours = std::chrono::duration_cast<std::chrono::hours>(duration).count();
    
    return static_cast<int>(hours);
}

int DateTime::minutesBetween(const DateTime& other) const {
    if ((!has_date_ && !has_time_) || (!other.has_date_ && !other.has_time_)) {
        return 0;
    }
    
    auto tp1 = toTimePoint();
    auto tp2 = other.toTimePoint();
    
    auto duration = tp2 - tp1;
    auto minutes = std::chrono::duration_cast<std::chrono::minutes>(duration).count();
    
    return static_cast<int>(minutes);
}

int DateTime::secondsBetween(const DateTime& other) const {
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
bool DateTime::operator==(const DateTime& other) const {
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

bool DateTime::operator!=(const DateTime& other) const {
    return !(*this == other);
}

bool DateTime::operator<(const DateTime& other) const {
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

bool DateTime::operator<=(const DateTime& other) const {
    return *this < other || *this == other;
}

bool DateTime::operator>(const DateTime& other) const {
    return !(*this <= other);
}

bool DateTime::operator>=(const DateTime& other) const {
    return !(*this < other);
}

} // namespace pluss
