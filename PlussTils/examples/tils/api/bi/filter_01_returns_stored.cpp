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
  // filter returns the stored filter wrapped in an optional. It is a copy of the stored value.
  bi::Session session;
  session.set_filter(region_filter("region", "north"));
  const auto found = session.filter("region");
  std::cout << "found: " << found.has_value() << '\n';
  std::cout << "key: " << found->key << '\n';
  std::cout << "feature: " << found->feature << '\n';
  std::cout << "value: " << std::get<std::string>(std::get<pluss::bi::FilterScalar>(found->payload)) << '\n';
  return 0;
}
