#include <pluss/tils/aux/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  std::cout << std::boolalpha;
  std::cout << "warn is at least info: " << (OutputLevel::Warn > OutputLevel::Info) << '\n';
  std::cout << "debug is at least error: " << (OutputLevel::Debug >= OutputLevel::Error) << '\n';
  return 0;
}
