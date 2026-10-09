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
  // clear_filter removes the filter stored under one key and leaves other filters alone.
  bi::Session session;
  session.set_filter(region_filter("region", "north"));
  session.set_filter(region_filter("region_b", "south"));
  session.clear_filter("region");
  std::cout << "region still stored: " << session.has_filter("region") << '\n';
  std::cout << "region_b still stored: " << session.has_filter("region_b") << '\n';
  std::cout << "filters left: " << session.filters().size() << '\n';
  return 0;
}
