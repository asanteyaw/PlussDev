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
  // The returned filter is a copy. Changing the session afterwards does not change it.
  bi::Session session;
  session.set_filter(region_filter("region", "north"));
  auto copy = session.filter("region");
  session.set_filter(region_filter("region", "south"));
  std::cout << "copy still north: "
            << (std::get<std::string>(std::get<pluss::bi::FilterScalar>(copy->payload)) == "north") << '\n';
  std::cout << "session now south: "
            << (std::get<std::string>(std::get<pluss::bi::FilterScalar>(session.filter("region")->payload)) == "south") << '\n';
  return 0;
}
