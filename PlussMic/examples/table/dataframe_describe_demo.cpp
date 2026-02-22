#include <chrono>
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <numeric>
#include <string>
#include <thread>
#include <vector>

#include <pluss/pluss.h>

namespace {

std::filesystem::path data_file(const std::string& name) {
  return std::filesystem::path(PLUSSMIC_SOURCE_DIR) / "data" / name;
}

struct TimingSummary {
  double min_ms{0.0};
  double max_ms{0.0};
  double avg_ms{0.0};
};

template <typename LoaderFn>
TimingSummary benchmark_csv_load(std::string_view label, std::size_t rounds, LoaderFn&& loader) {
  using Clock = std::chrono::steady_clock;
  std::vector<double> samples_ms;
  samples_ms.reserve(rounds);

  std::int64_t ncases = 0;
  std::int64_t nfeatures = 0;

  for (std::size_t i = 0; i < rounds; ++i) {
    std::cout << "running " << label << " round " << (i + 1) << "/" << rounds << " ...\n";
    const auto t0 = Clock::now();
    auto df = loader();
    const auto t1 = Clock::now();
    const auto elapsed_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    samples_ms.push_back(elapsed_ms);
    std::cout << "  round " << (i + 1) << ": " << elapsed_ms << " ms\n";
    ncases = df.nrows();
    nfeatures = df.nfeatures();
  }

  const auto [min_it, max_it] = std::minmax_element(samples_ms.begin(), samples_ms.end());
  const double total = std::accumulate(samples_ms.begin(), samples_ms.end(), 0.0);
  const double avg = total / static_cast<double>(samples_ms.size());

  std::cout << std::string(72, '=') << "\n";
  std::cout << "CSV benchmark (" << label << ")\n";
  std::cout << "rounds: " << rounds << "\n";
  std::cout << "shape : [" << ncases << " cs x " << nfeatures << " feat]\n";
  std::cout << "min   : " << *min_it << " ms\n";
  std::cout << "avg   : " << avg << " ms\n";
  std::cout << "max   : " << *max_it << " ms\n";

  return TimingSummary{*min_it, *max_it, avg};
}

} // namespace

int main() {
  auto df = pluss::table::DataFrame::load_csv(
      data_file("messy_data.csv").string(),
      {
          {"nrows", 80},
          {"columns",
           std::vector<std::string>{
               "Business Case ID",
               "Agency Name",
               "Lifecycle Cost",
               "Schedule Variance (in days)",
               "Cost Variance (%)",
           }},
      });

  std::cout << "=== describe demo ===\n";
  std::cout << "input\n";
  df.head(8);

  std::cout << "\nsummary statistics\n";
  df.describe();
  

  const auto us_csv = data_file("US_Accidents_Dec20.csv").string();
  std::cout << "\n=== CSV loader benchmark on US_Accidents_Dec20.csv ===\n";
  std::cout << "file: " << us_csv << "\n";
  std::cout << "hardware_concurrency: " << std::thread::hardware_concurrency() << "\n";

  constexpr std::size_t rounds = 3;

  pluss::loader::ReadCsvOptions serial_opts;
  serial_opts.engine = pluss::loader::CsvEngine::Serial;
  serial_opts.parallel_min_bytes = 0;
  serial_opts.profile = true;

  pluss::loader::ReadCsvOptions parallel_opts;
  parallel_opts.engine = pluss::loader::CsvEngine::Parallel;
  parallel_opts.parallel_min_bytes = 0;
  parallel_opts.profile = true;
  if (const auto hw = std::thread::hardware_concurrency(); hw > 0) {
    parallel_opts.parallel_threads = static_cast<std::size_t>(hw);
  }

  const auto serial_stats = benchmark_csv_load("serial engine", rounds, [&]() {
    return pluss::table::DataFrame::load_csv(us_csv, serial_opts);
  });
  const auto parallel_stats = benchmark_csv_load("parallel engine", rounds, [&]() {
    return pluss::table::DataFrame::load_csv(us_csv, parallel_opts);
  });

  const double speedup =
      parallel_stats.avg_ms > 0.0 ? serial_stats.avg_ms / parallel_stats.avg_ms : 0.0;
  std::cout << std::string(72, '-') << "\n";
  std::cout << "speedup (serial_avg / parallel_avg): " << speedup << "x\n";
  std::cout << std::string(72, '=') << "\n";

  return 0;
}
