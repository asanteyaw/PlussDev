// Writing results straight to an output iterator, so no intermediate vector is built. A report that
// may be very long can stream its lines to a file, a socket, or the terminal as it computes them.
#include <pluss/tils/kit/transmit.h>

#include <iostream>
#include <iterator>
#include <string>
#include <vector>

int main() {
  const std::vector<int> minutes{5, 42, 90, 181};

  auto out = pluss::tils::transmit_into(
      [](int m) { return std::to_string(m / 60) + "h " + std::to_string(m % 60) + "m"; },
      minutes,
      std::ostream_iterator<std::string>(std::cout, "\n"));
  (void)out;
  return 0;
}
