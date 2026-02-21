#pragma once

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace pluss::table {
class DataFrame;
}

namespace pluss::loader {

enum class BadLinePolicy : std::uint8_t {
  Error = 0,
  Warn = 1,
  Skip = 2,
};

enum class CsvEngine : std::uint8_t {
  Auto = 0,
  Serial = 1,
  Parallel = 2,
};

// Broad pandas-inspired option surface. Some options are hints/placeholders for
// planned extensions (Excel/chunked readers) and may currently be no-ops.
struct ReadCsvOptions {
  std::string filepath;
  bool auto_detect_sep{true};
  CsvEngine engine{CsvEngine::Auto};
  std::optional<std::size_t> parallel_threads{};
  std::size_t parallel_min_bytes{4 * 1024 * 1024};
  bool profile{false};

  // Parsing structure
  char sep{','};
  std::optional<char> delimiter{};
  std::optional<char> quotechar{'"'};
  bool doublequote{true};
  std::optional<char> escapechar{};
  std::optional<char> comment{};
  bool skipinitialspace{false};
  bool skip_blank_lines{true};
  std::string lineterminator{"\n"};

  // Header and labels
  bool header{true};
  std::int64_t header_row{0};
  std::vector<std::string> names{};
  std::vector<std::string> usecols{};
  std::vector<std::size_t> usecols_idx{};
  std::optional<std::string> case_col{};
  std::optional<std::size_t> case_col_idx{};

  // Row filtering / limits
  std::unordered_set<std::size_t> skiprows{};
  std::size_t skiprows_first_n{0};
  std::optional<std::size_t> nrows{};
  std::size_t skipfooter{0};

  // Type and null handling
  std::unordered_map<std::string, std::string> dtype{};
  std::vector<std::string> parse_dates{};
  std::vector<std::size_t> parse_dates_idx{};
  bool infer_datetime_format{true};
  bool dayfirst{false};

  bool na_filter{true};
  bool keep_default_na{true};
  std::unordered_set<std::string> na_values{};
  std::unordered_set<std::string> true_values{"true", "True", "1", "yes", "YES"};
  std::unordered_set<std::string> false_values{"false", "False", "0", "no", "NO"};

  std::optional<char> thousands{};
  char decimal{'.'};

  // Engine/error behavior
  BadLinePolicy on_bad_lines{BadLinePolicy::Error};
  std::string encoding{"utf-8"};
  bool low_memory{true};
  bool memory_map{false};
  std::optional<std::size_t> chunksize{};
};

struct WriteCsvOptions {
  char sep{','};
  bool header{true};
  bool add_cases{false};
  bool cases{false}; // legacy alias
  std::optional<std::string> case_label{};
  std::string na_rep{"NA"};
  std::optional<std::string> float_format{};
  char quotechar{'"'};
  bool doublequote{true};
  std::string line_terminator{"\n"};
  char decimal{'.'};
  std::string encoding{"utf-8"};
};

pluss::table::DataFrame load_csv(const ReadCsvOptions& options);
pluss::table::DataFrame load_csv(std::string_view filepath, const ReadCsvOptions& options = {});
pluss::table::DataFrame load_csv(std::string_view filepath,
                                 char sep,
                                 std::optional<std::size_t> nrows = std::nullopt,
                                 std::vector<std::string> usecols = {},
                                 std::optional<std::string> case_col = std::nullopt,
                                 BadLinePolicy on_bad_lines = BadLinePolicy::Error);
pluss::table::DataFrame load_csv(std::string_view filepath,
                                 char sep,
                                 std::optional<std::size_t> nrows,
                                 std::initializer_list<std::string_view> usecols,
                                 std::optional<std::string> case_col = std::nullopt,
                                 BadLinePolicy on_bad_lines = BadLinePolicy::Error);
pluss::table::DataFrame load_csv_records(const std::vector<std::vector<std::string>>& records,
                                         const ReadCsvOptions& options = {});
void save_csv(const pluss::table::DataFrame& frame,
              std::string_view filepath,
              const WriteCsvOptions& options = {});

} // namespace pluss::loader
