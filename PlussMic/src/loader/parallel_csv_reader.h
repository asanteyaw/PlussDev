#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace pluss::loader {

struct ReadCsvOptions;

// Internal high-throughput CSV parser used by CsvLoader when engine=Parallel or Auto.
class ParallelCsvReader final {
public:
  static bool should_use_parallel(std::size_t file_bytes, const ReadCsvOptions& options);
  static std::vector<std::vector<std::string>> read_file(const ReadCsvOptions& options);
};

} // namespace pluss::loader

