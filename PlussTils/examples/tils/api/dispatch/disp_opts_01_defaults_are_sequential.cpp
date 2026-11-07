#include <pluss/tils/auxiliary/transmit.h>

#include <iostream>

using namespace pluss::tils;

int main() {
  std::cout << std::boolalpha;
  const transmit_options defaults{};
  std::cout << "default exec is seq: " << (defaults.exec == transmit_exec::seq) << '\n';
  std::cout << "default grain size: " << defaults.grain_size << '\n';
  std::cout << "default workers (0 means hardware_concurrency): " << defaults.workers << '\n';
  return 0;
}
