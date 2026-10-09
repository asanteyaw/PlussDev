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
  // A new session has no filters and no parameters, and it starts at version zero.
  bi::Session session;
  std::cout << "empty: " << session.empty() << '\n';
  std::cout << "version: " << session.version() << '\n';
  std::cout << "filters: " << session.filters().size() << ", params: " << session.params().size() << '\n';
  return 0;
}
