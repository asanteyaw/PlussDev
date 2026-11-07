// Sizes and durations for a storage report. Raw counts are hard to read, so the output options
// turn them into IEC bytes, SI bytes, metric counts, and durations.
#include <pluss/tils/auxiliary/output.h>

#include <iostream>

using namespace pluss::tils;

int main() {
  OutputEngine out{std::cout};
  out.writeln("cache size:", 1536, PrintOptions{.humanize = Humanize::BytesIEC});
  out.writeln("disk size:", 3221225472LL, PrintOptions{.humanize = Humanize::BytesIEC});
  out.writeln("upload size (SI):", 2500000LL, PrintOptions{.humanize = Humanize::BytesSI});
  out.writeln("rows processed:", 6700000LL, PrintOptions{.humanize = Humanize::Metric});
  out.writeln("job took:", 90000000000LL, PrintOptions{.humanize = Humanize::DurationNs});
  out.writeln("cache size, two decimals:", 1536,
              PrintOptions{.humanize = Humanize::BytesIEC, .humanize_precision = 2});
  return 0;
}
