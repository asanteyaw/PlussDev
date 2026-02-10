#pragma once

#include <pluss/Timeline.h>
#include <internals/core/include/b10/deets/operations.h>
#include <internals/core/include/b10/deets/operations_tl.h>
#include <string>

namespace pluss {

// Public factories 1 preferred API for users.
inline Timeline make_timeline() {
    return b10::detail::tl_gen();
}

inline Timeline make_timeline(const std::string& s) {
    return b10::detail::tl_gen(s);
}

inline Timeline make_timeline(const char* s) {
    return b10::detail::tl_gen(std::string(s));
}

inline Timeline make_timeline(int year, int month, int day) {
    return b10::detail::tl_gen(year, month, day);
}

inline Timeline make_timeline_time(int hour, int minute, int second) {
    return b10::detail::tl_time_gen(hour, minute, second);
}

inline Timeline make_timeline(const Series& date_series, const Series& time_series) {
    return b10::detail::tl_gen(date_series, time_series);
}

inline Timeline make_timeline(const Series& series) {
    return b10::detail::tl_from_series_gen(series);
}

// moment - parse/construct from a textual/rvalue-like representation.
inline Timeline moment(const std::string& s) {
    return b10::detail::tl_gen(s);
}

// Allow constructing Timeline from a Note (scalar, pair-like vector, or matrix of pairs)
inline Timeline moment(const Note& n) {
    if (!n.defined()) return b10::detail::tl_gen();

    int nd = n.ndim();
    // Scalar
    if (nd == 0) return b10::detail::tl_gen(n.scalar().str());

    // 1D vector: if length == 2 treat as [date, time]
    auto shape = n.shape();
    if (nd == 1 && !shape.empty() && shape[0] == 2) {
        auto vec = n.vector();
        std::string s = vec[0].str();
        if (!vec[1].str().empty()) s += std::string(" ") + vec[1].str();
        return b10::detail::tl_gen(s);
    }

    // If last dimension is 2, treat that as paired date/time and build Series of the same leading shape
    if (!shape.empty() && shape.back() == 2) {
        // Helper to recursively walk the Note tree and produce combined strings for each leading-index
        std::vector<std::string> combined;
        combined.reserve(1024);

        std::function<void(const Text&, size_t)> walk = [&](const Text& node, size_t dim) {
            if (dim + 1 == shape.size()) {
                // node should be a vector of length 2 (pair)
                if (node.is_leaf()) {
                    // single scalar in last dim -> treat as scalar
                    combined.push_back(node.leaf().str());
                } else {
                    const auto& children = node.children();
                    if (children.size() == 2) {
                        std::string s = children[0].is_leaf() ? children[0].leaf().str() : children[0].children()[0].leaf().str();
                        std::string t = children[1].is_leaf() ? children[1].leaf().str() : children[1].children()[0].leaf().str();
                        if (!t.empty()) s += std::string(" ") + t;
                        combined.push_back(s);
                    } else {
                        // Fallback: join all leaf strings in node
                        std::vector<std::string> vals;
                        std::function<void(const Text&)> collect = [&](const Text& x) {
                            if (x.is_leaf()) vals.push_back(x.leaf().str());
                            else for (const auto& c : x.children()) collect(c);
                        };
                        collect(node);
                        std::string s;
                        for (size_t i = 0; i < vals.size(); ++i) { if (i) s += " "; s += vals[i]; }
                        combined.push_back(s);
                    }
                }
                return;
            }

            // intermediate dimension: iterate children
            if (node.is_leaf()) {
                // Unexpected leaf; push its string
                combined.push_back(node.leaf().str());
                return;
            }
            for (const auto& child : node.children()) walk(child, dim + 1);
        };

        walk(n.tree(), 0);

        // Parse each combined string into date/time integers
        std::vector<int64_t> date_flat;
        std::vector<int64_t> time_flat;
        date_flat.reserve(combined.size());
        time_flat.reserve(combined.size());

        for (const auto& s : combined) {
            Timeline t = b10::detail::tl_gen(s);
            int64_t d = 0, tm = 0;
            if (t.has_date()) d = t.date_to_Series().item<int64_t>();
            if (t.has_time()) tm = t.time_to_Series().item<int64_t>();
            date_flat.push_back(d);
            time_flat.push_back(tm);
        }

        // Build tensors and reshape to leading shape
        std::vector<int64_t> leading_shape(shape.begin(), shape.end() - 1);
        pluss::Series date_t = torch::series(date_flat, torch::dtype(torch::kInt64));
        pluss::Series time_t = torch::series(time_flat, torch::dtype(torch::kInt64));
        if (!leading_shape.empty()) date_t = date_t.reshape(leading_shape);
        if (!leading_shape.empty()) time_t = time_t.reshape(leading_shape);

        Series ds = b10::detail::series_gen(date_t);
        Series ts = b10::detail::series_gen(time_t);
        return b10::detail::tl_gen(ds, ts);
    }

    // Fallback: parse debug string
    return b10::detail::tl_gen(n.debug());
}

/**
 * Timeline range and sequence builders (analogous to arange/linspace for Series)
 */

// Create a range of dates from start to end with given frequency
// freq can be: "D" (day), "W" (week), "M" (month), "Y" (year)
inline Timeline date_range(const std::string& start, const std::string& end, const std::string& freq = "D") {
    Timeline start_tl = make_timeline(start);
    Timeline end_tl = make_timeline(end);
    
    std::vector<int64_t> dates;
    Timeline current = start_tl;
    
    while (current <= end_tl) {
        if (current.has_date()) {
            dates.push_back(current.date_to_Series().item<int64_t>());
        }
        
        if (freq == "D" || freq == "d") {
            current = current.add_days(1);
        } else if (freq == "W" || freq == "w") {
            current = current.add_days(7);
        } else if (freq == "M" || freq == "m") {
            current = current.add_months(1);
        } else if (freq == "Y" || freq == "y") {
            current = current.add_years(1);
        } else {
            break;  // Unknown frequency
        }
    }
    
    if (dates.empty()) return make_timeline();
    
    pluss::Series date_tensor = torch::series(dates, torch::dtype(torch::kInt64));
    Series date_series = b10::detail::series_gen(date_tensor);
    Series time_series; // Empty time component
    return make_timeline(date_series, time_series);
}

// Create a range of dates with specified number of periods
inline Timeline date_range(const std::string& start, int64_t periods, const std::string& freq = "D") {
    Timeline start_tl = make_timeline(start);
    
    std::vector<int64_t> dates;
    Timeline current = start_tl;
    
    for (int64_t i = 0; i < periods; ++i) {
        if (current.has_date()) {
            dates.push_back(current.date_to_Series().item<int64_t>());
        }
        
        if (i < periods - 1) {  // Don't advance after last date
            if (freq == "D" || freq == "d") {
                current = current.add_days(1);
            } else if (freq == "W" || freq == "w") {
                current = current.add_days(7);
            } else if (freq == "M" || freq == "m") {
                current = current.add_months(1);
            } else if (freq == "Y" || freq == "y") {
                current = current.add_years(1);
            }
        }
    }
    
    if (dates.empty()) return make_timeline();
    
    pluss::Series date_tensor = torch::series(dates, torch::dtype(torch::kInt64));
    Series date_series = b10::detail::series_gen(date_tensor);
    Series time_series;
    return make_timeline(date_series, time_series);
}

// Create a range of business days (Monday-Friday)
inline Timeline business_days(const std::string& start, const std::string& end) {
    Timeline start_tl = make_timeline(start);
    Timeline end_tl = make_timeline(end);
    
    std::vector<int64_t> dates;
    Timeline current = start_tl;
    
    while (current <= end_tl) {
        int dow = current.day_of_week();
        // 1 = Monday through 5 = Friday
        if (dow >= 1 && dow <= 5) {
            if (current.has_date()) {
                dates.push_back(current.date_to_Series().item<int64_t>());
            }
        }
        current = current.add_days(1);
    }
    
    if (dates.empty()) return make_timeline();
    
    pluss::Series date_tensor = torch::series(dates, torch::dtype(torch::kInt64));
    Series date_series = b10::detail::series_gen(date_tensor);
    Series time_series;
    return make_timeline(date_series, time_series);
}

// Create a range of business days with specified number of periods
inline Timeline business_days(const std::string& start, int64_t periods) {
    Timeline start_tl = make_timeline(start);
    
    std::vector<int64_t> dates;
    Timeline current = start_tl;
    int64_t count = 0;
    
    while (count < periods) {
        int dow = current.day_of_week();
        if (dow >= 1 && dow <= 5) {
            if (current.has_date()) {
                dates.push_back(current.date_to_Series().item<int64_t>());
                count++;
            }
        }
        if (count < periods) {
            current = current.add_days(1);
        }
    }
    
    if (dates.empty()) return make_timeline();
    
    pluss::Series date_tensor = torch::series(dates, torch::dtype(torch::kInt64));
    Series date_series = b10::detail::series_gen(date_tensor);
    Series time_series;
    return make_timeline(date_series, time_series);
}

// Create a time range within a day (e.g., hourly intervals)
inline Timeline time_range(const std::string& start_time, const std::string& end_time, int step_minutes = 60) {
    // Use a reference date for constructing times
    Timeline start_full = make_timeline("2000-01-01 " + start_time);
    Timeline end_full = make_timeline("2000-01-01 " + end_time);
    
    std::vector<int64_t> times;
    Timeline current = start_full;
    
    while (current.has_time() && end_full.has_time() &&
           current.time_to_Series().item<int64_t>() <= end_full.time_to_Series().item<int64_t>()) {
        times.push_back(current.time_to_Series().item<int64_t>());
        current = current.add_minutes(step_minutes);
    }
    
    if (times.empty()) return make_timeline();
    
    Series date_series;
    pluss::Series time_tensor = torch::series(times, torch::dtype(torch::kInt64));
    Series time_series = b10::detail::series_gen(time_tensor);
    return make_timeline(date_series, time_series);
}

// Create a Timeline for a specific month (all days in that month)
inline Timeline month_dates(int year, int month) {
    static const int days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    
    bool leap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
    int days = (month == 2 && leap) ? 29 : days_in_month[month - 1];
    
    std::vector<int64_t> dates;
    for (int day = 1; day <= days; ++day) {
        int64_t date_int = year * 10000 + month * 100 + day;
        dates.push_back(date_int);
    }
    
    pluss::Series date_tensor = torch::series(dates, torch::dtype(torch::kInt64));
    Series date_series = b10::detail::series_gen(date_tensor);
    Series time_series;
    return make_timeline(date_series, time_series);
}

// Create a Timeline for a specific year (all days in that year)
inline Timeline year_dates(int year) {
    std::string start = std::to_string(year) + "-01-01";
    std::string end = std::to_string(year) + "-12-31";
    return date_range(start, end, "D");
}

} // namespace pluss
