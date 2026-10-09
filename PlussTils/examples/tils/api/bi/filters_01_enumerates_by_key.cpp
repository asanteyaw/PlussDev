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
  // filters returns the whole map. Keys come out in sorted order.
  bi::Session session;
  session.set_filter(region_filter("zeta", "north"));
  session.set_filter(region_filter("alpha", "south"));
  for (const auto& [key, filter] : session.filters()) {
    std::cout << key << ": " << filter.feature << '\n';
  }
  return 0;
}
