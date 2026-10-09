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
  // An empty key is never stored, so has_filter reports false rather than throwing.
  bi::Session session;
  std::cout << "empty key does not throw: " << !throws([&] { (void)session.has_filter(""); }) << '\n';
  std::cout << "empty key reports absent: " << !session.has_filter("") << '\n';
  return 0;
}
