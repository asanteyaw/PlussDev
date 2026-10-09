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
  // A filter is stored under its key. Setting the same key again replaces the earlier filter.
  session.set_filter(bi::Filter{.key = "region", .feature = "region", .op = bi::FilterOp::Eq,
                                .payload = bi::FilterScalar{std::string("north")}});
  session.set_filter(bi::Filter{.key = "region", .feature = "region", .op = bi::FilterOp::Eq,
                                .payload = bi::FilterScalar{std::string("south")}});
  std::cout << "filters after two sets with one key: " << session.filters().size() << '\n';
  std::cout << "has region: " << session.has_filter("region") << '\n';
  std::cout << "version after two sets: " << session.version() << '\n';
  return 0;
}
