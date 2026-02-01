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

// Allow constructing Timeline from a Note (scalar, pair-like vector, or matrix of pairs)
inline Timeline moment(const Note& n) {
    if (!n.defined()) return TimelineFactory::make_default();

    int nd = n.ndim();
    // Scalar
    if (nd == 0) return TimelineFactory::from_string(n.scalar().str());

    // 1D vector: if length == 2 treat as [date, time]
    auto shape = n.shape();
    if (nd == 1 && !shape.empty() && shape[0] == 2) {
        auto vec = n.vector();
        std::string s = vec[0].str();
        if (!vec[1].str().empty()) s += std::string(" ") + vec[1].str();
        return TimelineFactory::from_string(s);
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
            Timeline t = TimelineFactory::from_string(s);
            int64_t d = 0, tm = 0;
            if (t.has_date()) d = t.date_to_Series().item<int64_t>();
            if (t.has_time()) tm = t.time_to_Series().item<int64_t>();
            date_flat.push_back(d);
            time_flat.push_back(tm);
        }

        // Build tensors and reshape to leading shape
        std::vector<int64_t> leading_shape(shape.begin(), shape.end() - 1);
        at::Tensor date_t = torch::tensor(date_flat, torch::dtype(torch::kInt64));
        at::Tensor time_t = torch::tensor(time_flat, torch::dtype(torch::kInt64));
        if (!leading_shape.empty()) date_t = date_t.reshape(leading_shape);
        if (!leading_shape.empty()) time_t = time_t.reshape(leading_shape);

        Series ds = Series(date_t);
        Series ts = Series(time_t);
        return TimelineFactory::from_series_pair(ds, ts);
    }

    // Fallback: parse debug string
    return TimelineFactory::from_string(n.debug());
}

} // namespace pluss
