// A service reads its settings from the environment at startup. A variable that is not set gets a
// sensible default, so the service runs the same way on a laptop and in production.
// This program sets two of the variables itself, so the output is the same on every machine.
#include <pluss/tils/env.h>

#include <cstdlib>
#include <iostream>

namespace env = pluss::tils::env;

int main() {
  setenv("PLUSS_GUIDE_REGION", "eu-west-2", 1);
  setenv("PLUSS_GUIDE_DEBUG", "on", 1);
  unsetenv("PLUSS_GUIDE_PORT");

  const std::string region = env::get_or("PLUSS_GUIDE_REGION", "us-east-1");
  const int port = env::get_int("PLUSS_GUIDE_PORT", 8080);
  const bool debug = env::get_bool("PLUSS_GUIDE_DEBUG", false);

  std::cout << "region: " << region << '\n';
  std::cout << "port: " << port << '\n';
  std::cout << "debug: " << std::boolalpha << debug << '\n';
  return 0;
}
