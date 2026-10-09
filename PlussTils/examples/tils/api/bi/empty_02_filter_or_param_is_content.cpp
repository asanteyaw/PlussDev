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
  // A session is non-empty when it holds either a filter or a parameter.
  bi::Session session;
  session.set_param("threshold", 2.5);
  std::cout << "with a parameter: " << session.empty() << '\n';
  session.clear_params();
  std::cout << "after clearing parameters: " << session.empty() << '\n';
  session.set_filter(region_filter("region", "north"));
  std::cout << "with a filter: " << session.empty() << '\n';
  return 0;
}
