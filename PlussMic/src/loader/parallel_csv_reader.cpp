#include "loader/parallel_csv_reader.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <future>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#include "CodeM/core/Errors.h"
#include "loader/CsvLoader.h"

namespace pluss::loader {

namespace {

struct RowSpan {
  std::size_t begin{0};
  std::size_t end{0}; // exclusive
};

std::size_t resolve_threads(const ReadCsvOptions& options, std::size_t work_items) {
  if (work_items == 0) {
    return 1;
  }
  if (options.parallel_threads.has_value() && *options.parallel_threads > 0) {
    return std::max<std::size_t>(1, std::min<std::size_t>(*options.parallel_threads, work_items));
  }
  const auto hw = std::thread::hardware_concurrency();
  const auto auto_threads = hw > 0 ? static_cast<std::size_t>(hw) : static_cast<std::size_t>(4);
  return std::max<std::size_t>(1, std::min<std::size_t>(auto_threads, work_items));
}

std::string read_entire_file(std::string_view filepath) {
  std::ifstream in(std::string(filepath), std::ios::in | std::ios::binary);
  PLUSS_CHECK(in.is_open(), "parallel_csv_reader: failed to open file '" + std::string(filepath) + "'");
  in.seekg(0, std::ios::end);
  const auto n = static_cast<std::size_t>(in.tellg());
  in.seekg(0, std::ios::beg);
  std::string out(n, '\0');
  if (n > 0) {
    in.read(out.data(), static_cast<std::streamsize>(n));
  }
  return out;
}

std::vector<RowSpan> build_row_spans(std::string_view text, const ReadCsvOptions& options) {
  std::vector<RowSpan> spans;
  spans.reserve(text.size() / 64 + 1);

  const bool has_quote = options.quotechar.has_value();
  const char quote = has_quote ? *options.quotechar : '\0';
  const bool has_escape = options.escapechar.has_value();
  const char escape = has_escape ? *options.escapechar : '\0';

  bool in_quotes = false;
  std::size_t row_begin = 0;

  for (std::size_t i = 0; i < text.size(); ++i) {
    const char c = text[i];
    if (in_quotes) {
      if (has_escape && c == escape) {
        if (i + 1 < text.size()) {
          ++i;
        }
        continue;
      }
      if (has_quote && c == quote) {
        if (options.doublequote && i + 1 < text.size() && text[i + 1] == quote) {
          ++i;
        } else {
          in_quotes = false;
        }
      }
      continue;
    }

    if (has_quote && c == quote) {
      in_quotes = true;
      continue;
    }

    if (c == '\n') {
      const auto row_end = (i > row_begin && text[i - 1] == '\r') ? (i - 1) : i;
      spans.push_back(RowSpan{row_begin, row_end});
      row_begin = i + 1;
    }
  }

  if (in_quotes) {
    PLUSS_FAIL("parallel_csv_reader: unterminated quoted field at EOF");
  }

  if (row_begin < text.size()) {
    const auto row_end = (text.back() == '\r') ? (text.size() - 1) : text.size();
    spans.push_back(RowSpan{row_begin, row_end});
  }

  return spans;
}

std::optional<std::vector<std::string>> parse_row(std::string_view row, const ReadCsvOptions& options) {
  if (row.empty()) {
    if (options.skip_blank_lines) {
      return std::nullopt;
    }
    return std::vector<std::string>{""};
  }

  if (options.comment.has_value() && row.front() == *options.comment) {
    if (options.skip_blank_lines) {
      return std::nullopt;
    }
    return std::vector<std::string>{""};
  }

  const char sep = options.delimiter.has_value() ? *options.delimiter : options.sep;
  const bool has_quote = options.quotechar.has_value();
  const char quote = has_quote ? *options.quotechar : '\0';
  const bool has_escape = options.escapechar.has_value();
  const char escape = has_escape ? *options.escapechar : '\0';

  std::vector<std::string> out;
  out.reserve(16);

  std::string field;
  field.reserve(32);

  bool in_quotes = false;
  for (std::size_t i = 0; i < row.size(); ++i) {
    const char c = row[i];
    if (!in_quotes) {
      if (has_quote && c == quote) {
        in_quotes = true;
        continue;
      }
      if (c == sep) {
        out.push_back(std::move(field));
        field.clear();
        continue;
      }
      field.push_back(c);
    } else {
      if (has_escape && c == escape) {
        if (i + 1 < row.size()) {
          field.push_back(row[i + 1]);
          ++i;
          continue;
        }
        PLUSS_FAIL("parallel_csv_reader: dangling escape character in quoted field");
      }
      if (has_quote && c == quote) {
        if (options.doublequote && i + 1 < row.size() && row[i + 1] == quote) {
          field.push_back(quote);
          ++i;
          continue;
        }
        in_quotes = false;
        continue;
      }
      field.push_back(c);
    }
  }

  PLUSS_CHECK(!in_quotes, "parallel_csv_reader: unterminated quoted field in row");
  out.push_back(std::move(field));
  return out;
}

std::vector<std::vector<std::string>> parse_parallel(std::string_view text, const ReadCsvOptions& options) {
  const auto spans = build_row_spans(text, options);
  if (spans.empty()) {
    return {};
  }

  const auto n_threads = resolve_threads(options, spans.size());
  if (n_threads <= 1 || spans.size() < 1024) {
    std::vector<std::vector<std::string>> rows;
    rows.reserve(spans.size());
    for (const auto& span : spans) {
      const auto maybe = parse_row(text.substr(span.begin, span.end - span.begin), options);
      if (maybe.has_value()) {
        rows.push_back(*maybe);
      }
    }
    return rows;
  }

  std::vector<std::future<std::vector<std::vector<std::string>>>> tasks;
  tasks.reserve(n_threads);

  const auto chunk = (spans.size() + n_threads - 1) / n_threads;
  for (std::size_t t = 0; t < n_threads; ++t) {
    const auto b = t * chunk;
    const auto e = std::min<std::size_t>(spans.size(), b + chunk);
    if (b >= e) {
      break;
    }

    tasks.push_back(std::async(std::launch::async, [b, e, &spans, text, &options]() {
      std::vector<std::vector<std::string>> local;
      local.reserve(e - b);
      for (std::size_t i = b; i < e; ++i) {
        const auto& span = spans[i];
        const auto maybe = parse_row(text.substr(span.begin, span.end - span.begin), options);
        if (maybe.has_value()) {
          local.push_back(*maybe);
        }
      }
      return local;
    }));
  }

  std::vector<std::vector<std::string>> out;
  for (auto& task : tasks) {
    auto local = task.get();
    out.insert(out.end(),
               std::make_move_iterator(local.begin()),
               std::make_move_iterator(local.end()));
  }
  return out;
}

} // namespace

bool ParallelCsvReader::should_use_parallel(std::size_t file_bytes, const ReadCsvOptions& options) {
  if (options.engine == CsvEngine::Serial) {
    return false;
  }
  if (options.engine == CsvEngine::Parallel) {
    return true;
  }
  return file_bytes >= options.parallel_min_bytes;
}

std::vector<std::vector<std::string>> ParallelCsvReader::read_file(const ReadCsvOptions& options) {
  PLUSS_CHECK(!options.filepath.empty(), "parallel_csv_reader: filepath is empty");
  const auto text = read_entire_file(options.filepath);
  if (text.empty()) {
    return {};
  }
  return parse_parallel(text, options);
}

} // namespace pluss::loader
