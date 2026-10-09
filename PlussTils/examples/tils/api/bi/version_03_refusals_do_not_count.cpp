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
  // A refused change throws before it is stored, so the version does not move.
  bi::Session session;
  session.set_param("a", 1.0);
  const auto before = session.version();
  std::cout << "refused: " << throws([&] { session.set_param("", 2.0); }) << '\n';
  std::cout << "version unchanged: " << (session.version() == before) << '\n';
  return 0;
}
