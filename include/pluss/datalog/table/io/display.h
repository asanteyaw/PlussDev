
#pragma once

#include <pluss/datalog/table/dataframe/dataframe.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <limits>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#if defined(__unix__) || defined(__APPLE__)
	#include <unistd.h>
	#include <sys/ioctl.h>
#endif

namespace pluss::table::io {

enum class OutputFormat {
	TEXT,   // Plain text for terminal
	HTML,   // HTML table
	LATEX   // LaTeX table
};

struct DisplayOptions {
	// Output format
	OutputFormat format = OutputFormat::TEXT;
	
	// If 0: use terminal width (or fallback).
	int max_width = 0;

	// Per-column constraints.
	int max_col_width = 24;
	int min_col_width = 4;

	// Row limiting (pandas-style: head + ellipsis + tail).
    // If <= 0: show all rows without truncation.
    // Default: show first 10 and last 10 rows (20 total) with ellipsis if total > 20
    int64_t max_rows = 20;
	int64_t head_rows = 10;
	int64_t tail_rows = 10;

	// If > 0: hard limit columns count (still uses ellipsis in the middle).
	// If 0: auto-fit columns to max_width.
	size_t max_cols = 0;

	bool show_index = true;
	bool show_summary = true; // e.g. "[100 rows x 8 columns]"

	// Formatting
	int float_precision = 4;
	bool align_numbers_right = false;  // Left-justify all content
	bool unicode_ellipsis = false; // false => "...", true => "…" (best-effort)
};

namespace detail {

inline int terminal_width_fallback() {
	if (const char* c = std::getenv("COLUMNS")) {
		try {
			const int w = std::stoi(c);
			if (w > 0) return w;
		} catch (...) {
		}
	}

#if defined(__unix__) || defined(__APPLE__)
	struct winsize ws {};
	if (::ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0) {
		return static_cast<int>(ws.ws_col);
	}
#endif

	return 120;
}

// Get current terminal width (adaptive)
inline int get_terminal_width() {
	return terminal_width_fallback();
}

inline std::string ellipsis(const DisplayOptions& opt) {
	return opt.unicode_ellipsis ? std::string("…") : std::string("...");
}

// Pandas rule: "..." if width > 3, else ".."
inline std::string dots_for_width(int width) {
	return (width > 3) ? "..." : "..";
}

inline std::string normalize_cell(std::string s) {
	for (char& ch : s) {
		if (ch == '\n' || ch == '\r' || ch == '\t') ch = ' ';
	}
	// collapse consecutive spaces (keep it simple, no locale)
	std::string out;
	out.reserve(s.size());
	bool prev_space = false;
	for (char ch : s) {
		const bool is_space = std::isspace(static_cast<unsigned char>(ch)) != 0;
		if (is_space) {
			if (!prev_space) out.push_back(' ');
			prev_space = true;
		} else {
			out.push_back(ch);
			prev_space = false;
		}
	}
	// trim
	while (!out.empty() && out.front() == ' ') out.erase(out.begin());
	while (!out.empty() && out.back() == ' ') out.pop_back();
	return out;
}

inline std::string ellipsize(std::string_view s, int width, const DisplayOptions& opt) {
	if (width <= 0) return std::string();
	const int n = static_cast<int>(s.size());
	if (n <= width) return std::string(s);

	const std::string e = ellipsis(opt);
	const int e_len = static_cast<int>(e.size());

	if (width <= e_len) return std::string(s.substr(0, static_cast<size_t>(width)));
	return std::string(s.substr(0, static_cast<size_t>(width - e_len))) + e;
}

enum class JustifyMode { LEFT, RIGHT, CENTER };

inline std::string pad(std::string_view s, int width, JustifyMode mode = JustifyMode::LEFT) {
	const int n = static_cast<int>(s.size());
	if (n >= width) return std::string(s);
	const int pad_n = width - n;
	
	switch (mode) {
	case JustifyMode::RIGHT:
		return std::string(static_cast<size_t>(pad_n), ' ') + std::string(s);
	case JustifyMode::CENTER: {
		const int left_pad = pad_n / 2;
		const int right_pad = pad_n - left_pad;
		return std::string(static_cast<size_t>(left_pad), ' ') + std::string(s) + 
		       std::string(static_cast<size_t>(right_pad), ' ');
	}
	case JustifyMode::LEFT:
	default:
		return std::string(s) + std::string(static_cast<size_t>(pad_n), ' ');
	}
}

inline bool thread_is_numeric(const pluss::table::Thread& t) {
	// Series are numeric for our purposes (even if encoded); timeline/note are not.
	return t.is_series();
}

// Cell value extraction delegated to Thread::cell_value()
inline std::string thread_cell(const pluss::table::Thread& t, int64_t idx, const DisplayOptions& /*opt*/) {
	return t.cell_value(idx);
}

inline int digits10(int64_t v) {
	if (v == 0) return 1;
	int d = 0;
	while (v != 0) {
		v /= 10;
		++d;
	}
	return d;
}

inline DisplayOptions normalize_options(DisplayOptions opt) {
	// Auto-detect terminal width if not specified
	if (opt.max_width <= 0) {
		opt.max_width = get_terminal_width();
	}
	
	if (opt.max_col_width < 4) opt.max_col_width = 4;
	if (opt.min_col_width < 1) opt.min_col_width = 1;
	if (opt.min_col_width > opt.max_col_width) opt.min_col_width = opt.max_col_width;

	if (opt.max_rows > 0) {
		if (opt.head_rows < 0) opt.head_rows = 0;
		if (opt.tail_rows < 0) opt.tail_rows = 0;
		if (opt.head_rows + opt.tail_rows > opt.max_rows) {
			// rebalance
			opt.head_rows = opt.max_rows / 2;
			opt.tail_rows = opt.max_rows - opt.head_rows;
		}
	}

	return opt;
}

inline std::vector<int64_t> selected_row_indices(int64_t total_rows, const DisplayOptions& opt, bool& rows_truncated) {
	rows_truncated = false;
	std::vector<int64_t> idx;
	if (total_rows <= 0) return idx;

	// If max_rows is 0 or negative, show all rows
	if (opt.max_rows <= 0) {
		idx.reserve(static_cast<size_t>(total_rows));
		for (int64_t i = 0; i < total_rows; ++i) idx.push_back(i);
		return idx;
	}

	// If total rows fit within max_rows, show all without truncation
	if (total_rows <= opt.max_rows) {
		idx.reserve(static_cast<size_t>(total_rows));
		for (int64_t i = 0; i < total_rows; ++i) idx.push_back(i);
		return idx;
	}

	// Need to truncate: show head + ellipsis + tail
	rows_truncated = true;
	const int64_t head_n = std::min<int64_t>(opt.head_rows, total_rows);
	const int64_t tail_n = std::min<int64_t>(opt.tail_rows, std::max<int64_t>(0, total_rows - head_n));
	
	// Special case: if tail_rows=0, show ONLY head rows without ellipsis
	// This is for head() function which wants to show exactly N rows
	if (opt.tail_rows == 0) {
		idx.reserve(static_cast<size_t>(head_n));
		for (int64_t i = 0; i < head_n; ++i) idx.push_back(i);
		return idx;
	}
	
	// Only show ellipsis if we're actually hiding rows
	if (head_n + tail_n >= total_rows) {
		// Edge case: head + tail covers everything, no ellipsis needed
		rows_truncated = false;
		idx.reserve(static_cast<size_t>(total_rows));
		for (int64_t i = 0; i < total_rows; ++i) idx.push_back(i);
		return idx;
	}
	
	idx.reserve(static_cast<size_t>(head_n + 1 + tail_n));
	for (int64_t i = 0; i < head_n; ++i) idx.push_back(i);
	idx.push_back(-1); // ellipsis row marker
	for (int64_t i = std::max<int64_t>(head_n, total_rows - tail_n); i < total_rows; ++i) idx.push_back(i);
	return idx;
}

struct ColumnView {
	// If index < 0, this is the synthetic ellipsis column.
	int index = -1;
	std::string name;
	int width = 3;
	bool numeric = false;
};

inline std::vector<ColumnView> pick_columns(const pluss::table::DataFrame& /* df */,
											const std::vector<std::string>& col_names,
											const std::vector<pluss::table::Thread>& cols,
											const std::vector<int64_t>& row_idx,
											int index_width,
											const DisplayOptions& opt,
											bool& cols_truncated) {
	cols_truncated = false;
	const size_t ncols = col_names.size();
	std::vector<int> desired_widths(ncols, opt.min_col_width);

	// Compute widths from the actually displayed row sample.
	for (size_t ci = 0; ci < ncols; ++ci) {
		int w = static_cast<int>(col_names[ci].size());
		w = std::min(w, opt.max_col_width);
		for (int64_t r : row_idx) {
			if (r < 0) continue;
			const std::string cell = normalize_cell(thread_cell(cols[ci], r, opt));
			w = std::max<int>(w, static_cast<int>(cell.size()));
			if (w >= opt.max_col_width) {
				w = opt.max_col_width;
				break;
			}
		}
		w = std::max(w, opt.min_col_width);
		desired_widths[ci] = w;
	}

	const int term_w = (opt.max_width > 0) ? opt.max_width : terminal_width_fallback();
	const int max_w = std::max(20, term_w);

	// Layout cost model: [index][space] + for each col: width + space
	const int base_used = opt.show_index ? (index_width + 1) : 0;
	int available = max_w - base_used;
	if (available <= 0) available = 1;

	auto build_view = [&](int ci) -> ColumnView {
		ColumnView v;
		v.index = ci;
		v.name = col_names[static_cast<size_t>(ci)];
		v.width = desired_widths[static_cast<size_t>(ci)];
		v.numeric = thread_is_numeric(cols[static_cast<size_t>(ci)]);
		return v;
	};

	// If max_cols is set, enforce it (pandas style: keep first + last).
	if (opt.max_cols > 0 && ncols > opt.max_cols) {
		cols_truncated = true;
		const size_t keep = opt.max_cols;
		const size_t left_keep = keep / 2;
		const size_t right_keep = keep - left_keep;
		std::vector<ColumnView> out;
		out.reserve(left_keep + 1 + right_keep);
		for (size_t i = 0; i < left_keep; ++i) out.push_back(build_view(static_cast<int>(i)));
		out.push_back(ColumnView{-1, ellipsis(opt), 3, false});
		for (size_t i = ncols - right_keep; i < ncols; ++i) out.push_back(build_view(static_cast<int>(i)));
		return out;
	}

	// Auto-fit to width; if truncated, include a middle "..." column.
	// Strategy: take as many from the left as possible while reserving room
	// for an ellipsis column plus at least one column from the right.
	std::vector<int> left;
	std::vector<int> right;

	int used_left = 0;
	int used_right = 0;

	auto col_cost = [&](int ci) { return desired_widths[static_cast<size_t>(ci)] + 1; };
	const int ellipsis_cost = 3 + 1;

	if (ncols == 0) return {};

	size_t li = 0;
	size_t ri = (ncols == 0) ? 0 : (ncols - 1);

	// Reserve for at least one right column if we have > 1 total.
	while (li < ncols) {
		if (li > ri) break;

		// Decide whether we still need to reserve space for right side + ellipsis.
		const bool would_truncate = (li <= ri && (li != 0 || ri != ncols - 1));
		const bool remaining_right = (ri > li);
		int reserve = 0;
		if (remaining_right) {
			reserve += ellipsis_cost;
			reserve += col_cost(static_cast<int>(ri));
		}

		const int next_cost = col_cost(static_cast<int>(li));
		if (used_left + next_cost + used_right + reserve > available) break;
		left.push_back(static_cast<int>(li));
		used_left += next_cost;
		++li;
	}

	// Now add right columns (excluding those already taken from the left).
	while (ri >= li && ncols > 0) {
		const int next_cost = col_cost(static_cast<int>(ri));
		const bool need_ellipsis = (li <= ri);
		const int reserve = need_ellipsis ? ellipsis_cost : 0;
		if (used_left + used_right + next_cost + reserve > available) break;
		right.push_back(static_cast<int>(ri));
		used_right += next_cost;
		if (ri == 0) break;
		--ri;
	}

	// If we couldn't fit anything on the left, try at least the first column.
	if (left.empty() && ncols > 0) {
		left.push_back(0);
	}

	// Determine whether we dropped any columns.
	std::vector<bool> taken(ncols, false);
	for (int c : left) taken[static_cast<size_t>(c)] = true;
	for (int c : right) taken[static_cast<size_t>(c)] = true;
	bool any_dropped = false;
	for (bool t : taken) {
		if (!t) { any_dropped = true; break; }
	}
	cols_truncated = any_dropped;

	std::vector<ColumnView> out;
	out.reserve(left.size() + (cols_truncated ? 1 : 0) + right.size());
	for (int c : left) out.push_back(build_view(c));
	// Pandas uses " ..." (space-prefixed, 4 chars) for column ellipsis
	if (cols_truncated) out.push_back(ColumnView{-1, " ...", 4, false});
	std::sort(right.begin(), right.end());
	for (int c : right) out.push_back(build_view(c));
	return out;
}

} // namespace detail

inline std::string to_string(const pluss::table::DataFrame& df, DisplayOptions opt = {}) {
	opt = detail::normalize_options(std::move(opt));

	const auto col_names = df.columns();
	if (col_names.empty()) return "Empty DataFrame\n";

	std::vector<pluss::table::Thread> cols;
	cols.reserve(col_names.size());
	for (const auto& name : col_names) cols.push_back(df.get_col(name));

	const int64_t total_rows = cols.front().size();
	bool rows_truncated = false;
	const auto row_idx = detail::selected_row_indices(total_rows, opt, rows_truncated);

	const int64_t last_idx = (total_rows == 0) ? 0 : (total_rows - 1);
	int index_width = std::max(1, detail::digits10(last_idx));
	if (rows_truncated) index_width = std::max(index_width, 3);

	bool cols_truncated = false;
	auto view = detail::pick_columns(df, col_names, cols, row_idx, index_width, opt, cols_truncated);
	if (cols_truncated) index_width = std::max(index_width, 3);

	// Final safety clamp for each column width (min/max already applied in desired widths).
	for (auto& c : view) {
		if (c.index < 0) {
			c.width = 3;
			continue;
		}
		c.width = std::clamp(c.width, opt.min_col_width, opt.max_col_width);
	}

	std::ostringstream oss;

	// Header
	if (opt.show_index) {
		// Pandas: index column header is right-justified
		oss << detail::pad("", index_width, detail::JustifyMode::RIGHT) << ' ';
	}
	for (const auto& c : view) {
		// Pandas: column headers are right-justified if numeric column
		auto mode = (opt.align_numbers_right && c.numeric) ? detail::JustifyMode::RIGHT : detail::JustifyMode::LEFT;
		oss << detail::pad(detail::ellipsize(c.name, c.width, opt), c.width, mode) << ' ';
	}
	oss << '\n';

	// Separator (index column has no name, so no underline for it)
	if (opt.show_index) {
		oss << std::string(static_cast<size_t>(index_width), ' ') << ' ';
	}
	for (const auto& c : view) {
		oss << std::string(static_cast<size_t>(std::max(1, c.width)), '-') << ' ';
	}
	oss << '\n';

	// Body
	for (int64_t r : row_idx) {
		const bool is_ellipsis_row = (r < 0);
		if (opt.show_index) {
			std::string idx_s;
			if (is_ellipsis_row) {
				// Pandas: index column uses left-justified dots
				idx_s = detail::dots_for_width(index_width);
				oss << detail::pad(idx_s, index_width, detail::JustifyMode::LEFT) << ' ';
			} else {
				idx_s = std::to_string(r);
				oss << detail::pad(detail::ellipsize(idx_s, index_width, opt), index_width, detail::JustifyMode::LEFT) << ' ';
			}
		}

		for (const auto& c : view) {
			std::string cell;
			detail::JustifyMode mode = detail::JustifyMode::LEFT;
			
			if (is_ellipsis_row) {
				// Pandas: ellipsis row uses right-justified dots with width rule
				cell = detail::dots_for_width(c.width);
				mode = detail::JustifyMode::RIGHT;
			} else if (c.index < 0) {
				// Pandas: ellipsis column uses " ..." (space-prefixed, 4 chars)
				cell = " ...";
				mode = detail::JustifyMode::RIGHT;
			} else {
				cell = detail::normalize_cell(detail::thread_cell(cols[static_cast<size_t>(c.index)], r, opt));
				mode = (opt.align_numbers_right && c.numeric) ? detail::JustifyMode::RIGHT : detail::JustifyMode::LEFT;
			}
			
			cell = detail::ellipsize(cell, c.width, opt);
			oss << detail::pad(cell, c.width, mode) << ' ';
		}
		oss << '\n';
	}

	if (opt.show_summary) {
		// pandas-like summary line
		oss << "\n[" << total_rows << " rows x " << col_names.size() << " columns]";
		oss << "\n";
	}

	return oss.str();
}

inline std::ostream& print(std::ostream& os, const pluss::table::DataFrame& df, DisplayOptions opt = {}) {
	os << to_string(df, std::move(opt));
	return os;
}

// HTML table formatter
inline std::string to_html(const pluss::table::DataFrame& df, DisplayOptions opt = {}) {
	opt = detail::normalize_options(std::move(opt));
	
	const auto col_names = df.columns();
	if (col_names.empty()) return "<p>Empty DataFrame</p>\n";
	
	std::vector<pluss::table::Thread> cols;
	cols.reserve(col_names.size());
	for (const auto& name : col_names) cols.push_back(df.get_col(name));
	
	const int64_t total_rows = cols.front().size();
	bool rows_truncated = false;
	const auto row_idx = detail::selected_row_indices(total_rows, opt, rows_truncated);
	
	std::ostringstream oss;
	oss << "<table border=\"1\" class=\"dataframe\">\n";
	
	// Header
	oss << "  <thead>\n    <tr style=\"text-align: right;\">\n";
	if (opt.show_index) {
		oss << "      <th></th>\n";
	}
	for (const auto& name : col_names) {
		oss << "      <th>" << name << "</th>\n";
	}
	oss << "    </tr>\n  </thead>\n";
	
	// Body
	oss << "  <tbody>\n";
	for (const auto r : row_idx) {
		if (r < 0) {
			oss << "    <tr>\n";
			if (opt.show_index) oss << "      <th>...</th>\n";
			for (size_t i = 0; i < col_names.size(); ++i) {
				oss << "      <td>...</td>\n";
			}
			oss << "    </tr>\n";
			continue;
		}
		
		oss << "    <tr>\n";
		if (opt.show_index) {
			oss << "      <th>" << r << "</th>\n";
		}
		for (size_t ci = 0; ci < col_names.size(); ++ci) {
			std::string cell = detail::normalize_cell(detail::thread_cell(cols[ci], r, opt));
			oss << "      <td>" << cell << "</td>\n";
		}
		oss << "    </tr>\n";
	}
	oss << "  </tbody>\n</table>";
	
	if (opt.show_summary) {
		oss << "\n<p>" << total_rows << " rows × " << col_names.size() << " columns</p>";
	}
	
	return oss.str();
}

// LaTeX table formatter
inline std::string to_latex(const pluss::table::DataFrame& df, DisplayOptions opt = {}) {
	opt = detail::normalize_options(std::move(opt));
	
	const auto col_names = df.columns();
	if (col_names.empty()) return "Empty DataFrame\n";
	
	std::vector<pluss::table::Thread> cols;
	cols.reserve(col_names.size());
	for (const auto& name : col_names) cols.push_back(df.get_col(name));
	
	const int64_t total_rows = cols.front().size();
	bool rows_truncated = false;
	const auto row_idx = detail::selected_row_indices(total_rows, opt, rows_truncated);
	
	std::ostringstream oss;
	
	// Table header
	oss << "\\begin{tabular}{";
	if (opt.show_index) oss << "l";
	for (size_t i = 0; i < col_names.size(); ++i) {
		oss << "r";  // right-align data columns
	}
	oss << "}\n\\toprule\n";
	
	// Column names
	if (opt.show_index) oss << " & ";
	for (size_t i = 0; i < col_names.size(); ++i) {
		oss << col_names[i];
		if (i < col_names.size() - 1) oss << " & ";
	}
	oss << " \\\\\\\\\n\\midrule\n";
	
	// Data rows
	for (const auto r : row_idx) {
		if (r < 0) {
			if (opt.show_index) oss << "\\ldots & ";
			for (size_t i = 0; i < col_names.size(); ++i) {
				oss << "\\ldots";
				if (i < col_names.size() - 1) oss << " & ";
			}
			oss << " \\\\\\\\\n";
			continue;
		}
		
		if (opt.show_index) {
			oss << r << " & ";
		}
		for (size_t ci = 0; ci < col_names.size(); ++ci) {
			std::string cell = detail::normalize_cell(detail::thread_cell(cols[ci], r, opt));
			// Escape LaTeX special characters
			std::string escaped;
			for (char ch : cell) {
				if (ch == '_' || ch == '%' || ch == '$' || ch == '#' || ch == '&') {
					escaped += '\\';
				}
				escaped += ch;
			}
			oss << escaped;
			if (ci < col_names.size() - 1) oss << " & ";
		}
		oss << " \\\\\\\\\n";
	}
	
	oss << "\\bottomrule\n\\end{tabular}\n";
	
	if (opt.show_summary) {
		oss << "% " << total_rows << " rows × " << col_names.size() << " columns\n";
	}
	
	return oss.str();
}

} // namespace pluss::table::io

namespace pluss::table {
// Default DataFrame streaming: pandas-like, width-aware, with summary.
inline std::ostream& operator<<(std::ostream& os, const DataFrame& df) {
    return pluss::table::io::print(os, df);
}
} // namespace pluss::table
