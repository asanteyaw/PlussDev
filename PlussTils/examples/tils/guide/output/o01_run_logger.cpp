// A run logger for a batch job. It writes levelled lines to standard output, filters out debug
// lines below its minimum level, and shows how a custom prefix changes the look of one level.
#include <pluss/tils/aux/output.h>

#include <iostream>

using namespace pluss::tils;

int main() {
  OutputOptions options;
  options.min_level = OutputLevel::Info;  // trace and debug lines are dropped, not printed
  OutputEngine log{std::cout, options};

  log.info("batch started, jobs queued:", 4);
  log.debug("this debug line is below the minimum level, so it never prints");
  log.info("loading", "sales.csv");
  log.warn("row 17 has an empty region, using 'unknown'");
  log.prefix(OutputLevel::Error, "[FAIL]");  // only the error prefix changes
  log.error("job 3 failed after", 2, "retries");
  log.info("batch finished, failed jobs:", 1);
  return 0;
}
