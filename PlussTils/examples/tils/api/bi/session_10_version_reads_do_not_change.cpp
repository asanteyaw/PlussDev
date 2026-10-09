#include <pluss/bi/session.h>
#include <pluss/bi/specs.h>

#include <iostream>
#include <string>
#include <variant>

namespace bi = pluss::bi;

template <class Fn>
bool throws(Fn fn) {
  try {
    fn();
  } catch (const std::exception&) {
    return true;
  }
  return false;
}

int main() {
  std::cout << std::boolalpha;
  // Reading the session, its maps, or its version never changes the version.
  bi::Session session;
  session.set_param("threshold", 2.5);
  const auto before = session.version();
  (void)session.param("threshold");
  (void)session.has_param("threshold");
  (void)session.params();
  (void)session.filters();
  std::cout << "version unchanged by reads: " << (session.version() == before) << '\n';
  std::cout << "version: " << session.version() << '\n';
  return 0;
}
