#include <algorithm>
#include <atomic>
#include <chrono>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include "pluss/pluss.h"

#if defined(__APPLE__)
#include <mach/mach.h>
#endif

namespace {

struct CliOptions {
  std::string filepath;
  pluss::loader::CsvEngine engine{pluss::loader::CsvEngine::Serial};
  std::size_t warmup_rounds{0};
  std::size_t timed_rounds{1};
  bool profile{false};
};

[[noreturn]] void die(std::string_view message) {
  throw std::runtime_error(std::string(message));
}

std::string lower_copy(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
    return static_cast<char>(std::tolower(ch));
  });
  return value;
}

pluss::loader::CsvEngine parse_engine(const std::string& value) {
  const auto key = lower_copy(value);
  if (key == "parallel") {
    return pluss::loader::CsvEngine::Parallel;
  }
  if (key == "serial") {
    return pluss::loader::CsvEngine::Serial;
  }
  if (key == "auto") {
    return pluss::loader::CsvEngine::Auto;
  }
  die("unknown engine (use parallel|serial|auto)");
}

std::string to_string(pluss::loader::CsvEngine engine) {
  switch (engine) {
    case pluss::loader::CsvEngine::Auto:
      return "auto";
    case pluss::loader::CsvEngine::Parallel:
      return "parallel";
    case pluss::loader::CsvEngine::Serial:
      return "serial";
  }
  return "unknown";
}

CliOptions parse_cli(int argc, char** argv) {
  CliOptions out;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--file") {
      if (i + 1 >= argc) {
        die("--file expects a value");
      }
      out.filepath = argv[++i];
      continue;
    }
    if (arg == "--engine") {
      if (i + 1 >= argc) {
        die("--engine expects a value");
      }
      out.engine = parse_engine(argv[++i]);
      continue;
    }
    if (arg == "--warmup") {
      if (i + 1 >= argc) {
        die("--warmup expects a value");
      }
      out.warmup_rounds = static_cast<std::size_t>(std::stoull(argv[++i]));
      continue;
    }
    if (arg == "--rounds") {
      if (i + 1 >= argc) {
        die("--rounds expects a value");
      }
      out.timed_rounds = static_cast<std::size_t>(std::stoull(argv[++i]));
      continue;
    }
    if (arg == "--profile") {
      if (i + 1 >= argc) {
        die("--profile expects 0|1");
      }
      out.profile = std::string_view(argv[++i]) == "1";
      continue;
    }
    if (arg == "--help" || arg == "-h") {
      std::cout << "Usage: plussmic_csv_engine_bench [--file <csv> (default: PlussMic/data/hour.csv)] [--engine serial] [--warmup 1] [--rounds 3] [--profile 0|1]\n";
      std::exit(0);
    }
    die("unknown argument: " + arg);
  }

  if (out.filepath.empty()) {
#ifdef PLUSSMIC_SOURCE_DIR
    out.filepath = (std::filesystem::path(PLUSSMIC_SOURCE_DIR) / "data" / "hour.csv").string();
    std::cout << "No --file given; benchmarking default " << out.filepath << "\n";
#else
    die("--file is required");
#endif
  }
  if (!std::filesystem::exists(out.filepath)) {
    die("file does not exist: " + out.filepath);
  }
  if (out.timed_rounds == 0) {
    die("--rounds must be >= 1");
  }
  return out;
}

struct TimingSummary {
  double min_ms{0.0};
  double max_ms{0.0};
  double avg_ms{0.0};
  double median_ms{0.0};
  double avg_peak_delta_mb{0.0};
  double max_peak_delta_mb{0.0};
  double avg_post_delta_mb{0.0};
  std::int64_t ncases{0};
  std::int64_t nfeatures{0};
};

std::uint64_t current_rss_bytes() {
#if defined(__APPLE__)
  mach_task_basic_info_data_t info;
  mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
  if (task_info(mach_task_self(),
                MACH_TASK_BASIC_INFO,
                reinterpret_cast<task_info_t>(&info),
                &count) == KERN_SUCCESS) {
    return static_cast<std::uint64_t>(info.resident_size);
  }
#endif
  return 0;
}

TimingSummary run_benchmark(const CliOptions& cli) {
  std::vector<double> samples_ms;
  samples_ms.reserve(cli.timed_rounds);
  std::vector<double> peak_delta_mb_samples;
  std::vector<double> post_delta_mb_samples;
  peak_delta_mb_samples.reserve(cli.timed_rounds);
  post_delta_mb_samples.reserve(cli.timed_rounds);

  pluss::loader::ReadCsvOptions options;
  options.filepath = cli.filepath;
  options.engine = cli.engine;
  options.parallel_min_bytes = 0;
  options.profile = cli.profile;

  for (std::size_t i = 0; i < cli.warmup_rounds; ++i) {
    auto df = pluss::table::DataFrame::load_csv(cli.filepath, options);
    std::cout << "warmup " << (i + 1) << "/" << cli.warmup_rounds
              << " shape=[" << df.ncases() << " x " << df.nfeatures() << "]\n";
  }

  std::int64_t ncases = 0;
  std::int64_t nfeatures = 0;
  using Clock = std::chrono::steady_clock;

  for (std::size_t i = 0; i < cli.timed_rounds; ++i) {
    const std::uint64_t rss_before = current_rss_bytes();
    std::atomic<bool> sampling{true};
    std::atomic<std::uint64_t> rss_peak{rss_before};
    std::thread sampler([&]() {
      while (sampling.load(std::memory_order_relaxed)) {
        const std::uint64_t cur = current_rss_bytes();
        std::uint64_t prev = rss_peak.load(std::memory_order_relaxed);
        while (cur > prev &&
               !rss_peak.compare_exchange_weak(prev, cur, std::memory_order_relaxed)) {
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
      }
    });

    const auto t0 = Clock::now();
    auto df = pluss::table::DataFrame::load_csv(cli.filepath, options);
    const auto t1 = Clock::now();
    sampling.store(false, std::memory_order_relaxed);
    sampler.join();

    const double elapsed_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    samples_ms.push_back(elapsed_ms);
    ncases = df.ncases();
    nfeatures = df.nfeatures();
    const std::uint64_t rss_after = current_rss_bytes();
    const std::uint64_t rss_peak_bytes = std::max(rss_peak.load(std::memory_order_relaxed), rss_after);

    constexpr double kMb = 1024.0 * 1024.0;
    const double peak_delta_mb = rss_peak_bytes >= rss_before
                                     ? static_cast<double>(rss_peak_bytes - rss_before) / kMb
                                     : 0.0;
    const double post_delta_mb = rss_after >= rss_before
                                     ? static_cast<double>(rss_after - rss_before) / kMb
                                     : 0.0;

    peak_delta_mb_samples.push_back(peak_delta_mb);
    post_delta_mb_samples.push_back(post_delta_mb);

    std::cout << "timed round " << (i + 1) << "/" << cli.timed_rounds
              << ": " << elapsed_ms << " ms"
              << " | peak_delta_mb=" << peak_delta_mb
              << " | post_delta_mb=" << post_delta_mb
              << "\n";
  }

  std::sort(samples_ms.begin(), samples_ms.end());
  const auto min_ms = samples_ms.front();
  const auto max_ms = samples_ms.back();
  const auto avg_ms = std::accumulate(samples_ms.begin(), samples_ms.end(), 0.0) /
                      static_cast<double>(samples_ms.size());
  const auto median_ms = samples_ms[samples_ms.size() / 2];
  const double avg_peak_delta_mb =
      std::accumulate(peak_delta_mb_samples.begin(), peak_delta_mb_samples.end(), 0.0) /
      static_cast<double>(peak_delta_mb_samples.size());
  const double max_peak_delta_mb = *std::max_element(peak_delta_mb_samples.begin(), peak_delta_mb_samples.end());
  const double avg_post_delta_mb =
      std::accumulate(post_delta_mb_samples.begin(), post_delta_mb_samples.end(), 0.0) /
      static_cast<double>(post_delta_mb_samples.size());

  return TimingSummary{
      min_ms,
      max_ms,
      avg_ms,
      median_ms,
      avg_peak_delta_mb,
      max_peak_delta_mb,
      avg_post_delta_mb,
      ncases,
      nfeatures,
  };
}

} // namespace

int main(int argc, char** argv) {
  try {
    const auto cli = parse_cli(argc, argv);
    const auto stats = run_benchmark(cli);

    std::cout << "============================================================\n";
    std::cout << "csv_engine_bench\n";
    std::cout << "file        : " << cli.filepath << "\n";
    std::cout << "engine      : " << to_string(cli.engine) << "\n";
    std::cout << "warmup      : " << cli.warmup_rounds << "\n";
    std::cout << "rounds      : " << cli.timed_rounds << "\n";
    std::cout << "shape       : [" << stats.ncases << " cs x " << stats.nfeatures << " feat]\n";
    std::cout << "min_ms      : " << stats.min_ms << "\n";
    std::cout << "median_ms   : " << stats.median_ms << "\n";
    std::cout << "avg_ms      : " << stats.avg_ms << "\n";
    std::cout << "max_ms      : " << stats.max_ms << "\n";
    std::cout << "avg_peak_delta_mb: " << stats.avg_peak_delta_mb << "\n";
    std::cout << "max_peak_delta_mb: " << stats.max_peak_delta_mb << "\n";
    std::cout << "avg_post_delta_mb: " << stats.avg_post_delta_mb << "\n";
    std::cout << "============================================================\n";
  } catch (const std::exception& e) {
    std::cerr << "error: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
