#include <pluss/tils/kit/output.h>

#include <array>
#include <iostream>
#include <limits>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  try {
    out.writelnf("{} {}", 1, 2, 3, PrintOptions{.strict_args = true});
    std::cout << "accepted\n";
  } catch (const std::exception&) {
    std::cout << "refused\n";
  }
  return 0;
}
