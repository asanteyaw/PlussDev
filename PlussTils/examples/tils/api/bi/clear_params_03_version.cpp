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
  session.clear_params();
  std::cout << "version after clear on empty params: " << session.version() << '\n';
  session.set_param("a", 1.0);
  session.set_param("b", 2.0);
  const auto before = session.version();
  session.clear_params();
  std::cout << "version moved by one for the whole clear: " << (session.version() == before + 1) << '\n';
  return 0;
}
