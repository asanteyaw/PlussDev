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
  // Filters and parameters live in separate maps. Each change to either bumps the one version.
  bi::Session session;
  session.set_param("threshold", 2.5);
  session.set_filter(bi::Filter{
      .key = "region_is_north",
      .feature = "region",
      .op = bi::FilterOp::Eq,
      .payload = bi::FilterScalar{std::string("north")},
  });
  std::cout << "params: " << session.params().size() << ", filters: " << session.filters().size() << '\n';
  std::cout << "empty after two changes: " << session.empty() << '\n';
  std::cout << "version: " << session.version() << '\n';
  return 0;
}
