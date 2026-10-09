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
  // Clearing a key that holds no filter is not an error, and it still counts as a change.
  bi::Session session;
  session.set_filter(region_filter("region", "north"));
  const auto before = session.version();
  session.clear_filter("never_set");
  std::cout << "no error for absent key: true\n";
  std::cout << "version moved: " << (session.version() == before + 1) << '\n';
  std::cout << "original filter untouched: " << session.has_filter("region") << '\n';
  return 0;
}
