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

bi::Filter region_filter(const std::string& key, const std::string& value) {
  return bi::Filter{.key = key, .feature = "region", .op = bi::FilterOp::Eq,
                    .payload = bi::FilterScalar{value}};
}

int main() {
  std::cout << std::boolalpha;
  // has_filter reports whether a filter is stored under a key.
  bi::Session session;
  std::cout << "before any filter: " << session.has_filter("region") << '\n';
  session.set_filter(region_filter("region", "north"));
  std::cout << "after storing: " << session.has_filter("region") << '\n';
  std::cout << "other key: " << session.has_filter("other") << '\n';
  return 0;
}
