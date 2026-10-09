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
  std::cout << "before any parameter: " << session.has_param("threshold") << '\n';
  session.set_param("threshold", 2.5);
  std::cout << "after storing: " << session.has_param("threshold") << '\n';
  std::cout << "other key: " << session.has_param("other") << '\n';
  return 0;
}
