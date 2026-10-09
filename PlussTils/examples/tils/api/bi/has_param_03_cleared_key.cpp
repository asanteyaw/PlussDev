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
  // A cleared key is absent, while an empty text value is still present.
  bi::Session session;
  session.set_param("note", std::string(""));
  std::cout << "empty text present: " << session.has_param("note") << '\n';
  session.clear_param("note");
  std::cout << "after clear present: " << session.has_param("note") << '\n';
  return 0;
}
