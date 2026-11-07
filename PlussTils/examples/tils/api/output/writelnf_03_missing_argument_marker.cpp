#include <pluss/tils/auxiliary/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.writelnf("{} and {}", "only one", PrintOptions{.missing_arg = "<?>"});
  return 0;
}
