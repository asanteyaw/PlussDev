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
  // A filter must name its key and its feature. Either empty name is refused.
  bi::Session session;
  std::cout << "empty key rejected: " << throws([&] {
    session.set_filter(bi::Filter{.key = "", .feature = "region"});
  }) << '\n';
  std::cout << "empty feature rejected: " << throws([&] {
    session.set_filter(bi::Filter{.key = "region", .feature = ""});
  }) << '\n';
  std::cout << "session still empty: " << session.empty() << '\n';
  std::cout << "version unchanged by rejected calls: " << (session.version() == 0) << '\n';
  return 0;
}
