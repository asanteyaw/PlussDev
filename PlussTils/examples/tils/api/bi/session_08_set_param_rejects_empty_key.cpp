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
  std::cout << "empty key rejected: " << throws([&] { session.set_param("", 1.0); }) << '\n';
  std::cout << "empty key with text rejected: " << throws([&] { session.set_param("", std::string_view("x")); }) << '\n';
  std::cout << "version unchanged by rejected calls: " << (session.version() == 0) << '\n';
  return 0;
}
