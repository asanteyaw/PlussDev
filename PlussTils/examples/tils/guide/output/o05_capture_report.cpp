// Capturing output into a string instead of printing it. This is how a test checks what a report
// would say, or how a program saves a summary to a file it writes itself.
#include <pluss/tils/kit/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace pluss::tils;

int main() {
  std::ostringstream captured;
  OutputEngine out{captured};
  const std::vector<double> latencies{12.5, 48.0, 7.25};

  out.writeln("latencies (ms):", latencies, PrintOptions{}.fixed(1));
  out.writelnf("count={} slowest={}", latencies.size(), 48.0);

  const std::string report = captured.str();
  std::cout << "captured " << report.size() << " characters\n";
  std::cout << "captured text:\n" << report;
  return 0;
}
