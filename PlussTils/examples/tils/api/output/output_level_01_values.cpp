#include <pluss/tils/aux/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  std::cout << "trace: " << static_cast<std::size_t>(OutputLevel::Trace) << '\n';
  std::cout << "debug: " << static_cast<std::size_t>(OutputLevel::Debug) << '\n';
  std::cout << "info: " << static_cast<std::size_t>(OutputLevel::Info) << '\n';
  std::cout << "warn: " << static_cast<std::size_t>(OutputLevel::Warn) << '\n';
  std::cout << "error: " << static_cast<std::size_t>(OutputLevel::Error) << '\n';
  return 0;
}
