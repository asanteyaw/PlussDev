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
  // Testing for a filter is a read. It never changes the version.
  bi::Session session;
  session.set_filter(region_filter("region", "north"));
  const auto before = session.version();
  (void)session.has_filter("region");
  (void)session.has_filter("missing");
  std::cout << "version unchanged: " << (session.version() == before) << '\n';
  return 0;
}
