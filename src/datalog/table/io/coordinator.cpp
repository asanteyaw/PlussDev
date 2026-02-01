#include <pluss/pluss.h>
#include <pluss/note_factory.h>
#include <pluss/tl_factory.h>
#include <torch/torch.h>
#include <cmath>
#include <string>
#include <vector>
#include <charconv>
#include <cerrno>
#include <cstdlib>
#include <regex>

namespace {

inline std::string trim_copy(const std::string& s) {
    auto first = s.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return std::string();
    auto last = s.find_last_not_of(" \t\r\n");
    return s.substr(first, last - first + 1);
}

inline bool is_missing_value(const std::string& raw) {
    auto s = trim_copy(raw);
    if (s.empty()) return true;
    for (auto& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s == "nan" || s == "na" || s == "null" || s == "none";
}

inline bool try_parse_int64(const std::string& raw, int64_t& out) {
    auto s = trim_copy(raw);
    if (s.empty()) return false;
    long long tmp = 0;
    auto res = std::from_chars(s.data(), s.data() + s.size(), tmp);
    if (res.ec == std::errc() && res.ptr == s.data() + s.size()) {
        out = static_cast<int64_t>(tmp);
        return true;
    }
    return false;
}

inline bool try_parse_double(const std::string& raw, double& out) {
    auto s = trim_copy(raw);
    if (s.empty()) return false;
    char* endptr = nullptr;
    errno = 0;
    const double v = std::strtod(s.c_str(), &endptr);
    if (endptr != s.c_str() && *endptr == '\0' && errno == 0) {
        out = v;
        return true;
    }
    return false;
}

inline bool looks_like_datetime(const std::string& raw) {
    auto s = trim_copy(raw);
    if (s.empty()) return false;
    // Treat common separators as datetime-like.
    if ((s.find('-') != std::string::npos) || (s.find('/') != std::string::npos) ||
        (s.find(':') != std::string::npos) || (s.find('T') != std::string::npos) ||
        (s.find('t') != std::string::npos)) {
        return true;
    }

    // Avoid classifying arbitrary integers (IDs) as datetime, but do allow compact dates like YYYYMMDD.
    const bool all_digits = std::all_of(s.begin(), s.end(), [](unsigned char c) { return std::isdigit(c); });
    if (!all_digits) return false;
    if (s.size() == 8 && (s.rfind("19", 0) == 0 || s.rfind("20", 0) == 0)) return true;
    return false;
}

enum class DateOrder { AUTO, MDY, DMY };

inline bool parse_date_time_basic(const std::string& raw,
                                  int64_t& date_out,
                                  int64_t& time_out,
                                  bool& has_date_out,
                                  bool& has_time_out,
                                  DateOrder order = DateOrder::AUTO) {
    date_out = 0;
    time_out = 0;
    has_date_out = false;
    has_time_out = false;

    const std::string s = trim_copy(raw);
    if (s.empty()) return false;

    auto parse_date = [&](const std::string& d) -> bool {
        static const std::regex ymd(R"(^\s*(\d{4})[-/](\d{1,2})[-/](\d{1,2})\s*$)");
        static const std::regex mdy_or_dmy(R"(^\s*(\d{1,2})[-/](\d{1,2})[-/](\d{4})\s*$)");
        static const std::regex yyyymmdd(R"(^\s*(\d{4})(\d{2})(\d{2})\s*$)");
        std::smatch m;
        int y = 0, mo = 0, da = 0;
        if (std::regex_match(d, m, ymd)) {
            y = std::stoi(m[1].str());
            mo = std::stoi(m[2].str());
            da = std::stoi(m[3].str());
        } else if (std::regex_match(d, m, mdy_or_dmy)) {
            const int a = std::stoi(m[1].str());
            const int b = std::stoi(m[2].str());
            y = std::stoi(m[3].str());
            auto interpret_mdy = [&]() {
                mo = a;
                da = b;
            };
            auto interpret_dmy = [&]() {
                da = a;
                mo = b;
            };

            // Unambiguous hints first.
            if (a > 12) {
                interpret_dmy();
            } else if (b > 12) {
                interpret_mdy();
            } else {
                // Ambiguous: follow requested order.
                switch (order) {
                    case DateOrder::MDY: interpret_mdy(); break;
                    case DateOrder::DMY: interpret_dmy(); break;
                    case DateOrder::AUTO:
                    default:
                        // AUTO means "unknown" at the token level; caller should prefer a column-wide choice.
                        interpret_mdy();
                        break;
                }
            }
        } else if (std::regex_match(d, m, yyyymmdd)) {
            y = std::stoi(m[1].str());
            mo = std::stoi(m[2].str());
            da = std::stoi(m[3].str());
        } else {
            return false;
        }
        if (y <= 0 || mo <= 0 || mo > 12 || da <= 0 || da > 31) return false;
        date_out = static_cast<int64_t>(y) * 10000 + static_cast<int64_t>(mo) * 100 + static_cast<int64_t>(da);
        has_date_out = true;
        return true;
    };

    auto parse_time = [&](const std::string& t) -> bool {
        static const std::regex hms(R"(^\s*(\d{1,2}):(\d{2})(?::(\d{2}))?\s*$)");
        std::smatch m;
        int h = 0, mi = 0, se = 0;
        if (!std::regex_match(t, m, hms)) return false;
        h = std::stoi(m[1].str());
        mi = std::stoi(m[2].str());
        se = m[3].matched ? std::stoi(m[3].str()) : 0;
        if (h < 0 || h > 23 || mi < 0 || mi > 59 || se < 0 || se > 59) return false;
        time_out = static_cast<int64_t>(h) * 10000 + static_cast<int64_t>(mi) * 100 + static_cast<int64_t>(se);
        has_time_out = true;
        return true;
    };

    // split into date/time if needed
    const auto tpos = s.find('T');
    const auto spos = s.find(' ');
    if (tpos != std::string::npos) {
        return parse_date(s.substr(0, tpos)) && parse_time(s.substr(tpos + 1));
    }
    if (spos != std::string::npos) {
        // allow "date time" or "time date" (prefer date first)
        const auto left = trim_copy(s.substr(0, spos));
        const auto right = trim_copy(s.substr(spos + 1));
        bool ok_date_left = parse_date(left);
        bool ok_time_right = parse_time(right);
        if (ok_date_left && ok_time_right) return true;

        // reset and try swapped
        date_out = 0; time_out = 0; has_date_out = false; has_time_out = false;
        bool ok_time_left = parse_time(left);
        bool ok_date_right = parse_date(right);
        return ok_time_left && ok_date_right;
    }

    // date-only or time-only
    if (s.find(':') != std::string::npos) return parse_time(s);
    return parse_date(s);
}

} // namespace

namespace pluss::table::io {

std::shared_ptr<pluss::table::DataFrame> convert_raw_to_dataframe(const RawTable& raw) {
    auto df = std::make_shared<pluss::table::DataFrame>();

    for (const auto& col : raw.columns) {
        const auto& values = raw.data.at(col);

        // Decide per-column type:
        // - numeric -> Series
        // - datetime-like -> Timeline
        // - otherwise -> Note
        size_t non_missing = 0;
        size_t int_ok = 0;
        size_t float_ok = 0;
        size_t dt_like = 0;
        bool any_missing = false;

        for (const auto& raw_v : values) {
            if (is_missing_value(raw_v)) {
                any_missing = true;
                continue;
            }
            ++non_missing;

            int64_t tmp_i = 0;
            if (try_parse_int64(raw_v, tmp_i)) ++int_ok;

            double tmp_d = 0.0;
            if (try_parse_double(raw_v, tmp_d)) ++float_ok;

            if (looks_like_datetime(raw_v)) ++dt_like;
        }

        const bool all_int = (non_missing > 0) && (int_ok == non_missing);
        const bool all_float = (non_missing > 0) && (float_ok == non_missing);

        // Datetime heuristics: require "datetime-looking" strings AND parseable by our basic parser.
        // (This avoids turning categorical strings into datetimes and avoids year-like integers.)
        size_t dt_parse_ok_dmy = 0;
        size_t dt_parse_ok_mdy = 0;
        bool saw_dmy_hint = false; // e.g. 22/09/2011
        bool saw_mdy_hint = false; // e.g. 09/22/2011
        bool any_time_component = false;
        bool any_date_component = false;
        if (non_missing > 0 && dt_like > 0) {
            for (const auto& raw_v : values) {
                if (is_missing_value(raw_v)) continue;
                if (!looks_like_datetime(raw_v)) continue;

                // Collect hints for slash/dash date strings.
                // If the first component > 12, it can't be a month => DMY.
                // If the second component > 12, it can't be a month => MDY.
                {
                    static const std::regex mdy_or_dmy(R"(^\s*(\d{1,2})[-/](\d{1,2})[-/](\d{4})\s*$)");
                    std::smatch m;
                    const std::string trimmed_v = trim_copy(raw_v);
                    if (std::regex_match(trimmed_v, m, mdy_or_dmy)) {
                        const int a = std::stoi(m[1].str());
                        const int b = std::stoi(m[2].str());
                        if (a > 12) saw_dmy_hint = true;
                        if (b > 12) saw_mdy_hint = true;
                    }
                }

                // Score parse success under explicit DMY/MDY choices for ambiguous dates.
                int64_t d_dmy = 0, tm_dmy = 0;
                bool hd_dmy = false, ht_dmy = false;
                const bool ok_dmy = parse_date_time_basic(raw_v, d_dmy, tm_dmy, hd_dmy, ht_dmy, DateOrder::DMY);
                if (ok_dmy) ++dt_parse_ok_dmy;

                int64_t d_mdy = 0, tm_mdy = 0;
                bool hd_mdy = false, ht_mdy = false;
                const bool ok_mdy = parse_date_time_basic(raw_v, d_mdy, tm_mdy, hd_mdy, ht_mdy, DateOrder::MDY);
                if (ok_mdy) ++dt_parse_ok_mdy;

                if (ok_dmy || ok_mdy) {
                    any_time_component = any_time_component || ht_dmy || ht_mdy;
                    any_date_component = any_date_component || hd_dmy || hd_mdy;
                }
            }
        }
        const size_t dt_parse_ok_best = std::max(dt_parse_ok_dmy, dt_parse_ok_mdy);
        const bool is_datetime_col = (dt_parse_ok_best > 0) && (static_cast<double>(dt_parse_ok_best) / static_cast<double>(std::max<size_t>(1, dt_like)) >= 0.80);

        DateOrder chosen_order = DateOrder::MDY;
        if (saw_dmy_hint && !saw_mdy_hint) chosen_order = DateOrder::DMY;
        else if (saw_mdy_hint && !saw_dmy_hint) chosen_order = DateOrder::MDY;
        else if (dt_parse_ok_dmy > dt_parse_ok_mdy) chosen_order = DateOrder::DMY;
        else if (dt_parse_ok_mdy > dt_parse_ok_dmy) chosen_order = DateOrder::MDY;
        // else tie -> default MDY (US-style) unless data proves otherwise.

        if ((all_int && !any_missing) || all_float) {
            // Numeric Series
            if (all_int && !any_missing) {
                std::vector<int32_t> int_vals;
                int_vals.reserve(values.size());
                for (const auto& v : values) {
                    int64_t parsed = 0;
                    if (!is_missing_value(v) && try_parse_int64(v, parsed)) int_vals.push_back(static_cast<int32_t>(parsed));
                    else int_vals.push_back(0);
                }
                SeriesDataContainer container_int(int_vals);
                at::Tensor t_int = container_int.convert_to_tensor(at::TensorOptions().dtype(at::kInt));
                pluss::Series s_int(t_int);
                df = df->set_col(col, pluss::table::Thread(std::move(s_int), col));
            } else {
                std::vector<float> float_vals;
                float_vals.reserve(values.size());
                for (const auto& v : values) {
                    if (is_missing_value(v)) {
                        float_vals.push_back(NAN);
                        continue;
                    }
                    double parsed = 0.0;
                    if (try_parse_double(v, parsed)) float_vals.push_back(static_cast<float>(parsed));
                    else float_vals.push_back(NAN);
                }
                SeriesDataContainer container_flt(float_vals);
                at::Tensor t_flt = container_flt.convert_to_tensor(at::TensorOptions().dtype(at::kFloat));
                pluss::Series s_flt(t_flt);
                df = df->set_col(col, pluss::table::Thread(std::move(s_flt), col));
            }
        } else if (is_datetime_col && (any_date_component || any_time_component)) {
            // Timeline column: parse each cell into (date_int, time_int) series.
            std::vector<int64_t> date_vals;
            std::vector<int64_t> time_vals;
            date_vals.reserve(values.size());
            time_vals.reserve(values.size());
            for (const auto& v : values) {
                if (is_missing_value(v)) {
                    date_vals.push_back(0);
                    time_vals.push_back(0);
                    continue;
                }
                int64_t d = 0, tm = 0;
                bool hd = false, ht = false;
                if (parse_date_time_basic(v, d, tm, hd, ht, chosen_order)) {
                    date_vals.push_back(hd ? d : 0);
                    time_vals.push_back(ht ? tm : 0);
                } else {
                    date_vals.push_back(0);
                    time_vals.push_back(0);
                }
            }
            at::Tensor date_t = torch::tensor(date_vals, torch::dtype(torch::kInt64));
            pluss::Series ds(date_t);

            pluss::Series ts;
            if (any_time_component) {
                at::Tensor time_t = torch::tensor(time_vals, torch::dtype(torch::kInt64));
                ts = pluss::Series(time_t);
            }
            pluss::Timeline tl = pluss::TimelineFactory::from_series_pair(ds, ts);
            df = df->set_col(col, pluss::table::Thread(std::move(tl), col));
        } else {
            // Text column -> Note vector
            std::vector<pluss::Chord> chords;
            chords.reserve(values.size());
            for (const auto& v : values) {
                if (is_missing_value(v)) chords.emplace_back(std::string(""));
                else chords.emplace_back(v);
            }
            pluss::Note note = pluss::make_note(std::move(chords));
            df = df->set_col(col, pluss::table::Thread(std::move(note), col));
        }
    }

    return df;
}

} // namespace pluss::table::io
