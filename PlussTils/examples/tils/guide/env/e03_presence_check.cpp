// A deployment check needs to know whether a secret is configured, without ever printing it. The
// optional form of get answers presence, and the value is read only where the program needs it.
#include <pluss/tils/env.h>

#include <cstdlib>
#include <iostream>

namespace env = pluss::tils::env;

int main() {
  setenv("PLUSS_GUIDE_API_KEY", "not-printed-anywhere", 1);
  unsetenv("PLUSS_GUIDE_SMTP_PASSWORD");

  std::cout << std::boolalpha;
  const auto api_key = env::get("PLUSS_GUIDE_API_KEY");
  const auto smtp_password = env::get("PLUSS_GUIDE_SMTP_PASSWORD");
  std::cout << "api key configured: " << api_key.has_value() << '\n';
  std::cout << "smtp password configured: " << smtp_password.has_value() << '\n';

  // An empty name is never a variable, so the check reports it as absent.
  std::cout << "empty name configured: " << env::get("").has_value() << '\n';
  return 0;
}
