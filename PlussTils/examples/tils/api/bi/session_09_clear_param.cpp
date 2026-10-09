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
  bi::Session session;
  session.set_param("threshold", 2.5);
  session.clear_param("threshold");
  std::cout << "still set: " << session.has_param("threshold") << '\n';
  std::cout << "version after clear: " << session.version() << '\n';
  // Clearing a key that was never set still counts as a change to the session.
  session.clear_param("never_set");
  std::cout << "version after clearing an absent key: " << session.version() << '\n';
  return 0;
}
