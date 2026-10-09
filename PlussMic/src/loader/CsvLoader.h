#pragma once

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "pluss/coden/coden.h"
#include "table/Slate.h"

namespace pluss::table {
class DataFrame;
}

namespace pluss::loader {

struct ReadCsvOptions;

class CsvCaseReader {
public:
  CsvCaseReader(std::string_view filepath,
                std::size_t case_batch_size);
  CsvCaseReader(std::string_view filepath,
                std::size_t case_batch_size,
                const ReadCsvOptions& options);
  CsvCaseReader(CsvCaseReader&&) noexcept;
  CsvCaseReader& operator=(CsvCaseReader&&) noexcept;
  ~CsvCaseReader();

  CsvCaseReader(const CsvCaseReader&) = delete;
  CsvCaseReader& operator=(const CsvCaseReader&) = delete;

  bool next(pluss::table::DataFrame& output);
  std::size_t cases_read() const noexcept;

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

enum class BadLinePolicy : std::uint8_t {
  Error = 0,
  Warn = 1,
  Skip = 2,
};

enum class CsvEngine : std::uint8_t {
  Auto = 0,
  Parallel = 1,
  Serial = 2,
};

struct CsvSeparator {
  char value{','};
  bool explicit_set{false};

  constexpr CsvSeparator() = default;
  explicit constexpr CsvSeparator(char v, bool is_explicit = true)
      : value(v), explicit_set(is_explicit) {}

  constexpr CsvSeparator& operator=(char v) {
    value = v;
    explicit_set = true;
    return *this;
  }

  constexpr operator char() const { return value; }
};

// CSV parsing policy. Options that cannot be honored must fail explicitly.
struct ReadCsvOptions {
  std::string filepath;
  bool auto_detect_sep{true};
  CsvEngine engine{CsvEngine::Auto};
  std::optional<std::size_t> parallel_threads{};
  std::size_t parallel_min_bytes{4 * 1024 * 1024};
  // Auto policy: prefer Serial (Note-backed) ingest only for very large files.
  // Tuned to bias Auto toward Parallel on large-but-common workloads.
  std::size_t auto_serial_min_bytes{2ULL * 1024ULL * 1024ULL * 1024ULL};
  bool profile{false};

  // Parsing structure
  CsvSeparator sep{};
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
  std::vector<std::string> usefeats{};
  std::vector<std::size_t> usefeats_idx{};
  std::optional<std::string> case_col{};
  std::optional<std::size_t> case_col_idx{};

  // Case filtering / limits
  std::unordered_set<std::size_t> skipcases{};
  std::size_t skipcases_first_n{0};
  std::optional<std::size_t> ncases{};
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
  std::string null_rep{};
  std::optional<std::string> float_format{};
  char quotechar{'"'};
  bool doublequote{true};
  std::string line_terminator{"\n"};
  char decimal{'.'};
  std::string encoding{"utf-8"};
};

struct CsvRawColumn {
  std::string name{};
  std::size_t source_index{0};
  std::vector<std::string> values{};
  std::optional<pluss::coden::Note> note_values{};
};

struct CsvRawBatch {
  std::vector<CsvRawColumn> features{};
  std::optional<CsvRawColumn> cases{};
};

// Neutral CSV ingest payload (no DataFrame ownership).
struct CsvBatch {
  std::vector<std::pair<std::string, pluss::table::Slate>> features{};
  std::optional<pluss::table::Slate> cases{};
};

CsvRawBatch read_csv_records_raw_batch(const std::vector<std::vector<std::string>>& records,
                                       const ReadCsvOptions& options = {});
CsvRawBatch read_csv_raw_batch(const ReadCsvOptions& options);
CsvRawBatch read_csv_raw_batch(std::string_view filepath, const ReadCsvOptions& options = {});

CsvBatch read_csv_batch(const ReadCsvOptions& options);
CsvBatch read_csv_batch(std::string_view filepath, const ReadCsvOptions& options = {});
CsvBatch read_csv_records_batch(const std::vector<std::vector<std::string>>& records,
                                const ReadCsvOptions& options = {});

pluss::table::DataFrame load_csv(const ReadCsvOptions& options);
pluss::table::DataFrame load_csv(std::string_view filepath, const ReadCsvOptions& options = {});
pluss::table::DataFrame load_csv(std::string_view filepath,
                                 char sep,
                                 std::optional<std::size_t> ncases = std::nullopt,
                                 std::vector<std::string> usefeats = {},
                                 std::optional<std::string> case_col = std::nullopt,
                                 BadLinePolicy on_bad_lines = BadLinePolicy::Error);
pluss::table::DataFrame load_csv(std::string_view filepath,
                                 char sep,
                                 std::optional<std::size_t> ncases,
                                 std::initializer_list<std::string_view> usefeats,
                                 std::optional<std::string> case_col = std::nullopt,
                                 BadLinePolicy on_bad_lines = BadLinePolicy::Error);
pluss::table::DataFrame load_csv_records(const std::vector<std::vector<std::string>>& records,
                                         const ReadCsvOptions& options = {});
void save_csv(const pluss::table::DataFrame& frame,
              std::string_view filepath,
              const WriteCsvOptions& options = {});

} // namespace pluss::loader
