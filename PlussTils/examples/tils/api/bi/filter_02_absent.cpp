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
  bi::Session session;
  std::cout << "absent key has no value: " << !session.filter("region").has_value() << '\n';
  session.set_filter(region_filter("region", "north"));
  session.clear_filter("region");
  std::cout << "cleared key has no value: " << !session.filter("region").has_value() << '\n';
  return 0;
}
