#include <pluss/pluss.h>
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <stdexcept>
#include <string>
#include <cctype>

namespace pluss::table::io {

// Trim whitespace from both ends (in-place view)
static inline std::string trim(const std::string& s) {
    size_t l = 0;
    size_t r = s.size();
    while (l < r && std::isspace(static_cast<unsigned char>(s[l]))) ++l;
    while (r > l && std::isspace(static_cast<unsigned char>(s[r-1]))) --r;
    return s.substr(l, r - l);
}

// Parse a CSV line into fields. Supports quoted fields with double quotes
// and commas inside quoted fields. Handles CRLF endings.
// Parse a delimited line into fields. Supports quoted fields with double quotes
// and delimiters (default comma). Handles CRLF endings.
static std::vector<std::string> parse_csv_line(const std::string& line, char delimiter = ',') {
    std::vector<std::string> fields;
    std::string cur;
    bool in_quotes = false;
    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (in_quotes) {
            if (c == '"') {
                if (i + 1 < line.size() && line[i+1] == '"') {
                    // escaped quote
                    cur.push_back('"');
                    ++i;
                } else {
                    in_quotes = false;
                }
            } else {
                cur.push_back(c);
            }
        } else {
            if (c == '"') {
                in_quotes = true;
            } else if (c == delimiter) {
                fields.push_back(trim(cur));
                cur.clear();
            } else if (c == '\r') {
                // ignore CR (handle CRLF)
            } else {
                cur.push_back(c);
            }
        }
    }
    fields.push_back(trim(cur));
    return fields;
}

RawTable load_csv_raw(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + filename);
    }

    RawTable raw;
    std::string line;

    // Read header (first non-empty line). Attempt to detect delimiter (comma vs semicolon).
    char detected_delim = ',';
    while (std::getline(file, line)) {
        if (!line.empty()) {
            // quick heuristic: if the header contains semicolons but no commas, treat ';' as delimiter
            if (line.find(';') != std::string::npos && line.find(',') == std::string::npos) detected_delim = ';';
            auto cols = parse_csv_line(line, detected_delim);
            for (const auto& c : cols) {
                raw.columns.push_back(c);
                raw.data.emplace(c, std::vector<std::string>{});
            }
            break;
        }
    }

    const size_t num_cols = raw.columns.size();
    if (num_cols == 0) {
        // empty file or no header
        return raw;
    }

    // Read remaining rows
    while (std::getline(file, line)) {
        auto fields = parse_csv_line(line, detected_delim);
        // ensure we have num_cols fields; pad with empty strings if needed
        fields.resize(num_cols);
        for (size_t i = 0; i < num_cols; ++i) {
            raw.data[raw.columns[i]].push_back(fields[i]);
        }
    }

    file.close();
    return raw;
}

std::shared_ptr<DataFrame> load_csv(const std::string& filename) {
    RawTable raw = load_csv_raw(filename);
    return convert_raw_to_dataframe(raw);
}

}  // namespace pluss::table::io