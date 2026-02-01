
// High-level, pandas-like presentation helpers.
// Core storage/selection logic lives in dataframe_impl.cpp.

#include <pluss/datalog/table/dataframe/dataframe.h>
#include <pluss/datalog/table/thread/thread.h>

#include <pluss/pluss.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include <unistd.h>
#include <sys/ioctl.h>

namespace pluss::table {

namespace {

int terminal_width() {
    if (const char* c = std::getenv("COLUMNS")) {
        try {
            int w = std::stoi(c);
            if (w > 0) return w;
        } catch (...) {
        }
    }
    struct winsize ws {};
    if (::ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0) return static_cast<int>(ws.ws_col);
    return 120;
}

std::string trunc(const std::string& s, int width) {
    if (width <= 0) return std::string();
    if (static_cast<int>(s.size()) <= width) return s;
    if (width <= 3) return s.substr(0, static_cast<size_t>(width));
    return s.substr(0, static_cast<size_t>(width - 3)) + "...";
}

std::string series_cell(const pluss::Series& s, int64_t idx) {
    try {
        auto t = at::Tensor(static_cast<const at::TensorBase&>(s));
        if (t.numel() == 0) return std::string();
        auto flat = t.view({-1});
        auto val = flat[idx];
        if (val.is_floating_point()) {
            float f = val.item<float>();
            if (std::isnan(f)) return "nan";
            std::ostringstream oss;
            if (std::floor(f) == f) oss << std::fixed << std::setprecision(0) << f;
            else oss << std::fixed << std::setprecision(4) << f;
            return oss.str();
        }
        return std::to_string(val.item<int64_t>());
    } catch (...) {
        return std::string();
    }
}

std::string timeline_cell(const pluss::Timeline& tl, int64_t idx) {
    try {
        auto ds_t = at::Tensor(static_cast<const at::TensorBase&>(tl.date_to_Series()));
        auto ts_t = at::Tensor(static_cast<const at::TensorBase&>(tl.time_to_Series()));
        const bool ds_scalar = (ds_t.numel() == 1);
        const bool ts_scalar = (ts_t.numel() == 1);
        if (ds_scalar && ts_scalar) return tl.to_string();

        int64_t d = 0;
        int64_t tm = 0;
        if (ds_t.numel() > 0) d = ds_t.view({-1})[idx].item<int64_t>();
        if (ts_t.numel() > 0) tm = ts_t.view({-1})[idx].item<int64_t>();
        if (d == 0 && tm == 0) return std::string();

        std::ostringstream oss;
        if (d != 0) {
            int y = static_cast<int>(d / 10000);
            int mo = static_cast<int>((d / 100) % 100);
            int da = static_cast<int>(d % 100);
            oss << std::setw(4) << std::setfill('0') << y << "-" << std::setw(2) << mo << "-" << std::setw(2) << da;
        }
        if (tm != 0) {
            int h = static_cast<int>(tm / 10000);
            int mi = static_cast<int>((tm / 100) % 100);
            int se = static_cast<int>(tm % 100);
            if (d != 0) oss << " ";
            oss << std::setw(2) << std::setfill('0') << h << ":" << std::setw(2) << mi << ":" << std::setw(2) << se;
        }
        return oss.str();
    } catch (...) {
        return std::string();
    }
}

std::string thread_cell(const pluss::table::Thread& t, int64_t idx) {
    if (t.is_series()) return series_cell(t.as_series(), idx);
    if (t.is_timeline()) return timeline_cell(t.as_timeline(), idx);
    if (t.is_note()) {
        try {
            auto v = t.as_note().vector();
            if (static_cast<size_t>(idx) < v.size()) return v[idx].str();
        } catch (...) {
        }
    }
    return std::string();
}

int64_t non_null_count(const pluss::table::Thread& t) {
    try {
        if (t.is_series()) {
            auto tt = at::Tensor(static_cast<const at::TensorBase&>(t.as_series())).view({-1});
            if (tt.numel() == 0) return 0;
            if (tt.is_floating_point()) return (tt == tt).sum().item<int64_t>(); // NaN != NaN
            return tt.numel();
        }
        if (t.is_timeline()) {
            const auto& tl = t.as_timeline();
            auto ds = at::Tensor(static_cast<const at::TensorBase&>(tl.date_to_Series())).view({-1});
            auto ts = at::Tensor(static_cast<const at::TensorBase&>(tl.time_to_Series())).view({-1});
            const int64_t n = std::max<int64_t>(ds.numel(), ts.numel());
            if (n == 0) return 0;
            if (ds.numel() == 0) return (ts != 0).sum().item<int64_t>();
            if (ts.numel() == 0) return (ds != 0).sum().item<int64_t>();
            return ((ds != 0) | (ts != 0)).sum().item<int64_t>();
        }
        if (t.is_note()) {
            auto v = t.as_note().vector();
            int64_t c = 0;
            for (const auto& chord : v) if (!chord.str().empty()) ++c;
            return c;
        }
    } catch (...) {
    }
    return t.size();
}

} // namespace

void DataFrame::head(int64_t n) const {
    auto col_names = this->columns();
    if (col_names.empty()) {
        std::cout << "Empty DataFrame\n";
        return;
    }

    std::vector<pluss::table::Thread> cols;
    cols.reserve(col_names.size());
    for (const auto& name : col_names) cols.push_back(this->get_col(name));

    const int64_t rows = cols.front().size();
    const int64_t rows_to_show = std::min<int64_t>(n, rows);

    const int w_term = terminal_width();
    const int max_col_width = 24;
    const int min_col_width = 4;

    const int64_t last_idx = (rows == 0) ? 0 : (rows - 1);
    int index_width = static_cast<int>(std::to_string(last_idx).size());
    index_width = std::max(index_width, 1);

    // compute desired widths
    std::vector<int> widths;
    widths.reserve(col_names.size());
    for (size_t ci = 0; ci < col_names.size(); ++ci) {
        int w = static_cast<int>(col_names[ci].size());
        w = std::min(w, max_col_width);
        for (int64_t ri = 0; ri < rows_to_show; ++ri) {
            w = std::max<int>(w, static_cast<int>(thread_cell(cols[ci], ri).size()));
            if (w >= max_col_width) { w = max_col_width; break; }
        }
        w = std::max(w, min_col_width);
        widths.push_back(w);
    }

    // choose columns that fit the terminal width (account for index + spaces)
    std::vector<size_t> shown;
    int used = index_width + 1;
    for (size_t i = 0; i < col_names.size(); ++i) {
        const int add = widths[i] + 1;
        const bool would_hide = (i + 1 < col_names.size());
        const int reserve = would_hide ? 4 : 0;
        if (used + add + reserve > w_term) break;
        shown.push_back(i);
        used += add;
    }
    const bool truncated_cols = (shown.size() < col_names.size());

    // header
    std::cout << std::setw(index_width) << std::right << "" << " ";
    for (auto i : shown) {
        std::cout << std::setw(widths[i]) << std::left << trunc(col_names[i], widths[i]) << " ";
    }
    if (truncated_cols) std::cout << "...";
    std::cout << "\n";

    // separator
    std::cout << std::string(static_cast<size_t>(index_width), '-') << " ";
    for (auto i : shown) {
        std::cout << std::string(static_cast<size_t>(widths[i]), '-') << " ";
    }
    if (truncated_cols) std::cout << "---";
    std::cout << "\n";

    for (int64_t ri = 0; ri < rows_to_show; ++ri) {
        std::cout << std::setw(index_width) << std::right << ri << " ";
        for (auto i : shown) {
            std::cout << std::setw(widths[i]) << std::left << trunc(thread_cell(cols[i], ri), widths[i]) << " ";
        }
        if (truncated_cols) std::cout << "...";
        std::cout << "\n";
    }
}

void DataFrame::tail(int64_t n) const {
    auto col_names = this->columns();
    if (col_names.empty()) {
        std::cout << "Empty DataFrame\n";
        return;
    }
    const int64_t rows = this->get_col(col_names.front()).size();
    const int64_t start = std::max<int64_t>(0, rows - n);
    auto df_tail = this->iloc(start, rows);
    df_tail->head(n);
}

void DataFrame::info() const {
    auto col_names = this->columns();
    const size_t num_cols = col_names.size();

    size_t num_rows = 0;
    if (!col_names.empty()) num_rows = static_cast<size_t>(this->get_col(col_names.front()).size());

    std::cout << "<class 'pluss::table::DataFrame'>\n";
    std::cout << "Index: 0 to " << (num_rows == 0 ? 0 : num_rows - 1) << "\n";
    std::cout << "Data columns (total " << num_cols << "):\n";

    std::cout << std::left << std::setw(5) << "#" << std::setw(24) << "Column" << std::setw(18) << "Non-Null Count" << std::setw(10) << "Dtype" << "\n";
    std::cout << std::string(60, '-') << "\n";

    for (size_t i = 0; i < col_names.size(); ++i) {
        const auto t = this->get_col(col_names[i]);
        const std::string dtype = t.is_series() ? "Series" : (t.is_timeline() ? "Timeline" : "Note");
        std::ostringstream nn;
        nn << non_null_count(t) << " non-null";
        std::cout << std::setw(5) << static_cast<int>(i)
                  << std::setw(24) << col_names[i]
                  << std::setw(18) << nn.str()
                  << std::setw(10) << dtype
                  << "\n";
    }
}

void DataFrame::describe() const {
    auto col_names = this->columns();
    if (col_names.empty()) {
        std::cout << "Empty DataFrame\n";
        return;
    }

    std::vector<std::string> stat_cols = {"column","count","mean","std","min","25%","50%","75%","max"};
    std::cout << std::setw(14) << std::right << stat_cols[0];
    for (size_t i = 1; i < stat_cols.size(); ++i) std::cout << std::setw(14) << std::right << stat_cols[i];
    std::cout << "\n" << std::string(14 * stat_cols.size(), '-') << "\n";

    for (const auto& name : col_names) {
        const auto t = this->get_col(name);
        if (!(t.is_series() || t.is_timeline())) continue;
        try {
            auto s = t.to_series();
            auto tt = s.to(config({{"dtype","float32"}}));
            float mean = tt.mean().item<float>();
            float stddev = tt.std().item<float>();
            float min = tt.min().item<float>();
            float q25 = tt.quantile(0.25).item<float>();
            float median = tt.median().item<float>();
            float q75 = tt.quantile(0.75).item<float>();
            float max = tt.max().item<float>();
            std::cout << std::setw(14) << std::right << name
                      << std::setw(14) << std::right << static_cast<float>(tt.numel())
                      << std::setw(14) << std::right << mean
                      << std::setw(14) << std::right << stddev
                      << std::setw(14) << std::right << min
                      << std::setw(14) << std::right << q25
                      << std::setw(14) << std::right << median
                      << std::setw(14) << std::right << q75
                      << std::setw(14) << std::right << max << "\n";
        } catch (...) {
            // best-effort: skip columns that can't be summarized
        }
    }
}

} // namespace pluss::table

